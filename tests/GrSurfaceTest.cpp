/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <set>
#include "GrClip.h"
#include "GrContext.h"
#include "GrContextPriv.h"
#include "GrGpu.h"
#include "GrProxyProvider.h"
#include "GrRenderTarget.h"
#include "GrResourceProvider.h"
#include "GrTexture.h"
#include "GrTexturePriv.h"
#include "SkAutoPixmapStorage.h"
#include "SkMipMap.h"
#include "SkSurface.h"
#include "Test.h"

// Tests that GrSurface::asTexture(), GrSurface::asRenderTarget(), and static upcasting of texture
// and render targets to GrSurface all work as expected.
DEF_GPUTEST_FOR_NULLGL_CONTEXT(GrSurface, reporter, ctxInfo) {
    GrContext* context = ctxInfo.grContext();
    auto resourceProvider = context->priv().resourceProvider();
    GrGpu* gpu = context->priv().getGpu();

    GrSurfaceDesc desc;
    desc.fFlags = kRenderTarget_GrSurfaceFlag;
    desc.fWidth = 256;
    desc.fHeight = 256;
    desc.fConfig = kRGBA_8888_GrPixelConfig;
    desc.fSampleCnt = 1;
    sk_sp<GrSurface> texRT1 = resourceProvider->createTexture(desc, SkBudgeted::kNo);

    REPORTER_ASSERT(reporter, texRT1.get() == texRT1->asRenderTarget());
    REPORTER_ASSERT(reporter, texRT1.get() == texRT1->asTexture());
    REPORTER_ASSERT(reporter, static_cast<GrSurface*>(texRT1->asRenderTarget()) ==
                    texRT1->asTexture());
    REPORTER_ASSERT(reporter, texRT1->asRenderTarget() ==
                    static_cast<GrSurface*>(texRT1->asTexture()));
    REPORTER_ASSERT(reporter, static_cast<GrSurface*>(texRT1->asRenderTarget()) ==
                    static_cast<GrSurface*>(texRT1->asTexture()));

    desc.fFlags = kNone_GrSurfaceFlags;
    sk_sp<GrTexture> tex1 = resourceProvider->createTexture(desc, SkBudgeted::kNo);
    REPORTER_ASSERT(reporter, nullptr == tex1->asRenderTarget());
    REPORTER_ASSERT(reporter, tex1.get() == tex1->asTexture());
    REPORTER_ASSERT(reporter, static_cast<GrSurface*>(tex1.get()) == tex1->asTexture());

    GrBackendTexture backendTex = gpu->createTestingOnlyBackendTexture(
        nullptr, 256, 256, GrColorType::kRGBA_8888, false, GrMipMapped::kNo);

    sk_sp<GrSurface> texRT2 = resourceProvider->wrapRenderableBackendTexture(
            backendTex, 1, kBorrow_GrWrapOwnership, GrWrapCacheable::kNo);

    REPORTER_ASSERT(reporter, texRT2.get() == texRT2->asRenderTarget());
    REPORTER_ASSERT(reporter, texRT2.get() == texRT2->asTexture());
    REPORTER_ASSERT(reporter, static_cast<GrSurface*>(texRT2->asRenderTarget()) ==
                    texRT2->asTexture());
    REPORTER_ASSERT(reporter, texRT2->asRenderTarget() ==
                    static_cast<GrSurface*>(texRT2->asTexture()));
    REPORTER_ASSERT(reporter, static_cast<GrSurface*>(texRT2->asRenderTarget()) ==
                    static_cast<GrSurface*>(texRT2->asTexture()));

    gpu->deleteTestingOnlyBackendTexture(backendTex);
}

