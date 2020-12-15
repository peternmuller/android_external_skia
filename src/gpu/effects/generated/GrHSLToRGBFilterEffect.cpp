/*
 * Copyright 2019 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrHSLToRGBFilterEffect.fp; do not modify.
 **************************************************************************************************/
#include "GrHSLToRGBFilterEffect.h"

#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLHSLToRGBFilterEffect : public GrGLSLFragmentProcessor {
public:
    GrGLSLHSLToRGBFilterEffect() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrHSLToRGBFilterEffect& _outer = args.fFp.cast<GrHSLToRGBFilterEffect>();
        (void)_outer;
        SkString _sample0 = this->invokeChild(0, args);
        fragBuilder->codeAppendf(
                R"SkSL(half4 color = %s;
half3 hsl = color.xyz;
half C = (1.0 - abs(2.0 * hsl.z - 1.0)) * hsl.y;
half3 p = hsl.xxx + half3(0.0, 0.66666668653488159, 0.3333333432674408);
half3 q = clamp(abs(fract(p) * 6.0 - 3.0) - 1.0, 0.0, 1.0);
half3 rgb = (q - 0.5) * C + hsl.z;
color = clamp(half4(rgb, color.w), 0.0, 1.0);
color.xyz *= color.w;
return color;
)SkSL",
                _sample0.c_str());
    }

private:
    void onSetData(const GrGLSLProgramDataManager& pdman,
                   const GrFragmentProcessor& _proc) override {}
};
GrGLSLFragmentProcessor* GrHSLToRGBFilterEffect::onCreateGLSLInstance() const {
    return new GrGLSLHSLToRGBFilterEffect();
}
void GrHSLToRGBFilterEffect::onGetGLSLProcessorKey(const GrShaderCaps& caps,
                                                   GrProcessorKeyBuilder* b) const {}
bool GrHSLToRGBFilterEffect::onIsEqual(const GrFragmentProcessor& other) const {
    const GrHSLToRGBFilterEffect& that = other.cast<GrHSLToRGBFilterEffect>();
    (void)that;
    return true;
}
bool GrHSLToRGBFilterEffect::usesExplicitReturn() const { return true; }
GrHSLToRGBFilterEffect::GrHSLToRGBFilterEffect(const GrHSLToRGBFilterEffect& src)
        : INHERITED(kGrHSLToRGBFilterEffect_ClassID, src.optimizationFlags()) {
    this->cloneAndRegisterAllChildProcessors(src);
}
std::unique_ptr<GrFragmentProcessor> GrHSLToRGBFilterEffect::clone() const {
    return std::make_unique<GrHSLToRGBFilterEffect>(*this);
}
#if GR_TEST_UTILS
SkString GrHSLToRGBFilterEffect::onDumpInfo() const { return SkString(); }
#endif
