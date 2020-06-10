/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrLumaColorFilterEffect.fp; do not modify.
 **************************************************************************************************/
#ifndef GrLumaColorFilterEffect_DEFINED
#define GrLumaColorFilterEffect_DEFINED
#include "include/core/SkTypes.h"
#include "include/core/SkM44.h"

#include "src/gpu/GrCoordTransform.h"
#include "src/gpu/GrFragmentProcessor.h"
class GrLumaColorFilterEffect : public GrFragmentProcessor {
public:
#include "include/private/SkColorData.h"

    SkPMColor4f constantOutputForConstantInput(const SkPMColor4f& inColor) const override {
        SkPMColor4f input = this->numChildProcessors()
                                    ? ConstantOutputForConstantInput(
                                              this->childProcessor(inputFP_index), inColor)
                                    : inColor;
        float luma = SK_ITU_BT709_LUM_COEFF_R * input.fR + SK_ITU_BT709_LUM_COEFF_G * input.fG +
                     SK_ITU_BT709_LUM_COEFF_B * input.fB;
        return {0, 0, 0, SkTPin(luma, 0.0f, 1.0f)};
    }
    static std::unique_ptr<GrFragmentProcessor> Make(std::unique_ptr<GrFragmentProcessor> inputFP) {
        return std::unique_ptr<GrFragmentProcessor>(
                new GrLumaColorFilterEffect(std::move(inputFP)));
    }
    GrLumaColorFilterEffect(const GrLumaColorFilterEffect& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "LumaColorFilterEffect"; }
    int inputFP_index = -1;

private:
    GrLumaColorFilterEffect(std::unique_ptr<GrFragmentProcessor> inputFP)
            : INHERITED(kGrLumaColorFilterEffect_ClassID,
                        (OptimizationFlags)(inputFP ? ProcessorOptimizationFlags(inputFP.get())
                                                    : kAll_OptimizationFlags) &
                                kConstantOutputForConstantInput_OptimizationFlag) {
        if (inputFP) {
            inputFP_index = this->numChildProcessors();
            this->registerChildProcessor(std::move(inputFP));
        }
    }
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    typedef GrFragmentProcessor INHERITED;
};
#endif