// This test checks that the isConfigTexturable and isConfigRenderable are
// consistent with createTexture's result.
DEF_GPUTEST_FOR_ALL_CONTEXTS(GrSurfaceRenderability, reporter, ctxInfo) {
    GrContext* context = ctxInfo.grContext();
    GrProxyProvider* proxyProvider = context->priv().proxyProvider();
    GrResourceProvider* resourceProvider = context->priv().resourceProvider();
    const GrCaps* caps = context->priv().caps();

    GrPixelConfig configs[] = {
        kUnknown_GrPixelConfig,
        kAlpha_8_GrPixelConfig,
        kAlpha_8_as_Alpha_GrPixelConfig,
        kAlpha_8_as_Red_GrPixelConfig,
        kGray_8_GrPixelConfig,
        kGray_8_as_Lum_GrPixelConfig,
        kGray_8_as_Red_GrPixelConfig,
        kRGB_565_GrPixelConfig,
        kRGBA_4444_GrPixelConfig,
        kRGBA_8888_GrPixelConfig,
        kRGB_888_GrPixelConfig,
        kRG_88_GrPixelConfig,
        kBGRA_8888_GrPixelConfig,
        kSRGBA_8888_GrPixelConfig,
        kSBGRA_8888_GrPixelConfig,
        kRGBA_1010102_GrPixelConfig,
        kRGBA_float_GrPixelConfig,
        kRG_float_GrPixelConfig,
        kAlpha_half_GrPixelConfig,
        kAlpha_half_as_Red_GrPixelConfig,
        kRGBA_half_GrPixelConfig,
        kRGB_ETC1_GrPixelConfig,
    };
    GR_STATIC_ASSERT(kGrPixelConfigCnt == SK_ARRAY_COUNT(configs));

    GrSurfaceDesc desc;
    desc.fWidth = 64;
    desc.fHeight = 64;

    for (GrPixelConfig config : configs) {
        for (GrSurfaceOrigin origin : { kTopLeft_GrSurfaceOrigin, kBottomLeft_GrSurfaceOrigin }) {
            desc.fFlags = kNone_GrSurfaceFlags;
            desc.fConfig = config;
            desc.fSampleCnt = 1;

            sk_sp<GrSurface> tex = resourceProvider->createTexture(desc, SkBudgeted::kNo);
            bool ict = caps->isConfigTexturable(desc.fConfig);
            REPORTER_ASSERT(reporter, SkToBool(tex) == ict,
                            "config:%d, tex:%d, isConfigTexturable:%d", config, SkToBool(tex), ict);

            GrSRGBEncoded srgbEncoded = GrSRGBEncoded::kNo;
            GrColorType colorType = GrPixelConfigToColorTypeAndEncoding(config, &srgbEncoded);
            const GrBackendFormat format =
                    caps->getBackendFormatFromGrColorType(colorType, srgbEncoded);

            sk_sp<GrTextureProxy> proxy =
                    proxyProvider->createMipMapProxy(format, desc, origin, SkBudgeted::kNo);
            REPORTER_ASSERT(reporter, SkToBool(proxy.get()) ==
                            (caps->isConfigTexturable(desc.fConfig) &&
                             caps->mipMapSupport()));

            desc.fFlags = kRenderTarget_GrSurfaceFlag;
            tex = resourceProvider->createTexture(desc, SkBudgeted::kNo);
            bool isRenderable = caps->isConfigRenderable(config);
            REPORTER_ASSERT(reporter, SkToBool(tex) == isRenderable,
                            "config:%d, tex:%d, isRenderable:%d", config, SkToBool(tex),
                            isRenderable);

            desc.fSampleCnt = 2;
            tex = resourceProvider->createTexture(desc, SkBudgeted::kNo);
            isRenderable = SkToBool(caps->getRenderTargetSampleCount(2, config));
            REPORTER_ASSERT(reporter, SkToBool(tex) == isRenderable,
                            "config:%d, tex:%d, isRenderable:%d", config, SkToBool(tex),
                            isRenderable);
        }
    }
}

#include "GrDrawingManager.h"
#include "GrSurfaceProxy.h"
#include "GrTextureContext.h"

