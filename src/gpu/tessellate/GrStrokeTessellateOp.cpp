/*
 * Copyright 2020 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/tessellate/GrStrokeTessellateOp.h"

#include "src/core/SkPathPriv.h"
#include "src/gpu/tessellate/GrStrokePatchBuilder.h"
#include "src/gpu/tessellate/GrStrokeTessellateShader.h"

static SkPMColor4f get_paint_constant_blended_color(const GrPaint& paint) {
    SkPMColor4f constantColor;
    // Patches can overlap, so until a stencil technique is implemented, the provided paints must be
    // constant blended colors.
    SkAssertResult(paint.isConstantBlendedColor(&constantColor));
    return constantColor;
}

GrStrokeTessellateOp::GrStrokeTessellateOp(GrAAType aaType, const SkMatrix& viewMatrix,
                                           const SkStrokeRec& stroke, const SkPath& path,
                                           GrPaint&& paint)
        : GrDrawOp(ClassID())
        , fAAType(aaType)
        , fViewMatrix(viewMatrix)
        , fMatrixScale(fViewMatrix.getMaxScale())
        , fStroke(stroke)
        , fColor(get_paint_constant_blended_color(paint))
        , fProcessors(std::move(paint))
        , fPaths(path)
        , fTotalCombinedVerbCnt(path.countVerbs()) {
    SkASSERT(fAAType != GrAAType::kCoverage);  // No mixed samples support yet.
    SkASSERT(fMatrixScale >= 0);
    SkRect devBounds = path.getBounds();
    float inflationRadius = fStroke.getInflationRadius();
    devBounds.outset(inflationRadius, inflationRadius);
    viewMatrix.mapRect(&devBounds, devBounds);
    this->setBounds(devBounds, HasAABloat(GrAAType::kCoverage == fAAType), IsHairline::kNo);
}

GrDrawOp::FixedFunctionFlags GrStrokeTessellateOp::fixedFunctionFlags() const {
    auto flags = FixedFunctionFlags::kNone;
    if (GrAAType::kNone != fAAType) {
        flags |= FixedFunctionFlags::kUsesHWAA;
    }
    return flags;
}

GrProcessorSet::Analysis GrStrokeTessellateOp::finalize(const GrCaps& caps,
                                                        const GrAppliedClip* clip,
                                                        bool hasMixedSampledCoverage,
                                                        GrClampType clampType) {
    return fProcessors.finalize(fColor, GrProcessorAnalysisCoverage::kNone, clip,
                                &GrUserStencilSettings::kUnused, hasMixedSampledCoverage, caps,
                                clampType, &fColor);
}

GrOp::CombineResult GrStrokeTessellateOp::onCombineIfPossible(GrOp* grOp,
                                                              GrRecordingContext::Arenas* arenas,
                                                              const GrCaps&) {
    auto* op = grOp->cast<GrStrokeTessellateOp>();
    if (fColor != op->fColor ||
        fViewMatrix != op->fViewMatrix ||
        fAAType != op->fAAType ||
        !fStroke.hasEqualEffect(op->fStroke) ||
        fProcessors != op->fProcessors) {
        return CombineResult::kCannotCombine;
    }

    fPaths.concat(std::move(op->fPaths), arenas->recordTimeAllocator());
    fTotalCombinedVerbCnt += op->fTotalCombinedVerbCnt;

    return CombineResult::kMerged;
}

void GrStrokeTessellateOp::onPrePrepare(GrRecordingContext*, const GrSurfaceProxyView* writeView,
                                        GrAppliedClip*,
                                        const GrXferProcessor::DstProxyView&,
                                        GrXferBarrierFlags renderPassXferBarriers) {}

void GrStrokeTessellateOp::onPrepare(GrOpFlushState* flushState) {
    GrStrokePatchBuilder builder(flushState, &fPatchChunks, fMatrixScale, fStroke,
                                 fTotalCombinedVerbCnt);
    for (const SkPath& path : fPaths) {
        builder.addPath(path);
    }
}

void GrStrokeTessellateOp::onExecute(GrOpFlushState* flushState, const SkRect& chainBounds) {
    GrPipeline::InitArgs initArgs;
    if (GrAAType::kNone != fAAType) {
        initArgs.fInputFlags |= GrPipeline::InputFlags::kHWAntialias;
        SkASSERT(flushState->proxy()->numSamples() > 1);  // No mixed samples yet.
        SkASSERT(fAAType != GrAAType::kCoverage);  // No mixed samples yet.
    }
    initArgs.fCaps = &flushState->caps();
    initArgs.fDstProxyView = flushState->drawOpArgs().dstProxyView();
    initArgs.fWriteSwizzle = flushState->drawOpArgs().writeSwizzle();
    GrPipeline pipeline(initArgs, std::move(fProcessors), flushState->detachAppliedClip());

    GrStrokeTessellateShader strokeShader(fStroke, fMatrixScale, fViewMatrix, fColor);
    GrPathShader::ProgramInfo programInfo(flushState->writeView(), &pipeline, &strokeShader,
                                          flushState->renderPassBarriers());

    SkASSERT(chainBounds == this->bounds());
    flushState->bindPipelineAndScissorClip(programInfo, this->bounds());
    flushState->bindTextures(strokeShader, nullptr, pipeline);

    for (const auto& chunk : fPatchChunks) {
        if (chunk.fPatchBuffer) {
            flushState->bindBuffers(nullptr, nullptr, std::move(chunk.fPatchBuffer));
            flushState->draw(chunk.fPatchCount, chunk.fBasePatch);
        }
    }
}
