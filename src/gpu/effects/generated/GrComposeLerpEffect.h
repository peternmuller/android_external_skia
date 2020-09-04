/*
 * Copyright 2019 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrComposeLerpEffect.fp; do not modify.
 **************************************************************************************************/
#ifndef GrComposeLerpEffect_DEFINED
#define GrComposeLerpEffect_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"

#include "src/gpu/GrFragmentProcessor.h"

class GrComposeLerpEffect : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make(std::unique_ptr<GrFragmentProcessor> child1,
                                                     std::unique_ptr<GrFragmentProcessor> child2,
                                                     float weight) {
        return std::unique_ptr<GrFragmentProcessor>(
                new GrComposeLerpEffect(std::move(child1), std::move(child2), weight));
    }
    GrComposeLerpEffect(const GrComposeLerpEffect& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "ComposeLerpEffect"; }
    bool usesExplicitReturn() const override;
    float weight;

private:
    GrComposeLerpEffect(std::unique_ptr<GrFragmentProcessor> child1,
                        std::unique_ptr<GrFragmentProcessor> child2,
                        float weight)
            : INHERITED(kGrComposeLerpEffect_ClassID, kNone_OptimizationFlags), weight(weight) {
        this->registerChild(std::move(child1), SkSL::SampleUsage::PassThrough());
        this->registerChild(std::move(child2), SkSL::SampleUsage::PassThrough());
    }
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
#if GR_TEST_UTILS
    SkString onDumpInfo() const override;
#endif
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    using INHERITED = GrFragmentProcessor;
};
#endif