DEF_GPUTEST_FOR_RENDERING_CONTEXTS(InitialTextureClear, reporter, context_info) {
    static constexpr int kSize = 100;
    GrSurfaceDesc desc;
    desc.fWidth = desc.fHeight = kSize;
    std::unique_ptr<uint32_t[]> data(new uint32_t[kSize * kSize]);

    GrContext* context = context_info.grContext();
    const GrCaps* caps = context->priv().caps();
    GrProxyProvider* proxyProvider = context->priv().proxyProvider();

    for (int c = 0; c <= kLast_GrPixelConfig; ++c) {
        desc.fConfig = static_cast<GrPixelConfig>(c);
        if (!caps->isConfigTexturable(desc.fConfig)) {
            continue;
        }
        desc.fFlags = kPerformInitialClear_GrSurfaceFlag;
        for (bool rt : {false, true}) {
            if (rt && !caps->isConfigRenderable(desc.fConfig)) {
                continue;
            }
            desc.fFlags |= rt ? kRenderTarget_GrSurfaceFlag : kNone_GrSurfaceFlags;
            for (GrSurfaceOrigin origin :
                 {kTopLeft_GrSurfaceOrigin, kBottomLeft_GrSurfaceOrigin}) {
                for (auto fit : { SkBackingFit::kApprox, SkBackingFit::kExact }) {
                    // Try directly creating the texture.
                    // Do this twice in an attempt to hit the cache on the second time through.
                    for (int i = 0; i < 2; ++i) {
                        auto proxy = proxyProvider->testingOnly_createInstantiatedProxy(
                                desc, origin, fit, SkBudgeted::kYes);
                        if (!proxy) {
                            continue;
                        }
                        auto texCtx = context->priv().makeWrappedSurfaceContext(std::move(proxy));
                        SkImageInfo info = SkImageInfo::Make(
                                kSize, kSize, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
                        memset(data.get(), 0xAB, kSize * kSize * sizeof(uint32_t));
                        if (texCtx->readPixels(info, data.get(), 0, 0, 0)) {
                            uint32_t cmp = GrPixelConfigIsOpaque(desc.fConfig) ? 0xFF000000 : 0;
                            for (int i = 0; i < kSize * kSize; ++i) {
                                if (cmp != data.get()[i]) {
                                    ERRORF(reporter, "Failed on config %d", desc.fConfig);
                                    break;
                                }
                            }
                        }
                        memset(data.get(), 0xBC, kSize * kSize * sizeof(uint32_t));
                        // Here we overwrite the texture so that the second time through we
                        // test against recycling without reclearing.
                        if (0 == i) {
                            texCtx->writePixels(info, data.get(), 0, 0, 0);
                        }
                    }
                    context->priv().testingOnly_purgeAllUnlockedResources();

                    GrSRGBEncoded srgbEncoded = GrSRGBEncoded::kNo;
                    GrColorType colorType = GrPixelConfigToColorTypeAndEncoding(desc.fConfig,
                                                                                &srgbEncoded);
                    const GrBackendFormat format =
                            caps->getBackendFormatFromGrColorType(colorType, srgbEncoded);

                    // Try creating the texture as a deferred proxy.
                    for (int i = 0; i < 2; ++i) {
                        auto surfCtx = context->priv().makeDeferredSurfaceContext(
                                format, desc, origin, GrMipMapped::kNo, fit, SkBudgeted::kYes);
                        if (!surfCtx) {
                            continue;
                        }
                        SkImageInfo info = SkImageInfo::Make(
                                kSize, kSize, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
                        memset(data.get(), 0xAB, kSize * kSize * sizeof(uint32_t));
                        if (surfCtx->readPixels(info, data.get(), 0, 0, 0)) {
                            uint32_t cmp = GrPixelConfigIsOpaque(desc.fConfig) ? 0xFF000000 : 0;
                            for (int i = 0; i < kSize * kSize; ++i) {
                                if (cmp != data.get()[i]) {
                                    ERRORF(reporter, "Failed on config %d", desc.fConfig);
                                    break;
                                }
                            }
                        }
                        // Here we overwrite the texture so that the second time through we
                        // test against recycling without reclearing.
                        if (0 == i) {
                            surfCtx->writePixels(info, data.get(), 0, 0, 0);
                        }
                    }
                    context->priv().testingOnly_purgeAllUnlockedResources();
                }
            }
        }
    }
}

DEF_GPUTEST_FOR_RENDERING_CONTEXTS(ReadOnlyTexture, reporter, context_info) {
    auto fillPixels = [](const SkPixmap* p, const std::function<uint32_t(int x, int y)>& f) {
        for (int y = 0; y < p->height(); ++y) {
            for (int x = 0; x < p->width(); ++x) {
                *p->writable_addr32(x, y) = f(x, y);
            }
        }
    };

    auto comparePixels = [](const SkPixmap& p1, const SkPixmap& p2, skiatest::Reporter* reporter) {
        SkASSERT(p1.info() == p2.info());
        for (int y = 0; y < p1.height(); ++y) {
            for (int x = 0; x < p1.width(); ++x) {
                REPORTER_ASSERT(reporter, p1.getColor(x, y) == p2.getColor(x, y));
                if (p1.getColor(x, y) != p2.getColor(x, y)) {
                    return;
                }
            }
        }
    };

    static constexpr int kSize = 100;
    SkAutoPixmapStorage pixels;
    pixels.alloc(SkImageInfo::Make(kSize, kSize, kRGBA_8888_SkColorType, kPremul_SkAlphaType));
    fillPixels(&pixels, [](int x, int y) {
        return (0xFFU << 24) | (x << 16) | (y << 8) | uint8_t((x * y) & 0xFF);
    });

    GrContext* context = context_info.grContext();
    GrProxyProvider* proxyProvider = context->priv().proxyProvider();

    // We test both kRW in addition to kRead mostly to ensure that the calls are structured such
    // that they'd succeed if the texture wasn't kRead. We want to be sure we're failing with
    // kRead for the right reason.
    for (auto ioType : {kRead_GrIOType, kRW_GrIOType}) {
        auto backendTex = context->priv().getGpu()->createTestingOnlyBackendTexture(
                pixels.addr(), kSize, kSize, kRGBA_8888_SkColorType, true, GrMipMapped::kNo);
        auto proxy = proxyProvider->wrapBackendTexture(backendTex, kTopLeft_GrSurfaceOrigin,
                                                       kBorrow_GrWrapOwnership,
                                                       GrWrapCacheable::kNo, ioType);
        auto surfContext = context->priv().makeWrappedSurfaceContext(proxy);

        // Read pixels should work with a read-only texture.
        SkAutoPixmapStorage read;
        read.alloc(pixels.info());
        auto readResult = surfContext->readPixels(pixels.info(), read.writable_addr(), 0, 0, 0);
        REPORTER_ASSERT(reporter, readResult);
        if (readResult) {
            comparePixels(pixels, read, reporter);
        }

        // Write pixels should not work with a read-only texture.
        SkAutoPixmapStorage write;
        write.alloc(pixels.info());
        fillPixels(&write, [&pixels](int x, int y) { return ~*pixels.addr32(); });
        auto writeResult = surfContext->writePixels(pixels.info(), pixels.addr(), 0, 0, 0);
        REPORTER_ASSERT(reporter, writeResult == (ioType == kRW_GrIOType));
        // Try the low level write.
        context->flush();
        auto gpuWriteResult = context->priv().getGpu()->writePixels(
                proxy->peekTexture(), 0, 0, kSize, kSize, GrColorType::kRGBA_8888, write.addr32(),
                0);
        REPORTER_ASSERT(reporter, gpuWriteResult == (ioType == kRW_GrIOType));

        // Copies should not work with a read-only texture
        auto copySrc = proxyProvider->createTextureProxy(
                SkImage::MakeFromRaster(write, nullptr, nullptr), kNone_GrSurfaceFlags, 1,
                SkBudgeted::kYes, SkBackingFit::kExact);
        REPORTER_ASSERT(reporter, copySrc);
        auto copyResult = surfContext->copy(copySrc.get());
        REPORTER_ASSERT(reporter, copyResult == (ioType == kRW_GrIOType));
        // Try the low level copy.
        context->flush();
        auto gpuCopyResult = context->priv().getGpu()->copySurface(
                proxy->peekTexture(), kTopLeft_GrSurfaceOrigin, copySrc->peekTexture(),
                kTopLeft_GrSurfaceOrigin, SkIRect::MakeWH(kSize, kSize), {0, 0});
        REPORTER_ASSERT(reporter, gpuCopyResult == (ioType == kRW_GrIOType));

        // Mip regen should not work with a read only texture.
        if (context->priv().caps()->mipMapSupport()) {
            backendTex = context->priv().getGpu()->createTestingOnlyBackendTexture(
                    nullptr, kSize, kSize, kRGBA_8888_SkColorType, true, GrMipMapped::kYes);
            proxy = proxyProvider->wrapBackendTexture(backendTex, kTopLeft_GrSurfaceOrigin,
                                                      kBorrow_GrWrapOwnership, GrWrapCacheable::kNo,
                                                      ioType);
            context->flush();
            proxy->peekTexture()->texturePriv().markMipMapsDirty();  // avoids assert in GrGpu.
            auto regenResult =
                    context->priv().getGpu()->regenerateMipMapLevels(proxy->peekTexture());
            REPORTER_ASSERT(reporter, regenResult == (ioType == kRW_GrIOType));
        }
    }
}

DEF_GPUTEST(TextureIdleProcTest, reporter, options) {
    static const int kS = 10;

    // Helper to delete a backend texture in a GrTexture's release proc.
    static const auto installBackendTextureReleaseProc = [](GrTexture* texture) {
        auto backendTexture = texture->getBackendTexture();
        auto context = texture->getContext();
        struct ReleaseContext {
            GrContext* fContext;
            GrBackendTexture fBackendTexture;
        };
        auto release = [](void* rc) {
            auto releaseContext = static_cast<ReleaseContext*>(rc);
            if (!releaseContext->fContext->abandoned()) {
                if (auto gpu = releaseContext->fContext->priv().getGpu()) {
                    gpu->deleteTestingOnlyBackendTexture(releaseContext->fBackendTexture);
                }
            }
            delete releaseContext;
        };
        texture->setRelease(sk_make_sp<GrReleaseProcHelper>(
                release, new ReleaseContext{context, backendTexture}));
    };

    // Various ways of making textures.
    auto makeWrapped = [](GrContext* context) {
        auto backendTexture = context->priv().getGpu()->createTestingOnlyBackendTexture(
                nullptr, kS, kS, GrColorType::kRGBA_8888, false, GrMipMapped::kNo);
        auto texture = context->priv().resourceProvider()->wrapBackendTexture(
                backendTexture, kBorrow_GrWrapOwnership, GrWrapCacheable::kNo, kRW_GrIOType);
        installBackendTextureReleaseProc(texture.get());
        return texture;
    };

    auto makeWrappedRenderable = [](GrContext* context) {
        auto backendTexture = context->priv().getGpu()->createTestingOnlyBackendTexture(
                nullptr, kS, kS, GrColorType::kRGBA_8888, true, GrMipMapped::kNo);
        auto texture = context->priv().resourceProvider()->wrapRenderableBackendTexture(
                backendTexture, 1, kBorrow_GrWrapOwnership, GrWrapCacheable::kNo);
        installBackendTextureReleaseProc(texture.get());
        return texture;
    };

    auto makeNormal = [](GrContext* context) {
        GrSurfaceDesc desc;
        desc.fConfig = kRGBA_8888_GrPixelConfig;
        desc.fWidth = desc.fHeight = kS;
        return context->priv().resourceProvider()->createTexture(desc, SkBudgeted::kNo);
    };

    auto makeRenderable = [](GrContext* context) {
        GrSurfaceDesc desc;
        desc.fFlags = kRenderTarget_GrSurfaceFlag;
        desc.fConfig = kRGBA_8888_GrPixelConfig;
        desc.fWidth = desc.fHeight = kS;
        return context->priv().resourceProvider()->createTexture(desc, SkBudgeted::kNo);
    };

    std::function<sk_sp<GrTexture>(GrContext*)> makers[] = {makeWrapped, makeWrappedRenderable,
                                                            makeNormal, makeRenderable};

    // Add a unique key, or not.
    auto addKey = [](GrTexture* texture) {
        static uint32_t gN = 0;
        static const GrUniqueKey::Domain kDomain = GrUniqueKey::GenerateDomain();
        GrUniqueKey key;
        GrUniqueKey::Builder builder(&key, kDomain, 1);
        builder[0] = gN++;
        builder.finish();
        texture->resourcePriv().setUniqueKey(key);
    };
    auto dontAddKey = [](GrTexture* texture) {};
    std::function<void(GrTexture*)> keyAdders[] = {addKey, dontAddKey};

    for (const auto& m : makers) {
        for (const auto& keyAdder : keyAdders) {
            for (int type = 0; type < sk_gpu_test::GrContextFactory::kContextTypeCnt; ++type) {
                sk_gpu_test::GrContextFactory factory;
                auto contextType = static_cast<sk_gpu_test::GrContextFactory::ContextType>(type);
                GrContext* context = factory.get(contextType);
                if (!context) {
                    continue;
                }

                // The callback we add simply adds an integer to a set.
                std::set<int> idleIDs;
                struct Context {
                    std::set<int>* fIdleIDs;
                    int fNum;
                };
                auto proc = [](void* context) {
                    static_cast<Context*>(context)->fIdleIDs->insert(
                            static_cast<Context*>(context)->fNum);
                    delete static_cast<Context*>(context);
                };

                // Makes a texture, possibly adds a key, and sets the callback.
                auto make = [&m, &keyAdder, &proc, &idleIDs](GrContext* context, int num) {
                    sk_sp<GrTexture> texture = m(context);
                    texture->setIdleProc(proc, new Context{&idleIDs, num});
                    keyAdder(texture.get());
                    return texture;
                };

                auto texture = make(context, 1);
                REPORTER_ASSERT(reporter, idleIDs.find(1) == idleIDs.end());
                bool isRT = SkToBool(texture->asRenderTarget());
                auto backendFormat = texture->backendFormat();
                texture.reset();
                REPORTER_ASSERT(reporter, idleIDs.find(1) != idleIDs.end());

                texture = make(context, 2);
                SkImageInfo info =
                        SkImageInfo::Make(kS, kS, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
                auto rt = SkSurface::MakeRenderTarget(context, SkBudgeted::kNo, info, 0, nullptr);
                auto rtc = rt->getCanvas()->internal_private_accessTopLayerRenderTargetContext();
                auto singleUseLazyCB = [&texture](GrResourceProvider* rp) {
                    return rp ? std::move(texture) : nullptr;
                };
                GrSurfaceDesc desc;
                desc.fWidth = desc.fHeight = kS;
                desc.fConfig = kRGBA_8888_GrPixelConfig;
                if (isRT) {
                    desc.fFlags = kRenderTarget_GrSurfaceFlag;
                }
                SkBudgeted budgeted;
                if (texture->resourcePriv().budgetedType() == GrBudgetedType::kBudgeted) {
                    budgeted = SkBudgeted::kYes;
                } else {
                    budgeted = SkBudgeted::kNo;
                }
                auto proxy = context->priv().proxyProvider()->createLazyProxy(
                        singleUseLazyCB, backendFormat, desc,
                        GrSurfaceOrigin::kTopLeft_GrSurfaceOrigin, GrMipMapped::kNo,
                        GrInternalSurfaceFlags ::kNone, SkBackingFit::kExact, budgeted,
                        GrSurfaceProxy::LazyInstantiationType::kSingleUse);
                rtc->drawTexture(GrNoClip(), proxy, GrSamplerState::Filter::kNearest, SkPMColor4f(),
                                 SkRect::MakeWH(kS, kS), SkRect::MakeWH(kS, kS),
                                 GrQuadAAFlags::kNone, SkCanvas::kFast_SrcRectConstraint,
                                 SkMatrix::I(), nullptr);
                // We still have the proxy, which should remain instantiated, thereby keeping the
                // texture not purgeable.
                REPORTER_ASSERT(reporter, idleIDs.find(2) == idleIDs.end());
                context->flush();
                REPORTER_ASSERT(reporter, idleIDs.find(2) == idleIDs.end());
                context->priv().getGpu()->testingOnly_flushGpuAndSync();
                REPORTER_ASSERT(reporter, idleIDs.find(2) == idleIDs.end());

                // This time we move the proxy into the draw.
                rtc->drawTexture(GrNoClip(), std::move(proxy), GrSamplerState::Filter::kNearest,
                                 SkPMColor4f(), SkRect::MakeWH(kS, kS), SkRect::MakeWH(kS, kS),
                                 GrQuadAAFlags::kNone, SkCanvas::kFast_SrcRectConstraint,
                                 SkMatrix::I(), nullptr);
                REPORTER_ASSERT(reporter, idleIDs.find(2) == idleIDs.end());
                context->flush();
                context->priv().getGpu()->testingOnly_flushGpuAndSync();
                // Now that the draw is fully consumed by the GPU, the texture should be idle.
                REPORTER_ASSERT(reporter, idleIDs.find(2) != idleIDs.end());

                // Make a proxy that should deinstantiate even if we keep a ref on it.
                auto deinstantiateLazyCB = [&make, &context](GrResourceProvider* rp) {
                    return rp ? make(context, 3) : nullptr;
                };
                proxy = context->priv().proxyProvider()->createLazyProxy(
                        deinstantiateLazyCB, backendFormat, desc,
                        GrSurfaceOrigin::kTopLeft_GrSurfaceOrigin, GrMipMapped::kNo,
                        GrInternalSurfaceFlags ::kNone, SkBackingFit::kExact, budgeted,
                        GrSurfaceProxy::LazyInstantiationType::kDeinstantiate);
                rtc->drawTexture(GrNoClip(), std::move(proxy), GrSamplerState::Filter::kNearest,
                                 SkPMColor4f(), SkRect::MakeWH(kS, kS), SkRect::MakeWH(kS, kS),
                                 GrQuadAAFlags::kNone, SkCanvas::kFast_SrcRectConstraint,
                                 SkMatrix::I(), nullptr);
                // At this point the proxy shouldn't even be instantiated, there is no texture with
                // id 3.
                REPORTER_ASSERT(reporter, idleIDs.find(3) == idleIDs.end());
                context->flush();
                context->priv().getGpu()->testingOnly_flushGpuAndSync();
                // Now that the draw is fully consumed, we should have deinstantiated the proxy and
                // the texture it made should be idle.
                REPORTER_ASSERT(reporter, idleIDs.find(3) != idleIDs.end());

                // Make sure we make the call during various shutdown scenarios where the texture
                // might persist after context is destroyed, abandoned, etc. We test three
                // variations of each scenario. One where the texture is just created. Another,
                // where the texture has been used in a draw and then the context is flushed. And
                // one where the the texture was drawn but the context is not flushed.
                // In each scenario we test holding a ref beyond the context shutdown and not.

                // These tests are difficult to get working with Vulkan. See http://skbug.com/8705
                // and http://skbug.com/8275
                GrBackendApi api = sk_gpu_test::GrContextFactory::ContextTypeBackend(contextType);
                if (api == GrBackendApi::kVulkan) {
                    continue;
                }
                int id = 4;
                enum class DrawType {
                    kNoDraw,
                    kDraw,
                    kDrawAndFlush,
                };
                for (auto drawType :
                     {DrawType::kNoDraw, DrawType::kDraw, DrawType::kDrawAndFlush}) {
                    for (bool unrefFirst : {false, true}) {
                        auto possiblyDrawAndFlush = [&context, &texture, drawType, unrefFirst] {
                            if (drawType == DrawType::kNoDraw) {
                                return;
                            }
                            SkImageInfo info = SkImageInfo::Make(kS, kS, kRGBA_8888_SkColorType,
                                                                 kPremul_SkAlphaType);
                            auto rt = SkSurface::MakeRenderTarget(context, SkBudgeted::kNo, info, 0,
                                                                  nullptr);
                            auto rtc = rt->getCanvas()
                                            ->internal_private_accessTopLayerRenderTargetContext();
                            auto proxy = context->priv().proxyProvider()->testingOnly_createWrapped(
                                                         texture, kTopLeft_GrSurfaceOrigin);
                            rtc->drawTexture(GrNoClip(), proxy, GrSamplerState::Filter::kNearest,
                                             SkPMColor4f(), SkRect::MakeWH(kS, kS),
                                             SkRect::MakeWH(kS, kS), GrQuadAAFlags::kNone,
                                             SkCanvas::kFast_SrcRectConstraint, SkMatrix::I(),
                                             nullptr);
                            if (drawType == DrawType::kDrawAndFlush) {
                                context->flush();
                            }
                            if (unrefFirst) {
                                texture.reset();
                            }
                        };
                        texture = make(context, id);
                        possiblyDrawAndFlush();
                        context->abandonContext();
                        texture.reset();
                        REPORTER_ASSERT(reporter, idleIDs.find(id) != idleIDs.end());
                        factory.destroyContexts();
                        context = factory.get(contextType);
                        ++id;

                        // Similar to previous, but reset the texture after the context was
                        // abandoned and then destroyed.
                        texture = make(context, id);
                        possiblyDrawAndFlush();
                        context->abandonContext();
                        factory.destroyContexts();
                        texture.reset();
                        REPORTER_ASSERT(reporter, idleIDs.find(id) != idleIDs.end());
                        context = factory.get(contextType);
                        id++;

                        texture = make(context, id);
                        possiblyDrawAndFlush();
                        factory.destroyContexts();
                        texture.reset();
                        REPORTER_ASSERT(reporter, idleIDs.find(id) != idleIDs.end());
                        context = factory.get(contextType);
                        id++;

                        texture = make(context, id);
                        possiblyDrawAndFlush();
                        factory.releaseResourcesAndAbandonContexts();
                        texture.reset();
                        REPORTER_ASSERT(reporter, idleIDs.find(id) != idleIDs.end());
                        context = factory.get(contextType);
                        id++;
                    }
                }
            }
        }
    }
}
