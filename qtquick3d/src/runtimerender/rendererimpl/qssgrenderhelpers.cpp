// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderhelpers_p.h"

#include "qssgrenderer_p.h"
#include "qssglayerrenderdata_p.h"
#include "qssgrhiparticles_p.h"
#include "qssgrhiquadrenderer_p.h"
#include "../qssgrendercontextcore.h"
#include "../qssgrhicustommaterialsystem_p.h"
#include "../resourcemanager/qssgrenderbuffermanager_p.h"
#include "../qssgrenderdefaultmaterialshadergenerator_p.h"
#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <QtCore/qbitarray.h>

QT_BEGIN_NAMESPACE

static constexpr float QSSG_PI = float(M_PI);
static constexpr float QSSG_HALFPI = float(M_PI_2);

static const QRhiShaderResourceBinding::StageFlags RENDERER_VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

static QSSGRhiShaderPipelinePtr shadersForDefaultMaterial(QSSGRhiGraphicsPipelineState *ps,
                                                          QSSGSubsetRenderable &subsetRenderable,
                                                          const QSSGShaderFeatures &featureSet)
{
    auto &renderer(subsetRenderable.renderer);
    const auto &shaderPipeline = QSSGRendererPrivate::getShaderPipelineForDefaultMaterial(*renderer, subsetRenderable, featureSet);
    if (shaderPipeline)
        QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(*ps, shaderPipeline.get());
    return shaderPipeline;
}

static QSSGRhiShaderPipelinePtr shadersForParticleMaterial(QSSGRhiGraphicsPipelineState *ps,
                                                           QSSGParticlesRenderable &particleRenderable)
{
    const auto &renderer(particleRenderable.renderer);
    const auto &shaderCache = renderer->contextInterface()->shaderCache();
    auto featureLevel = particleRenderable.particles.m_featureLevel;
    const auto &shaderPipeline = shaderCache->getBuiltInRhiShaders().getRhiParticleShader(featureLevel);
    if (shaderPipeline)
        QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(*ps, shaderPipeline.get());
    return shaderPipeline;
}

static void updateUniformsForDefaultMaterial(QSSGRhiShaderPipeline &shaderPipeline,
                                             QSSGRhiContext *rhiCtx,
                                             const QSSGLayerRenderData &inData,
                                             char *ubufData,
                                             QSSGRhiGraphicsPipelineState *ps,
                                             QSSGSubsetRenderable &subsetRenderable,
                                             const QSSGRenderCamera &camera,
                                             const QVector2D *depthAdjust,
                                             const QMatrix4x4 *alteredModelViewProjection)
{
    const auto &renderer(subsetRenderable.renderer);
    const QMatrix4x4 clipSpaceCorrMatrix = rhiCtx->rhi()->clipSpaceCorrMatrix();
    const QMatrix4x4 &mvp(alteredModelViewProjection ? *alteredModelViewProjection
                                                     : subsetRenderable.modelContext.modelViewProjection);

    const auto &modelNode = subsetRenderable.modelContext.model;
    QRhiTexture *lightmapTexture = inData.getLightmapTexture(subsetRenderable.modelContext);

    const QMatrix4x4 &localInstanceTransform(modelNode.localInstanceTransform);
    const QMatrix4x4 &globalInstanceTransform(modelNode.globalInstanceTransform);
    const QMatrix4x4 &modelMatrix(modelNode.usesBoneTexture() ? QMatrix4x4() : subsetRenderable.globalTransform);

    QSSGMaterialShaderGenerator::setRhiMaterialProperties(*renderer->contextInterface(),
                                                          shaderPipeline,
                                                          ubufData,
                                                          ps,
                                                          subsetRenderable.material,
                                                          subsetRenderable.shaderDescription,
                                                          inData.getDefaultMaterialPropertyTable(),
                                                          camera,
                                                          mvp,
                                                          subsetRenderable.modelContext.normalMatrix,
                                                          modelMatrix,
                                                          clipSpaceCorrMatrix,
                                                          localInstanceTransform,
                                                          globalInstanceTransform,
                                                          toDataView(modelNode.morphWeights),
                                                          subsetRenderable.firstImage,
                                                          subsetRenderable.opacity,
                                                          inData,
                                                          subsetRenderable.lights,
                                                          subsetRenderable.reflectionProbe,
                                                          subsetRenderable.renderableFlags.receivesShadows(),
                                                          subsetRenderable.renderableFlags.receivesReflections(),
                                                          depthAdjust,
                                                          lightmapTexture);
}

std::pair<QSSGBoxPoints, QSSGBoxPoints> RenderHelpers::calculateSortedObjectBounds(const QSSGRenderableObjectList &sortedOpaqueObjects,
                                                                                   const QSSGRenderableObjectList &sortedTransparentObjects)
{
    QSSGBounds3 boundsCasting;
    QSSGBounds3 boundsReceiving;
    for (const auto handles : { &sortedOpaqueObjects, &sortedTransparentObjects }) {
        // Since we may have nodes that are not a child of the camera parent we go through all
        // the opaque objects and include them in the bounds. Failing to do this can result in
        // too small bounds.
        for (const QSSGRenderableObjectHandle &handle : *handles) {
            const QSSGRenderableObject &obj = *handle.obj;
            // We skip objects not casting or receiving shadows since they don't influence or need to be covered by the shadow map
            if (obj.renderableFlags.castsShadows())
                boundsCasting.include(obj.globalBounds);
            if (obj.renderableFlags.receivesShadows())
                boundsReceiving.include(obj.globalBounds);
        }
    }
    return { boundsCasting.toQSSGBoxPointsNoEmptyCheck(), boundsReceiving.toQSSGBoxPointsNoEmptyCheck() };
}

static QVector3D calcCenter(const QSSGBoxPoints &vertices)
{
    QVector3D center = vertices[0];
    for (int i = 1; i < 8; ++i) {
        center += vertices[i];
    }
    return center * 0.125f;
}

static QSSGBounds3 calculateShadowCameraBoundingBox(const QSSGBoxPoints &points, const QVector3D &forward, const QVector3D &up, const QVector3D &right)
{
    QSSGBounds3 bounds;
    for (int i = 0; i < 8; ++i) {
        const float distanceZ = QVector3D::dotProduct(points[i], forward);
        const float distanceY = QVector3D::dotProduct(points[i], up);
        const float distanceX = QVector3D::dotProduct(points[i], right);
        bounds.include(QVector3D(distanceX, distanceY, distanceZ));
    }
    return bounds;
}

static QSSGBoxPoints computeFrustumBounds(const QSSGRenderCamera &inCamera)
{
    QMatrix4x4 viewProjection;
    inCamera.calculateViewProjectionMatrix(viewProjection);

    bool invertible = false;
    QMatrix4x4 inv = viewProjection.inverted(&invertible);
    Q_ASSERT(invertible);

    return { inv.map(QVector3D(-1, -1, -1)), inv.map(QVector3D(+1, -1, -1)), inv.map(QVector3D(+1, +1, -1)),
             inv.map(QVector3D(-1, +1, -1)), inv.map(QVector3D(-1, -1, +1)), inv.map(QVector3D(+1, -1, +1)),
             inv.map(QVector3D(+1, +1, +1)), inv.map(QVector3D(-1, +1, +1)) };
}

static void setupCubeReflectionCameras(const QSSGRenderReflectionProbe *inProbe, QSSGRenderCamera inCameras[6])
{
    Q_ASSERT(inProbe != nullptr);

    // setup light matrix
    quint32 mapRes = 1 << inProbe->reflectionMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    static const QQuaternion rotOfs[6] {
                                         QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(-QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
                                         QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
                                         QQuaternion::fromEulerAngles(qRadiansToDegrees(QSSG_HALFPI), 0.f, 0.f),
                                         QQuaternion::fromEulerAngles(qRadiansToDegrees(-QSSG_HALFPI), 0.f, 0.f),
                                         QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_PI), qRadiansToDegrees(-QSSG_PI)),
                                         QQuaternion::fromEulerAngles(0.f, 0.f, qRadiansToDegrees(QSSG_PI)),
                                         };

    const QVector3D inProbePos = inProbe->getGlobalPos();
    const QVector3D inProbePivot = inProbe->pivot;

    for (int i = 0; i < 6; ++i) {
        inCameras[i].parent = nullptr;
        inCameras[i].clipNear = 1.0f;
        inCameras[i].clipFar = qMax<float>(2.0f, 10000.0f);
        inCameras[i].fov = qDegreesToRadians(90.f);

        inCameras[i].localTransform = QSSGRenderNode::calculateTransformMatrix(inProbePos, QSSGRenderNode::initScale, inProbePivot, rotOfs[i]);
        inCameras[i].calculateGlobalVariables(theViewport);
    }
}

static void setupCameraForShadowMap(const QSSGRenderCamera &inCamera,
                                    const QSSGRenderLight *inLight,
                                    QSSGRenderCamera &theCamera,
                                    const QSSGBoxPoints &castingBox,
                                    const QSSGBoxPoints &receivingBox)
{
    using namespace RenderHelpers;

    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    theCamera.clipNear = 1.0f;
    theCamera.clipFar = inLight->m_shadowMapFar;
    // Setup camera projection
    QVector3D inLightPos = inLight->getGlobalPos();
    QVector3D inLightDir = inLight->getDirection();
    QVector3D inLightPivot = inLight->pivot;

    inLightPos -= inLightDir * inCamera.clipNear;
    theCamera.fov = qDegreesToRadians(90.f);
    theCamera.parent = nullptr;

    if (inLight->type == QSSGRenderLight::Type::DirectionalLight) {
        Q_ASSERT(theCamera.type == QSSGRenderCamera::Type::OrthographicCamera);
        const QVector3D forward = inLightDir.normalized();
        const QVector3D right = qFuzzyCompare(qAbs(forward.y()), 1.0f)
                ? QVector3D::crossProduct(forward, QVector3D(1, 0, 0)).normalized()
                : QVector3D::crossProduct(forward, QVector3D(0, 1, 0)).normalized();
        const QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        // Calculate bounding box of the scene camera frustum
        const QSSGBoxPoints frustumPoints = computeFrustumBounds(inCamera);
        const QSSGBounds3 frustumBounds = calculateShadowCameraBoundingBox(frustumPoints, forward, up, right);
        const QSSGBounds3 sceneCastingBounds = calculateShadowCameraBoundingBox(castingBox, forward, up, right);
        const QSSGBounds3 boundsReceiving = calculateShadowCameraBoundingBox(receivingBox, forward, up, right);

        QVector3D finalDims;
        QVector3D center;
        // Select smallest bounds from either scene or camera frustum
        if (sceneCastingBounds.isFinite() && boundsReceiving.isFinite() // handle empty scene
            && sceneCastingBounds.extents().lengthSquared() < frustumBounds.extents().lengthSquared()) {
            center = calcCenter(castingBox);
            const QVector3D centerReceiving = calcCenter(receivingBox);

            // Since we need to make sure every rendered geometry can get a valid depth value from the shadow map
            // we need to expand the scene bounding box along its z-axis so that it covers also receiving objects in the scene.
            //
            // We take the z dimensions of the casting bounds and expand it to include the z dimensions of the receiving objects.
            // We call the casting bounding box 'a' and the receiving bounding box 'b'.

            // length of boxes
            const float aLength = sceneCastingBounds.dimensions().z();
            const float bLength = boundsReceiving.dimensions().z();

            // center position of boxes
            const float aCenter = QVector3D::dotProduct(center, forward);
            const float bCenter = QVector3D::dotProduct(centerReceiving, forward);

            // distance between boxes
            const float d = bCenter - aCenter;

            // start/end positions
            const float a0 = 0.f;
            const float a1 = aLength;
            const float b0 = (aLength * 0.5f) + d - (bLength * 0.5f);
            const float b1 = (aLength * 0.5f) + d + (bLength * 0.5f);

            // goal start/end position
            const float ap0 = qMin(a0, b0);
            const float ap1 = qMax(a1, b1);
            // goal length
            const float length = ap1 - ap0;
            // goal center postion
            const float c = (ap1 + ap0) * 0.5f;

            // how much to move in forward direction
            const float move = c - aLength * 0.5f;

            center = center + forward * move;
            finalDims = sceneCastingBounds.dimensions();
            finalDims.setZ(length);
        } else {
            center = calcCenter(frustumPoints);
            finalDims = frustumBounds.dimensions();
        }

        // Expand dimensions a little bit to avoid precision problems
        finalDims *= 1.05f;

        // Apply bounding box parameters to shadow map camera projection matrix
        // so that the whole scene is fit inside the shadow map
        theViewport.setHeight(finalDims.y());
        theViewport.setWidth(finalDims.x());
        theCamera.clipNear = -0.5f * finalDims.z();
        theCamera.clipFar = 0.5f * finalDims.z();
        theCamera.localTransform = QSSGRenderNode::calculateTransformMatrix(center, QSSGRenderNode::initScale, inLightPivot, QQuaternion::fromDirection(forward, up));
    } else if (inLight->type == QSSGRenderLight::Type::PointLight) {
        theCamera.lookAt(inLightPos, QVector3D(0, 1.0, 0), QVector3D(0, 0, 0), inLightPivot);
    }

    theCamera.calculateGlobalVariables(theViewport);
}

static void addOpaqueDepthPrePassBindings(QSSGRhiContext *rhiCtx,
                                          QSSGRhiShaderPipeline *shaderPipeline,
                                          QSSGRenderableImage *renderableImage,
                                          QSSGRhiShaderResourceBindingList &bindings,
                                          bool isCustomMaterialMeshSubset = false)
{
    static const auto imageAffectsAlpha = [](QSSGRenderableImage::Type mapType) {
        return mapType == QSSGRenderableImage::Type::BaseColor ||
                mapType == QSSGRenderableImage::Type::Diffuse ||
                mapType == QSSGRenderableImage::Type::Translucency ||
                mapType == QSSGRenderableImage::Type::Opacity;
    };

    while (renderableImage) {
        const auto mapType = renderableImage->m_mapType;
        if (imageAffectsAlpha(mapType)) {
            const char *samplerName = QSSGMaterialShaderGenerator::getSamplerName(mapType);
            const int samplerHint = int(mapType);
            int samplerBinding = shaderPipeline->bindingForTexture(samplerName, samplerHint);
            if (samplerBinding >= 0) {
                QRhiTexture *texture = renderableImage->m_texture.m_texture;
                if (samplerBinding >= 0 && texture) {
                    const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
                    QRhiSampler *sampler = rhiCtx->sampler({ QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_minFilterType),
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_magFilterType),
                            mipmapped ? QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_mipFilterType) : QRhiSampler::None,
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_horizontalTilingMode),
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_verticalTilingMode),
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_depthTilingMode)
                    });
                    bindings.addTexture(samplerBinding, RENDERER_VISIBILITY_ALL, texture, sampler);
                }
            } // else this is not necessarily an error, e.g. having metalness/roughness maps with metalness disabled
        }
        renderableImage = renderableImage->m_nextImage;
    }
    // For custom Materials we can't know which maps affect alpha, so map all
    if (isCustomMaterialMeshSubset) {
        QVector<QShaderDescription::InOutVariable> samplerVars =
                shaderPipeline->fragmentStage()->shader().description().combinedImageSamplers();
        for (const QShaderDescription::InOutVariable &var : shaderPipeline->vertexStage()->shader().description().combinedImageSamplers()) {
            auto it = std::find_if(samplerVars.cbegin(), samplerVars.cend(),
                                   [&var](const QShaderDescription::InOutVariable &v) { return var.binding == v.binding; });
            if (it == samplerVars.cend())
                samplerVars.append(var);
        }

        int maxSamplerBinding = -1;
        for (const QShaderDescription::InOutVariable &var : samplerVars)
            maxSamplerBinding = qMax(maxSamplerBinding, var.binding);

        // Will need to set unused image-samplers to something dummy
        // because the shader code contains all custom property textures,
        // and not providing a binding for all of them is invalid with some
        // graphics APIs (and will need a real texture because setting a
        // null handle or similar is not permitted with some of them so the
        // srb does not accept null QRhiTextures either; but first let's
        // figure out what bindings are unused in this frame)
        QBitArray samplerBindingsSpecified(maxSamplerBinding + 1);

        if (maxSamplerBinding >= 0) {
            // custom property textures
            int customTexCount = shaderPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(shaderPipeline->extraTextureAt(i));
                const int samplerBinding = shaderPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    samplerBindingsSpecified.setBit(samplerBinding);
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    bindings.addTexture(samplerBinding,
                                        RENDERER_VISIBILITY_ALL,
                                        t.texture,
                                        sampler);
                }
            }
        }

        // use a dummy texture for the unused samplers in the shader
        QRhiSampler *dummySampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
        QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
        QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
        QRhiTexture *dummyCubeTexture = rhiCtx->dummyTexture(QRhiTexture::CubeMap, resourceUpdates);
        rhiCtx->commandBuffer()->resourceUpdate(resourceUpdates);

        for (const QShaderDescription::InOutVariable &var : samplerVars) {
            if (!samplerBindingsSpecified.testBit(var.binding)) {
                QRhiTexture *t = var.type == QShaderDescription::SamplerCube ? dummyCubeTexture : dummyTexture;
                bindings.addTexture(var.binding, RENDERER_VISIBILITY_ALL, t, dummySampler);
            }
        }
    }
}

static void setupCubeShadowCameras(const QSSGRenderLight *inLight, QSSGRenderCamera inCameras[6])
{
    Q_ASSERT(inLight != nullptr);
    Q_ASSERT(inLight->type != QSSGRenderLight::Type::DirectionalLight);

    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    static const QQuaternion rotOfs[6] {
                                         QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(-QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
                                         QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
                                         QQuaternion::fromEulerAngles(qRadiansToDegrees(QSSG_HALFPI), 0.f, 0.f),
                                         QQuaternion::fromEulerAngles(qRadiansToDegrees(-QSSG_HALFPI), 0.f, 0.f),
                                         QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_PI), qRadiansToDegrees(-QSSG_PI)),
                                         QQuaternion::fromEulerAngles(0.f, 0.f, qRadiansToDegrees(QSSG_PI)),
                                         };

    const QVector3D inLightPos = inLight->getGlobalPos();
    constexpr QVector3D lightPivot = QVector3D(0, 0, 0);

    for (int i = 0; i < 6; ++i) {
        inCameras[i].parent = nullptr;
        inCameras[i].clipNear = 1.0f;
        inCameras[i].clipFar = qMax<float>(2.0f, inLight->m_shadowMapFar);
        inCameras[i].fov = qDegreesToRadians(90.f);
        inCameras[i].localTransform = QSSGRenderNode::calculateTransformMatrix(inLightPos, QSSGRenderNode::initScale, lightPivot, rotOfs[i]);
        inCameras[i].calculateGlobalVariables(theViewport);
    }

    /*
        if ( inLight->type == RenderLightTypes::Point ) return;

    QVector3D viewDirs[6];
    QVector3D viewUp[6];
    QMatrix3x3 theDirMatrix( inLight->m_GlobalTransform.getUpper3x3() );

    viewDirs[0] = theDirMatrix.transform( QVector3D( 1.f, 0.f, 0.f ) );
    viewDirs[2] = theDirMatrix.transform( QVector3D( 0.f, -1.f, 0.f ) );
    viewDirs[4] = theDirMatrix.transform( QVector3D( 0.f, 0.f, 1.f ) );
    viewDirs[0].normalize();  viewDirs[2].normalize();  viewDirs[4].normalize();
    viewDirs[1] = -viewDirs[0];
    viewDirs[3] = -viewDirs[2];
    viewDirs[5] = -viewDirs[4];

    viewUp[0] = viewDirs[2];
    viewUp[1] = viewDirs[2];
    viewUp[2] = viewDirs[5];
    viewUp[3] = viewDirs[4];
    viewUp[4] = viewDirs[2];
    viewUp[5] = viewDirs[2];

    for (int i = 0; i < 6; ++i)
    {
        inCameras[i].LookAt( inLightPos, viewUp[i], inLightPos + viewDirs[i] );
        inCameras[i].CalculateGlobalVariables( theViewport, QVector2D( theViewport.m_Width,
                                                                     theViewport.m_Height ) );
    }
    */
}

static int setupInstancing(QSSGSubsetRenderable *renderable, QSSGRhiGraphicsPipelineState *ps, QSSGRhiContext *rhiCtx, const QVector3D &cameraDirection, const QVector3D &cameraPosition)
{
    // TODO: non-static so it can be used from QSSGCustomMaterialSystem::rhiPrepareRenderable()?
    const bool instancing = QSSGLayerRenderData::prepareInstancing(rhiCtx, renderable, cameraDirection, cameraPosition, renderable->instancingLodMin, renderable->instancingLodMax);
    int instanceBufferBinding = 0;
    if (instancing) {
        auto &ia = QSSGRhiInputAssemblerStatePrivate::get(*ps);
        // set up new bindings for instanced buffers
        const quint32 stride = renderable->modelContext.model.instanceTable->stride();
        QVarLengthArray<QRhiVertexInputBinding, 8> bindings;
        std::copy(ia.inputLayout.cbeginBindings(), ia.inputLayout.cendBindings(), std::back_inserter(bindings));
        bindings.append({ stride, QRhiVertexInputBinding::PerInstance });
        instanceBufferBinding = bindings.size() - 1;
        ia.inputLayout.setBindings(bindings.cbegin(), bindings.cend());
    }
    return instanceBufferBinding;
}

static void rhiPrepareResourcesForReflectionMap(QSSGRhiContext *rhiCtx,
                                                QSSGPassKey passKey,
                                                const QSSGLayerRenderData &inData,
                                                QSSGReflectionMapEntry *pEntry,
                                                QSSGRhiGraphicsPipelineState *ps,
                                                const QSSGRenderableObjectList &sortedOpaqueObjects,
                                                QSSGRenderCamera &inCamera,
                                                QSSGRenderer &renderer,
                                                QSSGRenderTextureCubeFace cubeFace)
{
    using namespace RenderHelpers;

    if ((inData.layer.background == QSSGRenderLayer::Background::SkyBox && inData.layer.lightProbe) ||
         inData.layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap)
        rhiPrepareSkyBoxForReflectionMap(rhiCtx, passKey, inData.layer, inCamera, renderer, pEntry, cubeFace);

    QSSGShaderFeatures features = inData.getShaderFeatures();
    const auto &defaultMaterialShaderKeyProperties = inData.getDefaultMaterialPropertyTable();

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject &inObject = *handle.obj;

        QMatrix4x4 modelViewProjection;
        if (inObject.type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || inObject.type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
            QSSGSubsetRenderable &renderable(static_cast<QSSGSubsetRenderable &>(inObject));
            const bool hasSkinning = defaultMaterialShaderKeyProperties.m_boneCount.getValue(renderable.shaderDescription) > 0;
            modelViewProjection = hasSkinning ? pEntry->m_viewProjection
                                              : pEntry->m_viewProjection * renderable.globalTransform;
        }

        rhiPrepareRenderable(rhiCtx, passKey, inData, inObject, pEntry->m_rhiRenderPassDesc, ps, features, 1,
                             &inCamera, &modelViewProjection, cubeFace, pEntry);
    }
}

static inline void addDepthTextureBindings(QSSGRhiContext *rhiCtx,
                                           QSSGRhiShaderPipeline *shaderPipeline,
                                           QSSGRhiShaderResourceBindingList &bindings)
{
    if (shaderPipeline->depthTexture()) {
        int binding = shaderPipeline->bindingForTexture("qt_depthTexture", int(QSSGRhiSamplerBindingHints::DepthTexture));
        if (binding >= 0) {
            // nearest min/mag, no mipmap
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
            bindings.addTexture(binding, QRhiShaderResourceBinding::FragmentStage, shaderPipeline->depthTexture(), sampler);
        } // else ignore, not an error
    }

    // SSAO texture
    if (shaderPipeline->ssaoTexture()) {
        int binding = shaderPipeline->bindingForTexture("qt_aoTexture", int(QSSGRhiSamplerBindingHints::AoTexture));
        if (binding >= 0) {
            // linear min/mag, no mipmap
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
            bindings.addTexture(binding, QRhiShaderResourceBinding::FragmentStage, shaderPipeline->ssaoTexture(), sampler);
        } // else ignore, not an error
    }
}

static void rhiPrepareResourcesForShadowMap(QSSGRhiContext *rhiCtx,
                                            const QSSGLayerRenderData &inData,
                                            QSSGPassKey passKey,
                                            QSSGShadowMapEntry *pEntry,
                                            QSSGRhiGraphicsPipelineState *ps,
                                            const QVector2D *depthAdjust,
                                            const QSSGRenderableObjectList &sortedOpaqueObjects,
                                            QSSGRenderCamera &inCamera,
                                            bool orthographic,
                                            QSSGRenderTextureCubeFace cubeFace)
{
    QSSGShaderFeatures featureSet;
    if (orthographic)
        featureSet.set(QSSGShaderFeatures::Feature::OrthoShadowPass, true);
    else
        featureSet.set(QSSGShaderFeatures::Feature::CubeShadowPass, true);

    const auto cubeFaceIdx = QSSGBaseTypeHelpers::indexOfCubeFace(cubeFace);
    const auto &defaultMaterialShaderKeyProperties = inData.getDefaultMaterialPropertyTable();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        QSSG_ASSERT(theObject->renderableFlags.castsShadows(), continue);

        QSSGShaderFeatures objectFeatureSet = featureSet;
        const bool isOpaqueDepthPrePass = theObject->depthWriteMode == QSSGDepthDrawMode::OpaquePrePass;
        if (isOpaqueDepthPrePass)
            objectFeatureSet.set(QSSGShaderFeatures::Feature::OpaqueDepthPrePass, true);

        QSSGRhiDrawCallData *dcd = nullptr;
        QMatrix4x4 modelViewProjection;
        QSSGSubsetRenderable &renderable(static_cast<QSSGSubsetRenderable &>(*theObject));
        if (theObject->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || theObject->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
            const bool hasSkinning = defaultMaterialShaderKeyProperties.m_boneCount.getValue(renderable.shaderDescription) > 0;
            modelViewProjection = hasSkinning ? pEntry->m_lightVP
                                              : pEntry->m_lightVP * renderable.globalTransform;
            // cubeFaceIdx is 0 for directional and 0..5 for pointlight & spotlight
            // pEntry is unique per light and a light can only be one of directional, point, or spotlight.
            const quintptr entryIdx = cubeFaceIdx + (quintptr(renderable.subset.offset) << 3);
            dcd = &rhiCtxD->drawCallData({ passKey, &renderable.modelContext.model, pEntry, entryIdx });
        }

        QSSGRhiShaderResourceBindingList bindings;
        QSSGRhiShaderPipelinePtr shaderPipeline;
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*theObject));
        if (theObject->type == QSSGSubsetRenderable::Type::DefaultMaterialMeshSubset) {
            const auto &material = static_cast<const QSSGRenderDefaultMaterial &>(subsetRenderable.getMaterial());
            ps->cullMode = QSSGRhiHelpers::toCullMode(material.cullMode);
            const bool blendParticles = defaultMaterialShaderKeyProperties.m_blendParticles.getValue(subsetRenderable.shaderDescription);

            shaderPipeline = shadersForDefaultMaterial(ps, subsetRenderable, objectFeatureSet);
            if (!shaderPipeline)
                continue;
            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
            char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            updateUniformsForDefaultMaterial(*shaderPipeline, rhiCtx, inData, ubufData, ps, subsetRenderable, inCamera, depthAdjust, &modelViewProjection);
            if (blendParticles)
                QSSGParticleRenderer::updateUniformsForParticleModel(*shaderPipeline, ubufData, &subsetRenderable.modelContext.model, subsetRenderable.subset.offset);
            dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
            if (blendParticles)
                QSSGParticleRenderer::prepareParticlesForModel(*shaderPipeline, rhiCtx, bindings, &subsetRenderable.modelContext.model);
        } else if (theObject->type == QSSGSubsetRenderable::Type::CustomMaterialMeshSubset) {
            const auto &material = static_cast<const QSSGRenderCustomMaterial &>(subsetRenderable.getMaterial());
            ps->cullMode = QSSGRhiHelpers::toCullMode(material.m_cullMode);

            QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.renderer->contextInterface()->customMaterialSystem().get());
            shaderPipeline = customMaterialSystem.shadersForCustomMaterial(ps, material, subsetRenderable, inData.getDefaultMaterialPropertyTable(), objectFeatureSet);
            if (!shaderPipeline)
                continue;
            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
            char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            // inCamera is the shadow camera, not the same as inData.camera
            customMaterialSystem.updateUniformsForCustomMaterial(*shaderPipeline, rhiCtx, inData, ubufData, ps, material, subsetRenderable,
                                                                 inCamera, depthAdjust, &modelViewProjection);
            dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
        }

        if (theObject->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || theObject->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {

            QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(*ps, shaderPipeline.get());
            auto &ia = QSSGRhiInputAssemblerStatePrivate::get(*ps);
            ia = subsetRenderable.subset.rhi.ia;
            int instanceBufferBinding = setupInstancing(&subsetRenderable, ps, rhiCtx, inData.cameraData->direction, inData.cameraData->position);
            QSSGRhiHelpers::bakeVertexInputLocations(&ia, *shaderPipeline, instanceBufferBinding);


            bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd->ubuf);

                 // Depth and SSAO textures, in case a custom material's shader code does something with them.
            addDepthTextureBindings(rhiCtx, shaderPipeline.get(), bindings);

            if (isOpaqueDepthPrePass) {
                addOpaqueDepthPrePassBindings(rhiCtx,
                                              shaderPipeline.get(),
                                              subsetRenderable.firstImage,
                                              bindings,
                                              (theObject->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset));
            }

            // There is no screen texture at this stage. But the shader from a
            // custom material may rely on it, and an object with that material
            // can end up in the shadow map's object list. So bind a dummy
            // texture then due to the lack of other options.
            int binding = shaderPipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
            if (binding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
                QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
                rhiCtx->commandBuffer()->resourceUpdate(resourceUpdates);
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    dummyTexture, sampler);
            }

            QRhiShaderResourceBindings *srb = rhiCtxD->srb(bindings);
            subsetRenderable.rhiRenderData.shadowPass.pipeline = rhiCtxD->pipeline(*ps,
                                                                                   pEntry->m_rhiRenderPassDesc,
                                                                                   srb);
            subsetRenderable.rhiRenderData.shadowPass.srb[cubeFaceIdx] = srb;
        }
    }
}

static void fillTargetBlend(QRhiGraphicsPipeline::TargetBlend *targetBlend, QSSGRenderDefaultMaterial::MaterialBlendMode materialBlend)
{
    // Assuming default values in the other TargetBlend fields
    switch (materialBlend) {
    case QSSGRenderDefaultMaterial::MaterialBlendMode::Screen:
        targetBlend->srcColor = QRhiGraphicsPipeline::SrcAlpha;
        targetBlend->dstColor = QRhiGraphicsPipeline::One;
        targetBlend->srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend->dstAlpha = QRhiGraphicsPipeline::One;
        break;
    case QSSGRenderDefaultMaterial::MaterialBlendMode::Multiply:
        targetBlend->srcColor = QRhiGraphicsPipeline::DstColor;
        targetBlend->dstColor = QRhiGraphicsPipeline::Zero;
        targetBlend->srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend->dstAlpha = QRhiGraphicsPipeline::One;
        break;
    default:
        // Use SourceOver for everything else
        targetBlend->srcColor = QRhiGraphicsPipeline::SrcAlpha;
        targetBlend->dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        targetBlend->srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend->dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        break;
    }
}

void RenderHelpers::rhiPrepareRenderable(QSSGRhiContext *rhiCtx,
                                         QSSGPassKey passKey,
                                         const QSSGLayerRenderData &inData,
                                         QSSGRenderableObject &inObject,
                                         QRhiRenderPassDescriptor *renderPassDescriptor,
                                         QSSGRhiGraphicsPipelineState *ps,
                                         QSSGShaderFeatures featureSet,
                                         int samples,
                                         QSSGRenderCamera *inCamera,
                                         QMatrix4x4 *alteredModelViewProjection,
                                         QSSGRenderTextureCubeFace cubeFace,
                                         QSSGReflectionMapEntry *entry)
{
    QSSGRenderCamera *camera = inData.camera;
    if (inCamera)
        camera = inCamera;

    const auto &defaultMaterialShaderKeyProperties = inData.getDefaultMaterialPropertyTable();

    switch (inObject.type) {
    case QSSGRenderableObject::Type::DefaultMaterialMeshSubset:
    {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(inObject));

        if ((cubeFace == QSSGRenderTextureCubeFaceNone) && subsetRenderable.reflectionProbeIndex >= 0 && subsetRenderable.renderableFlags.testFlag(QSSGRenderableObjectFlag::ReceivesReflections))
            featureSet.set(QSSGShaderFeatures::Feature::ReflectionProbe, true);

        if ((cubeFace != QSSGRenderTextureCubeFaceNone)) {
            // Disable tonemapping for the reflection pass
            featureSet.disableTonemapping();
        }

        if (subsetRenderable.renderableFlags.rendersWithLightmap())
            featureSet.set(QSSGShaderFeatures::Feature::Lightmap, true);

        const auto &shaderPipeline = shadersForDefaultMaterial(ps, subsetRenderable, featureSet);
        if (shaderPipeline) {
            // Unlike the subsetRenderable (which is allocated per frame so is
            // not persistent in any way), the model reference is persistent in
            // the sense that it references the model node in the scene graph.
            // Combined with the layer node (multiple View3Ds may share the
            // same scene!), this is suitable as a key to get the uniform
            // buffers that were used with the rendering of the same model in
            // the previous frame.
            QSSGRhiShaderResourceBindingList bindings;
            const auto &modelNode = subsetRenderable.modelContext.model;
            const bool blendParticles = defaultMaterialShaderKeyProperties.m_blendParticles.getValue(subsetRenderable.shaderDescription);


            // NOTE:
            // - entryIdx should 0 for QSSGRenderTextureCubeFaceNone.
            // In all other cases the entryIdx is a combination of the cubeface idx and the subset offset, where the lower bits
            // are the cubeface idx.
            const auto cubeFaceIdx = QSSGBaseTypeHelpers::indexOfCubeFace(cubeFace);
            const quintptr entryIdx = quintptr(cubeFace != QSSGRenderTextureCubeFaceNone) * (cubeFaceIdx + (quintptr(subsetRenderable.subset.offset) << 3));
            // If there's an entry we merge that with the address of the material
            const auto entryPartA = reinterpret_cast<quintptr>(&subsetRenderable.material);
            const auto entryPartB = reinterpret_cast<quintptr>(entry);
            const void *entryId = reinterpret_cast<const void *>(entryPartA ^ entryPartB);

            QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
            QSSGRhiDrawCallData &dcd = rhiCtxD->drawCallData({ passKey, &modelNode, entryId, entryIdx });

            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd.ubuf);
            char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            updateUniformsForDefaultMaterial(*shaderPipeline, rhiCtx, inData, ubufData, ps, subsetRenderable, *camera, nullptr, alteredModelViewProjection);
            if (blendParticles)
                QSSGParticleRenderer::updateUniformsForParticleModel(*shaderPipeline, ubufData, &subsetRenderable.modelContext.model, subsetRenderable.subset.offset);
            dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

            if (blendParticles)
                QSSGParticleRenderer::prepareParticlesForModel(*shaderPipeline, rhiCtx, bindings, &subsetRenderable.modelContext.model);

            // Skinning
            if (QRhiTexture *boneTexture = inData.getBonemapTexture(subsetRenderable.modelContext)) {
                int binding = shaderPipeline->bindingForTexture("qt_boneTexture");
                if (binding >= 0) {
                    QRhiSampler *boneSampler = rhiCtx->sampler({ QRhiSampler::Nearest,
                            QRhiSampler::Nearest,
                            QRhiSampler::None,
                            QRhiSampler::ClampToEdge,
                            QRhiSampler::ClampToEdge,
                            QRhiSampler::Repeat
                    });
                    bindings.addTexture(binding,
                                        QRhiShaderResourceBinding::VertexStage,
                                        boneTexture,
                                        boneSampler);
                }
            }
            // Morphing
            auto *targetsTexture = subsetRenderable.subset.rhi.targetsTexture;
            if (targetsTexture) {
                int binding = shaderPipeline->bindingForTexture("qt_morphTargetTexture");
                if (binding >= 0) {
                    QRhiSampler *targetsSampler = rhiCtx->sampler({ QRhiSampler::Nearest,
                                                                    QRhiSampler::Nearest,
                                                                    QRhiSampler::None,
                                                                    QRhiSampler::ClampToEdge,
                                                                    QRhiSampler::ClampToEdge,
                                                                    QRhiSampler::ClampToEdge
                                                               });
                    bindings.addTexture(binding, QRhiShaderResourceBinding::VertexStage, subsetRenderable.subset.rhi.targetsTexture, targetsSampler);
                }
            }

            ps->samples = samples;

            const auto &material = static_cast<const QSSGRenderDefaultMaterial &>(subsetRenderable.getMaterial());
            ps->cullMode = QSSGRhiHelpers::toCullMode(material.cullMode);
            fillTargetBlend(&ps->targetBlend, material.blendMode);

            auto &ia = QSSGRhiInputAssemblerStatePrivate::get(*ps);

            ia = subsetRenderable.subset.rhi.ia;
            QVector3D cameraDirection = inData.cameraData->direction;
            if (inCamera)
                cameraDirection = inCamera->getScalingCorrectDirection();
            QVector3D cameraPosition = inData.cameraData->position;
            if (inCamera)
                cameraPosition = inCamera->getGlobalPos();
            int instanceBufferBinding = setupInstancing(&subsetRenderable, ps, rhiCtx, cameraDirection, cameraPosition);
            QSSGRhiHelpers::bakeVertexInputLocations(&ia, *shaderPipeline, instanceBufferBinding);

            bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd.ubuf, 0, shaderPipeline->ub0Size());

            if (shaderPipeline->isLightingEnabled()) {
                bindings.addUniformBuffer(1, RENDERER_VISIBILITY_ALL, dcd.ubuf,
                                          shaderPipeline->ub0LightDataOffset(),
                                          shaderPipeline->ub0LightDataSize());
            }

            // Texture maps
            QSSGRenderableImage *renderableImage = subsetRenderable.firstImage;
            while (renderableImage) {
                const char *samplerName = QSSGMaterialShaderGenerator::getSamplerName(renderableImage->m_mapType);
                const int samplerHint = int(renderableImage->m_mapType);
                int samplerBinding = shaderPipeline->bindingForTexture(samplerName, samplerHint);
                if (samplerBinding >= 0) {
                    QRhiTexture *texture = renderableImage->m_texture.m_texture;
                    if (samplerBinding >= 0 && texture) {
                        const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
                        QSSGRhiSamplerDescription samplerDesc = {
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_minFilterType),
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_magFilterType),
                            mipmapped ? QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_mipFilterType) : QRhiSampler::None,
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_horizontalTilingMode),
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_verticalTilingMode),
                            QSSGRhiHelpers::toRhi(renderableImage->m_imageNode.m_depthTilingMode)
                        };
                        rhiCtx->checkAndAdjustForNPoT(texture, &samplerDesc);
                        QRhiSampler *sampler = rhiCtx->sampler(samplerDesc);
                        bindings.addTexture(samplerBinding, RENDERER_VISIBILITY_ALL, texture, sampler);
                    }
                } // else this is not necessarily an error, e.g. having metalness/roughness maps with metalness disabled
                renderableImage = renderableImage->m_nextImage;
            }

            if (shaderPipeline->isLightingEnabled()) {
                // Shadow map textures
                const int shadowMapCount = shaderPipeline->shadowMapCount();
                for (int i = 0; i < shadowMapCount; ++i) {
                    QSSGRhiShadowMapProperties &shadowMapProperties(shaderPipeline->shadowMapAt(i));
                    QRhiTexture *texture = shadowMapProperties.shadowMapTexture;
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                    Q_ASSERT(texture && sampler);
                    const QByteArray &name(shadowMapProperties.shadowMapTextureUniformName);
                    if (shadowMapProperties.cachedBinding < 0)
                        shadowMapProperties.cachedBinding = shaderPipeline->bindingForTexture(name);
                    if (shadowMapProperties.cachedBinding < 0) {
                        qWarning("No combined image sampler for shadow map texture '%s'", name.data());
                        continue;
                    }
                    bindings.addTexture(shadowMapProperties.cachedBinding, QRhiShaderResourceBinding::FragmentStage,
                                        texture, sampler);
                }

                 // Prioritize reflection texture over Light Probe texture because
                 // reflection texture also contains the irradiance and pre filtered
                 // values for the light probe.
                if (featureSet.isSet(QSSGShaderFeatures::Feature::ReflectionProbe)) {
                    int reflectionSampler = shaderPipeline->bindingForTexture("qt_reflectionMap");
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                    QRhiTexture* reflectionTexture = inData.getReflectionMapManager()->reflectionMapEntry(subsetRenderable.reflectionProbeIndex)->m_rhiPrefilteredCube;
                    if (reflectionSampler >= 0 && reflectionTexture)
                        bindings.addTexture(reflectionSampler, QRhiShaderResourceBinding::FragmentStage, reflectionTexture, sampler);
                } else if (shaderPipeline->lightProbeTexture()) {
                    int binding = shaderPipeline->bindingForTexture("qt_lightProbe", int(QSSGRhiSamplerBindingHints::LightProbe));
                    if (binding >= 0) {
                        auto tiling = shaderPipeline->lightProbeTiling();
                        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, // enables mipmapping
                                                                 QSSGRhiHelpers::toRhi(tiling.first), QSSGRhiHelpers::toRhi(tiling.second), QRhiSampler::Repeat });
                        bindings.addTexture(binding, QRhiShaderResourceBinding::FragmentStage,
                                            shaderPipeline->lightProbeTexture(), sampler);
                    } else {
                        qWarning("Could not find sampler for lightprobe");
                    }
                }

                // Screen Texture
                if (shaderPipeline->screenTexture()) {
                    int binding = shaderPipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
                    if (binding >= 0) {
                        // linear min/mag, mipmap filtering depends on the
                        // texture, with SCREEN_TEXTURE there are no mipmaps, but
                        // once SCREEN_MIP_TEXTURE is seen the texture (the same
                        // one) has mipmaps generated.
                        QRhiSampler::Filter mipFilter = shaderPipeline->screenTexture()->flags().testFlag(QRhiTexture::MipMapped)
                                ? QRhiSampler::Linear : QRhiSampler::None;
                        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, mipFilter,
                                                                 QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                        bindings.addTexture(binding,
                                            QRhiShaderResourceBinding::FragmentStage,
                                            shaderPipeline->screenTexture(), sampler);
                    } // else ignore, not an error
                }

                if (shaderPipeline->lightmapTexture()) {
                    int binding = shaderPipeline->bindingForTexture("qt_lightmap", int(QSSGRhiSamplerBindingHints::LightmapTexture));
                    if (binding >= 0) {
                        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                                 QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                        bindings.addTexture(binding,
                                            QRhiShaderResourceBinding::FragmentStage,
                                            shaderPipeline->lightmapTexture(), sampler);
                    } // else ignore, not an error
                }
            }

            // Depth and SSAO textures
            addDepthTextureBindings(rhiCtx, shaderPipeline.get(), bindings);

            // Instead of always doing a QHash find in srb(), store the binding
            // list and the srb object in the per-model+material
            // QSSGRhiUniformBufferSet. While this still needs comparing the
            // binding list, to see if something has changed, it results in
            // significant gains with lots of models in the scene (because the
            // srb hash table becomes large then, so avoiding the lookup as
            // much as possible is helpful)
            QRhiShaderResourceBindings *&srb = dcd.srb;
            bool srbChanged = false;
            if (!srb || bindings != dcd.bindings) {
                srb = rhiCtxD->srb(bindings);
                rhiCtxD->releaseCachedSrb(dcd.bindings);
                dcd.bindings = bindings;
                srbChanged = true;
            }

            if (cubeFace != QSSGRenderTextureCubeFaceNone)
                subsetRenderable.rhiRenderData.reflectionPass.srb[cubeFaceIdx] = srb;
            else
                subsetRenderable.rhiRenderData.mainPass.srb = srb;

            const auto pipelineKey = QSSGGraphicsPipelineStateKey::create(*ps, renderPassDescriptor, srb);
            if (dcd.pipeline
                && !srbChanged
                && dcd.renderTargetDescriptionHash == pipelineKey.extra.renderTargetDescriptionHash // we have the hash code anyway, use it to early out upon mismatch
                && dcd.renderTargetDescription == pipelineKey.renderTargetDescription
                && dcd.ps == *ps)
            {
                if (cubeFace != QSSGRenderTextureCubeFaceNone)
                    subsetRenderable.rhiRenderData.reflectionPass.pipeline = dcd.pipeline;
                else
                    subsetRenderable.rhiRenderData.mainPass.pipeline = dcd.pipeline;
            } else {
                if (cubeFace != QSSGRenderTextureCubeFaceNone) {
                    subsetRenderable.rhiRenderData.reflectionPass.pipeline = rhiCtxD->pipeline(pipelineKey,
                                                                                               renderPassDescriptor,
                                                                                               srb);
                    dcd.pipeline = subsetRenderable.rhiRenderData.reflectionPass.pipeline;
                } else {
                    subsetRenderable.rhiRenderData.mainPass.pipeline = rhiCtxD->pipeline(pipelineKey,
                                                                                         renderPassDescriptor,
                                                                                         srb);
                    dcd.pipeline = subsetRenderable.rhiRenderData.mainPass.pipeline;
                }
                dcd.renderTargetDescriptionHash = pipelineKey.extra.renderTargetDescriptionHash;
                dcd.renderTargetDescription = pipelineKey.renderTargetDescription;
                dcd.ps = *ps;
            }
        }
        break;
    }
    case QSSGRenderableObject::Type::CustomMaterialMeshSubset:
    {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(inObject));
        const QSSGRenderCustomMaterial &material = static_cast<const QSSGRenderCustomMaterial &>(subsetRenderable.getMaterial());
        QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.renderer->contextInterface()->customMaterialSystem().get());

        featureSet.set(QSSGShaderFeatures::Feature::LightProbe, inData.layer.lightProbe || material.m_iblProbe);

        if ((cubeFace == QSSGRenderTextureCubeFaceNone) && subsetRenderable.reflectionProbeIndex >= 0 && subsetRenderable.renderableFlags.testFlag(QSSGRenderableObjectFlag::ReceivesReflections))
            featureSet.set(QSSGShaderFeatures::Feature::ReflectionProbe, true);

        if (cubeFace != QSSGRenderTextureCubeFaceNone) {
            // Disable tonemapping for the reflection pass
            featureSet.disableTonemapping();
        }

        if (subsetRenderable.renderableFlags.rendersWithLightmap())
            featureSet.set(QSSGShaderFeatures::Feature::Lightmap, true);

        customMaterialSystem.rhiPrepareRenderable(ps, passKey, subsetRenderable, featureSet,
                                                  material, inData, renderPassDescriptor, samples,
                                                  inCamera, cubeFace, alteredModelViewProjection, entry);
        break;
    }
    case QSSGRenderableObject::Type::Particles:
    {
        QSSGParticlesRenderable &particleRenderable(static_cast<QSSGParticlesRenderable &>(inObject));
        const auto &shaderPipeline = shadersForParticleMaterial(ps, particleRenderable);
        if (shaderPipeline) {
            QSSGParticleRenderer::rhiPrepareRenderable(*shaderPipeline, passKey, rhiCtx, ps, particleRenderable, inData, renderPassDescriptor, samples,
                                                       inCamera, cubeFace, entry);
        }
        break;
    }
    }
}

void RenderHelpers::rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                        const QSSGRhiGraphicsPipelineState &state,
                                        QSSGRenderableObject &object,
                                        bool *needsSetViewport,
                                        QSSGRenderTextureCubeFace cubeFace)
{
    switch (object.type) {
    case QSSGRenderableObject::Type::DefaultMaterialMeshSubset:
    {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(object));

        QRhiGraphicsPipeline *ps = subsetRenderable.rhiRenderData.mainPass.pipeline;
        QRhiShaderResourceBindings *srb = subsetRenderable.rhiRenderData.mainPass.srb;

        if (cubeFace != QSSGRenderTextureCubeFaceNone) {
            const auto cubeFaceIdx = QSSGBaseTypeHelpers::indexOfCubeFace(cubeFace);
            ps = subsetRenderable.rhiRenderData.reflectionPass.pipeline;
            srb = subsetRenderable.rhiRenderData.reflectionPass.srb[cubeFaceIdx];
        }

        if (!ps || !srb)
            return;

        QRhiBuffer *vertexBuffer = subsetRenderable.subset.rhi.vertexBuffer->buffer();
        QRhiBuffer *indexBuffer = subsetRenderable.subset.rhi.indexBuffer ? subsetRenderable.subset.rhi.indexBuffer->buffer() : nullptr;

        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        // QRhi optimizes out unnecessary binding of the same pipline
        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(srb);

        if (*needsSetViewport) {
            cb->setViewport(state.viewport);
            if (state.flags.testFlag(QSSGRhiGraphicsPipelineState::Flag::UsesScissor))
                cb->setScissor(state.scissor);
            *needsSetViewport = false;
        }

        QRhiCommandBuffer::VertexInput vertexBuffers[2];
        int vertexBufferCount = 1;
        vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
        quint32 instances = 1;
        if ( subsetRenderable.modelContext.model.instancing()) {
            instances = subsetRenderable.modelContext.model.instanceCount();
            // If the instance count is 0, the bail out before trying to do any
            // draw calls. Making an instanced draw call with a count of 0 is invalid
            // for Metal and likely other API's as well.
            // It is possible that the particale system may produce 0 instances here
            if (instances == 0)
                return;
            vertexBuffers[1] = QRhiCommandBuffer::VertexInput(subsetRenderable.instanceBuffer, 0);
            vertexBufferCount = 2;
        }
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderCall);
        if (state.flags.testFlag(QSSGRhiGraphicsPipelineState::Flag::UsesStencilRef))
            cb->setStencilRef(state.stencilRef);
        if (indexBuffer) {
            cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, subsetRenderable.subset.rhi.indexBuffer->indexFormat());
            cb->drawIndexed(subsetRenderable.subset.lodCount(subsetRenderable.subsetLevelOfDetail), instances, subsetRenderable.subset.lodOffset(subsetRenderable.subsetLevelOfDetail));
            QSSGRHICTX_STAT(rhiCtx, drawIndexed(subsetRenderable.subset.lodCount(subsetRenderable.subsetLevelOfDetail), instances));
        } else {
            cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
            cb->draw(subsetRenderable.subset.count, instances, subsetRenderable.subset.offset);
            QSSGRHICTX_STAT(rhiCtx, draw(subsetRenderable.subset.count, instances));
        }
        Q_QUICK3D_PROFILE_END_WITH_IDS(QQuick3DProfiler::Quick3DRenderCall, (subsetRenderable.subset.count | quint64(instances) << 32),
                                         QVector<int>({subsetRenderable.modelContext.model.profilingId,
                                          subsetRenderable.material.profilingId}));
        break;
    }
    case QSSGRenderableObject::Type::CustomMaterialMeshSubset:
    {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(object));
        QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.renderer->contextInterface()->customMaterialSystem().get());
        customMaterialSystem.rhiRenderRenderable(rhiCtx, subsetRenderable, needsSetViewport, cubeFace, state);
        break;
    }
    case QSSGRenderableObject::Type::Particles:
    {
        QSSGParticlesRenderable &renderable(static_cast<QSSGParticlesRenderable &>(object));
        QSSGParticleRenderer::rhiRenderRenderable(rhiCtx, renderable, needsSetViewport, cubeFace, state);
        break;
    }
    }
}

void RenderHelpers::rhiRenderShadowMap(QSSGRhiContext *rhiCtx,
                                       QSSGPassKey passKey,
                                       QSSGRhiGraphicsPipelineState &ps,
                                       QSSGRenderShadowMap &shadowMapManager,
                                       const QSSGRenderCamera &camera,
                                       const QSSGShaderLightList &globalLights,
                                       const QSSGRenderableObjectList &sortedOpaqueObjects,
                                       QSSGRenderer &renderer,
                                       const QSSGBoxPoints &castingObjectsBox,
                                       const QSSGBoxPoints &receivingObjectsBox)
{
    const QSSGLayerRenderData &layerData = *QSSGLayerRenderData::getCurrent(renderer);

    static const auto rhiRenderOneShadowMap = [](QSSGRhiContext *rhiCtx,
                                                 QSSGRhiGraphicsPipelineState *ps,
                                                 const QSSGRenderableObjectList &sortedOpaqueObjects,
                                                 int cubeFace) {
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        bool needsSetViewport = true;

        for (const auto &handle : sortedOpaqueObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            QSSG_ASSERT(theObject->renderableFlags.castsShadows(), continue);
            if (theObject->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || theObject->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
                QSSGSubsetRenderable *renderable(static_cast<QSSGSubsetRenderable *>(theObject));

                QRhiBuffer *vertexBuffer = renderable->subset.rhi.vertexBuffer->buffer();
                QRhiBuffer *indexBuffer = renderable->subset.rhi.indexBuffer
                        ? renderable->subset.rhi.indexBuffer->buffer()
                        : nullptr;

                     // Ideally we shouldn't need to deal with this, as only "valid" objects should be processed at this point.
                if (!renderable->rhiRenderData.shadowPass.pipeline)
                    continue;

                Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderCall);

                cb->setGraphicsPipeline(renderable->rhiRenderData.shadowPass.pipeline);

                QRhiShaderResourceBindings *srb = renderable->rhiRenderData.shadowPass.srb[cubeFace];
                cb->setShaderResources(srb);

                if (needsSetViewport) {
                    cb->setViewport(ps->viewport);
                    needsSetViewport = false;
                }

                QRhiCommandBuffer::VertexInput vertexBuffers[2];
                int vertexBufferCount = 1;
                vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
                quint32 instances = 1;
                if (renderable->modelContext.model.instancing()) {
                    instances = renderable->modelContext.model.instanceCount();
                    vertexBuffers[1] = QRhiCommandBuffer::VertexInput(renderable->instanceBuffer, 0);
                    vertexBufferCount = 2;
                }
                if (indexBuffer) {
                    cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, renderable->subset.rhi.indexBuffer->indexFormat());
                    cb->drawIndexed(renderable->subset.count, instances, renderable->subset.offset);
                    QSSGRHICTX_STAT(rhiCtx, drawIndexed(renderable->subset.count, instances));
                } else {
                    cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
                    cb->draw(renderable->subset.count, instances, renderable->subset.offset);
                    QSSGRHICTX_STAT(rhiCtx, draw(renderable->subset.count, instances));
                }
                Q_QUICK3D_PROFILE_END_WITH_IDS(QQuick3DProfiler::Quick3DRenderCall, (renderable->subset.count | quint64(instances) << 32),
                                                 QVector<int>({renderable->modelContext.model.profilingId,
                                                  renderable->material.profilingId}));
            }
        }
    };

    static const auto rhiBlurShadowMap = [](QSSGRhiContext *rhiCtx,
                                            QSSGShadowMapEntry *pEntry,
                                            QSSGRenderer &renderer,
                                            float shadowFilter,
                                            float shadowMapFar,
                                            bool orthographic) {
        // may not be able to do the blur pass if the number of max color
        // attachments is the gl/vk spec mandated minimum of 4, and we need 6.
        // (applicable only to !orthographic, whereas orthographic always works)
        if (!pEntry->m_rhiBlurRenderTarget0 || !pEntry->m_rhiBlurRenderTarget1)
            return;

        QRhi *rhi = rhiCtx->rhi();
        QSSGRhiGraphicsPipelineState ps;
        QRhiTexture *map = orthographic ? pEntry->m_rhiDepthMap : pEntry->m_rhiDepthCube;
        QRhiTexture *workMap = orthographic ? pEntry->m_rhiDepthCopy : pEntry->m_rhiCubeCopy;
        const QSize size = map->pixelSize();
        ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

        const auto &shaderCache = renderer.contextInterface()->shaderCache();

        const auto &blurXPipeline = orthographic ? shaderCache->getBuiltInRhiShaders().getRhiOrthographicShadowBlurXShader()
                                                  : shaderCache->getBuiltInRhiShaders().getRhiCubemapShadowBlurXShader();
        if (!blurXPipeline)
            return;
        QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, blurXPipeline.get());

        ps.colorAttachmentCount = orthographic ? 1 : 6;

        QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);

        // construct a key that is unique for this frame (we use a dynamic buffer
        // so even if the same key gets used in the next frame, just updating the
        // contents on the same QRhiBuffer is ok due to QRhi's internal double buffering)
        QSSGRhiDrawCallData &dcd = rhiCtxD->drawCallData({ map, nullptr, nullptr, 0 });
        if (!dcd.ubuf) {
            dcd.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 8);
            dcd.ubuf->create();
        }

        // the blur also needs Y reversed in order to get correct results (while
        // the second blur step would end up with the correct orientation without
        // this too, but we need to blur the correct fragments in the second step
        // hence the flip is important)
        QMatrix4x4 flipY;
        // correct for D3D and Metal but not for Vulkan because there the Y is down
        // in NDC so that kind of self-corrects...
        if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
            flipY.data()[5] = -1.0f;
        float cameraProperties[2] = { shadowFilter, shadowMapFar };
        char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        memcpy(ubufData, flipY.constData(), 64);
        memcpy(ubufData + 64, cameraProperties, 8);
        dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                 QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
        Q_ASSERT(sampler);

        QSSGRhiShaderResourceBindingList bindings;
        bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd.ubuf);
        bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, map, sampler);
        QRhiShaderResourceBindings *srb = rhiCtxD->srb(bindings);

        QSSGRhiQuadRenderer::Flags quadFlags;
        if (orthographic) // orthoshadowshadowblurx and y have attr_uv as well
            quadFlags |= QSSGRhiQuadRenderer::UvCoords;
        renderer.rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
        renderer.rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, pEntry->m_rhiBlurRenderTarget0, quadFlags);

        // repeat for blur Y, now depthCopy -> depthMap or cubeCopy -> depthCube

        const auto &blurYPipeline = orthographic ? shaderCache->getBuiltInRhiShaders().getRhiOrthographicShadowBlurYShader()
                                                 : shaderCache->getBuiltInRhiShaders().getRhiCubemapShadowBlurYShader();
        if (!blurYPipeline)
            return;
        QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, blurYPipeline.get());

        bindings.clear();
        bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd.ubuf);
        bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, workMap, sampler);
        srb = rhiCtxD->srb(bindings);

        renderer.rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
        renderer.rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, pEntry->m_rhiBlurRenderTarget1, quadFlags);
    };

    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    // We need to deal with a clip depth range of [0, 1] or
    // [-1, 1], depending on the graphics API underneath.
    QVector2D depthAdjust; // (d + depthAdjust[0]) * depthAdjust[1] = d mapped to [0, 1]
    if (rhi->isClipDepthZeroToOne()) {
        // d is [0, 1] so no need for any mapping
        depthAdjust[0] = 0.0f;
        depthAdjust[1] = 1.0f;
    } else {
        // d is [-1, 1]
        depthAdjust[0] = 1.0f;
        depthAdjust[1] = 0.5f;
    }

    // Create shadow map for each light in the scene
    for (int i = 0, ie = globalLights.size(); i != ie; ++i) {
        if (!globalLights[i].shadows || globalLights[i].light->m_fullyBaked)
            continue;

        QSSGShadowMapEntry *pEntry = shadowMapManager.shadowMapEntry(i);
        if (!pEntry)
            continue;

        Q_ASSERT(pEntry->m_rhiDepthStencil);
        const bool orthographic = pEntry->m_rhiDepthMap && pEntry->m_rhiDepthCopy;
        if (orthographic) {
            const QSize size = pEntry->m_rhiDepthMap->pixelSize();
            ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

            const auto &light = globalLights[i].light;
            const auto cameraType = (light->type == QSSGRenderLight::Type::DirectionalLight) ? QSSGRenderCamera::Type::OrthographicCamera : QSSGRenderCamera::Type::CustomCamera;
            QSSGRenderCamera theCamera(cameraType);
            setupCameraForShadowMap(camera, light, theCamera, castingObjectsBox, receivingObjectsBox);
            theCamera.calculateViewProjectionMatrix(pEntry->m_lightVP);
            pEntry->m_lightView = theCamera.globalTransform.inverted(); // pre-calculate this for the material

            rhiPrepareResourcesForShadowMap(rhiCtx, layerData, passKey, pEntry, &ps, &depthAdjust,
                                            sortedOpaqueObjects, theCamera, true, QSSGRenderTextureCubeFaceNone);

            // Render into the 2D texture pEntry->m_rhiDepthMap, using
            // pEntry->m_rhiDepthStencil as the (throwaway) depth/stencil buffer.
            QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[0];
            cb->beginPass(rt, Qt::white, { 1.0f, 0 }, nullptr, rhiCtx->commonPassFlags());
            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
            QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));
            rhiRenderOneShadowMap(rhiCtx, &ps, sortedOpaqueObjects, 0);
            cb->endPass();
            QSSGRHICTX_STAT(rhiCtx, endRenderPass());
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("shadow_map"));

            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
            rhiBlurShadowMap(rhiCtx, pEntry, renderer, globalLights[i].light->m_shadowFilter, globalLights[i].light->m_shadowMapFar, true);
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("shadow_map_blur"));
        } else {
            Q_ASSERT(pEntry->m_rhiDepthCube && pEntry->m_rhiCubeCopy);
            const QSize size = pEntry->m_rhiDepthCube->pixelSize();
            ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

            QSSGRenderCamera theCameras[6] { QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera} };
            setupCubeShadowCameras(globalLights[i].light, theCameras);
            pEntry->m_lightView = QMatrix4x4();

            const bool swapYFaces = !rhi->isYUpInFramebuffer();
            for (const auto face : QSSGRenderTextureCubeFaces) {
                theCameras[quint8(face)].calculateViewProjectionMatrix(pEntry->m_lightVP);
                pEntry->m_lightCubeView[quint8(face)] = theCameras[quint8(face)].globalTransform.inverted(); // pre-calculate this for the material

                rhiPrepareResourcesForShadowMap(rhiCtx, layerData, passKey, pEntry, &ps, &depthAdjust,
                                                sortedOpaqueObjects, theCameras[quint8(face)], false, face);
            }

            for (const auto face : QSSGRenderTextureCubeFaces) {
                // Render into one face of the cubemap texture pEntry->m_rhiDephCube, using
                // pEntry->m_rhiDepthStencil as the (throwaway) depth/stencil buffer.

                QSSGRenderTextureCubeFace outFace = face;
                // FACE  S  T               GL
                // +x   -z, -y   right
                // -x   +z, -y   left
                // +y   +x, +z   top
                // -y   +x, -z   bottom
                // +z   +x, -y   front
                // -z   -x, -y   back
                // FACE  S  T               D3D
                // +x   -z, +y   right
                // -x   +z, +y   left
                // +y   +x, -z   bottom
                // -y   +x, +z   top
                // +z   +x, +y   front
                // -z   -x, +y   back
                if (swapYFaces) {
                    // +Y and -Y faces get swapped (D3D, Vulkan, Metal).
                    // See shadowMapping.glsllib. This is complemented there by reversing T as well.
                    if (outFace == QSSGRenderTextureCubeFace::PosY)
                        outFace = QSSGRenderTextureCubeFace::NegY;
                    else if (outFace == QSSGRenderTextureCubeFace::NegY)
                        outFace = QSSGRenderTextureCubeFace::PosY;
                }
                QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[quint8(outFace)];
                cb->beginPass(rt, Qt::white, { 1.0f, 0 }, nullptr, rhiCtx->commonPassFlags());
                QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));
                Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
                rhiRenderOneShadowMap(rhiCtx, &ps, sortedOpaqueObjects, quint8(face));
                cb->endPass();
                QSSGRHICTX_STAT(rhiCtx, endRenderPass());
                Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QSSG_RENDERPASS_NAME("shadow_cube", 0, outFace));
            }

            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
            rhiBlurShadowMap(rhiCtx, pEntry, renderer, globalLights[i].light->m_shadowFilter, globalLights[i].light->m_shadowMapFar, false);
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("shadow_cube_blur"));
        }
    }
}

void RenderHelpers::rhiRenderReflectionMap(QSSGRhiContext *rhiCtx,
                                           QSSGPassKey passKey,
                                           const QSSGLayerRenderData &inData,
                                           QSSGRhiGraphicsPipelineState *ps,
                                           QSSGRenderReflectionMap &reflectionMapManager,
                                           const QVector<QSSGRenderReflectionProbe *> &reflectionProbes,
                                           const QSSGRenderableObjectList &reflectionPassObjects,
                                           QSSGRenderer &renderer)
{
    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    const bool renderSkybox = (inData.layer.background == QSSGRenderLayer::Background::SkyBox ||
                               inData.layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap)
            && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch);

    for (int i = 0, ie = reflectionProbes.size(); i != ie; ++i) {
        QSSGReflectionMapEntry *pEntry = reflectionMapManager.reflectionMapEntry(i);
        if (!pEntry)
            continue;

        if (!pEntry->m_needsRender)
            continue;

        if (reflectionProbes[i]->refreshMode == QSSGRenderReflectionProbe::ReflectionRefreshMode::FirstFrame && pEntry->m_rendered)
            continue;

        if (reflectionProbes[i]->texture)
            continue;

        Q_ASSERT(pEntry->m_rhiDepthStencil);
        Q_ASSERT(pEntry->m_rhiCube);

        const QSize size = pEntry->m_rhiCube->pixelSize();
        ps->viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

        QSSGRenderCamera theCameras[6] { QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera} };
        setupCubeReflectionCameras(reflectionProbes[i], theCameras);
        const bool swapYFaces = !rhi->isYUpInFramebuffer();
        for (const auto face : QSSGRenderTextureCubeFaces) {
            const auto cubeFaceIdx = QSSGBaseTypeHelpers::indexOfCubeFace(face);
            theCameras[cubeFaceIdx].calculateViewProjectionMatrix(pEntry->m_viewProjection);

            rhiPrepareResourcesForReflectionMap(rhiCtx, passKey, inData, pEntry, ps,
                                                reflectionPassObjects, theCameras[cubeFaceIdx], renderer, face);
        }
        QRhiRenderPassDescriptor *renderPassDesc = nullptr;
        for (auto face : QSSGRenderTextureCubeFaces) {
            if (pEntry->m_timeSlicing == QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces)
                face = pEntry->m_timeSliceFace;

            QSSGRenderTextureCubeFace outFace = face;
            // Faces are swapped similarly to shadow maps due to differences in backends
            // Prefilter step handles correcting orientation differences in the final render
            if (swapYFaces) {
                if (outFace == QSSGRenderTextureCubeFace::PosY)
                    outFace = QSSGRenderTextureCubeFace::NegY;
                else if (outFace == QSSGRenderTextureCubeFace::NegY)
                    outFace = QSSGRenderTextureCubeFace::PosY;
            }
            QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[quint8(outFace)];
            cb->beginPass(rt, reflectionProbes[i]->clearColor, { 1.0f, 0 }, nullptr, rhiCtx->commonPassFlags());
            QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));
            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

            if (renderSkybox && pEntry->m_skyBoxSrbs[quint8(face)]) {
                const auto &shaderCache = renderer.contextInterface()->shaderCache();
                const bool isSkyBox = inData.layer.background == QSSGRenderLayer::Background::SkyBox;
                const auto &shaderPipeline = isSkyBox ? shaderCache->getBuiltInRhiShaders().getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode::None, inData.layer.skyBoxIsRgbe8)
                                                      : shaderCache->getBuiltInRhiShaders().getRhiSkyBoxCubeShader();
                Q_ASSERT(shaderPipeline);
                QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(*ps, shaderPipeline.get());
                QRhiShaderResourceBindings *srb = pEntry->m_skyBoxSrbs[quint8(face)];
                if (!renderPassDesc)
                    renderPassDesc = rt->newCompatibleRenderPassDescriptor();
                rt->setRenderPassDescriptor(renderPassDesc);
                isSkyBox ? renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx, ps, srb, renderPassDesc, {})
                         : renderer.rhiCubeRenderer()->recordRenderCube(rhiCtx, ps, srb, renderPassDesc, {});
            }

            bool needsSetViewport = true;
            for (const auto &handle : reflectionPassObjects)
                rhiRenderRenderable(rhiCtx, *ps, *handle.obj, &needsSetViewport, face);

            cb->endPass();
            QSSGRHICTX_STAT(rhiCtx, endRenderPass());
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QSSG_RENDERPASS_NAME("reflection_cube", 0, outFace));

            if (pEntry->m_timeSlicing == QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces)
                break;
        }
        if (renderPassDesc)
            renderPassDesc->deleteLater();

        pEntry->renderMips(rhiCtx);

        if (pEntry->m_timeSlicing == QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces)
            pEntry->m_timeSliceFace = QSSGBaseTypeHelpers::next(pEntry->m_timeSliceFace); // Wraps

        if (reflectionProbes[i]->refreshMode == QSSGRenderReflectionProbe::ReflectionRefreshMode::FirstFrame)
            pEntry->m_rendered = true;

        reflectionProbes[i]->hasScheduledUpdate = false;
        pEntry->m_needsRender = false;
    }
}

bool RenderHelpers::rhiPrepareAoTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex)
{
    QRhi *rhi = rhiCtx->rhi();
    bool needsBuild = false;

    if (!renderableTex->texture) {
        // the ambient occlusion texture is always non-msaa, even if multisampling is used in the main pass
        renderableTex->texture = rhiCtx->rhi()->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        needsBuild = true;
    } else if (renderableTex->texture->pixelSize() != size) {
        renderableTex->texture->setPixelSize(size);
        needsBuild = true;
    }

    if (needsBuild) {
        if (!renderableTex->texture->create()) {
            qWarning("Failed to build ambient occlusion texture (size %dx%d)", size.width(), size.height());
            renderableTex->reset();
            return false;
        }
        renderableTex->resetRenderTarget();
        renderableTex->rt = rhi->newTextureRenderTarget({ renderableTex->texture });
        renderableTex->rt->setName(QByteArrayLiteral("Ambient occlusion"));
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->create()) {
            qWarning("Failed to build render target for ambient occlusion texture");
            renderableTex->reset();
            return false;
        }
    }

    return true;
}

void RenderHelpers::rhiRenderAoTexture(QSSGRhiContext *rhiCtx,
                                       QSSGPassKey passKey,
                                       QSSGRenderer &renderer,
                                       QSSGRhiShaderPipeline &shaderPipeline,
                                       QSSGRhiGraphicsPipelineState &ps,
                                       const QSSGAmbientOcclusionSettings &ao,
                                       const QSSGRhiRenderableTexture &rhiAoTexture,
                                       const QSSGRhiRenderableTexture &rhiDepthTexture,
                                       const QSSGRenderCamera &camera)
{
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);

    // no texelFetch in GLSL <= 120 and GLSL ES 100
    if (!rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)) {
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        // just clear and stop there
        cb->beginPass(rhiAoTexture.rt, Qt::white, { 1.0f, 0 });
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rhiAoTexture.rt));
        cb->endPass();
        QSSGRHICTX_STAT(rhiCtx, endRenderPass());
        return;
    }

    QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, &shaderPipeline);

    const float R2 = ao.aoDistance * ao.aoDistance * 0.16f;
    const QSize textureSize = rhiAoTexture.texture->pixelSize();
    const float rw = float(textureSize.width());
    const float rh = float(textureSize.height());
    const float fov = camera.verticalFov(rw / rh);
    const float tanHalfFovY = tanf(0.5f * fov * (rh / rw));
    const float invFocalLenX = tanHalfFovY * (rw / rh);

    const QVector4D aoProps(ao.aoStrength * 0.01f, ao.aoDistance * 0.4f, ao.aoSoftness * 0.02f, ao.aoBias);
    const QVector4D aoProps2(float(ao.aoSamplerate), (ao.aoDither) ? 1.0f : 0.0f, 0.0f, 0.0f);
    const QVector4D aoScreenConst(1.0f / R2, rh / (2.0f * tanHalfFovY), 1.0f / rw, 1.0f / rh);
    const QVector4D uvToEyeConst(2.0f * invFocalLenX, -2.0f * tanHalfFovY, -invFocalLenX, tanHalfFovY);
    const QVector2D cameraProps(camera.clipNear, camera.clipFar);

    //    layout(std140, binding = 0) uniform buf {
    //        vec4 aoProperties;
    //        vec4 aoProperties2;
    //        vec4 aoScreenConst;
    //        vec4 uvToEyeConst;
    //        vec2 cameraProperties;

    const int UBUF_SIZE = 72;
    QSSGRhiDrawCallData &dcd(rhiCtxD->drawCallData({ passKey, nullptr, nullptr, 0 }));
    if (!dcd.ubuf) {
        dcd.ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
        dcd.ubuf->create();
    }

    char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(ubufData, &aoProps, 16);
    memcpy(ubufData + 16, &aoProps2, 16);
    memcpy(ubufData + 32, &aoScreenConst, 16);
    memcpy(ubufData + 48, &uvToEyeConst, 16);
    memcpy(ubufData + 64, &cameraProps, 8);
    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
    QSSGRhiShaderResourceBindingList bindings;
    bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd.ubuf);
    bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, rhiDepthTexture.texture, sampler);
    QRhiShaderResourceBindings *srb = rhiCtxD->srb(bindings);

    renderer.rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    renderer.rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, rhiAoTexture.rt, {});
}

bool RenderHelpers::rhiPrepareScreenTexture(QSSGRhiContext *rhiCtx, const QSize &size, bool wantsMips, QSSGRhiRenderableTexture *renderableTex)
{
    QRhi *rhi = rhiCtx->rhi();
    bool needsBuild = false;
    QRhiTexture::Flags flags = QRhiTexture::RenderTarget;
    if (wantsMips)
        flags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

    if (!renderableTex->texture) {
        // always non-msaa, even if multisampling is used in the main pass
        renderableTex->texture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, flags);
        needsBuild = true;
    } else if (renderableTex->texture->pixelSize() != size) {
        renderableTex->texture->setPixelSize(size);
        needsBuild = true;
    }

    if (!renderableTex->depthStencil) {
        renderableTex->depthStencil = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size);
        needsBuild = true;
    } else if (renderableTex->depthStencil->pixelSize() != size) {
        renderableTex->depthStencil->setPixelSize(size);
        needsBuild = true;
    }

    if (needsBuild) {
        if (!renderableTex->texture->create()) {
            qWarning("Failed to build screen texture (size %dx%d)", size.width(), size.height());
            renderableTex->reset();
            return false;
        }
        if (!renderableTex->depthStencil->create()) {
            qWarning("Failed to build depth-stencil buffer for screen texture (size %dx%d)",
                     size.width(), size.height());
            renderableTex->reset();
            return false;
        }
        renderableTex->resetRenderTarget();
        QRhiTextureRenderTargetDescription desc;
        desc.setColorAttachments({ QRhiColorAttachment(renderableTex->texture) });
        desc.setDepthStencilBuffer(renderableTex->depthStencil);
        renderableTex->rt = rhi->newTextureRenderTarget(desc);
        renderableTex->rt->setName(QByteArrayLiteral("Screen texture"));
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->create()) {
            qWarning("Failed to build render target for screen texture");
            renderableTex->reset();
            return false;
        }
    }

    return true;
}

void RenderHelpers::rhiPrepareGrid(QSSGRhiContext *rhiCtx, QSSGPassKey passKey, QSSGRenderLayer &layer, QSSGRenderCamera &inCamera, QSSGRenderer &renderer)
{
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare grid"));

    QSSGRhiShaderResourceBindingList bindings;

    int uniformBinding = 0;
    const int ubufSize = 64 * 2 * sizeof(float) + 4 * sizeof(float) + 4 * sizeof(quint32); // 2x mat4 + 4x float + 1x bool

    QSSGRhiDrawCallData &dcd(rhiCtxD->drawCallData({ passKey, nullptr, nullptr, 0 })); // Change to Grid?

    QRhi *rhi = rhiCtx->rhi();
    if (!dcd.ubuf) {
        dcd.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
        dcd.ubuf->create();
    }

    // Param
    const float nearF = inCamera.clipNear;
    const float farF = inCamera.clipFar;
    const float scale = layer.gridScale;
    const quint32 gridFlags = layer.gridFlags;

    const float yFactor = rhi->isYUpInNDC() ? 1.0f : -1.0f;

    QMatrix4x4 viewProj(Qt::Uninitialized);
    inCamera.calculateViewProjectionMatrix(viewProj);
    QMatrix4x4 invViewProj = viewProj.inverted();

    char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(ubufData + 64 * 0, viewProj.constData(), 64);
    memcpy(ubufData + 64 * 1, invViewProj.constData(), 64);
    memcpy(ubufData + 64 * 2 + 0, &nearF, 4);
    memcpy(ubufData + 64 * 2 + 4 * 1, &farF, 4);
    memcpy(ubufData + 64 * 2 + 4 * 2, &scale, 4);
    memcpy(ubufData + 64 * 2 + 4 * 3, &yFactor, 4);
    memcpy(ubufData + 64 * 2 + 4 * 4, &gridFlags, 4);
    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    bindings.addUniformBuffer(uniformBinding, RENDERER_VISIBILITY_ALL, dcd.ubuf);

    layer.gridSrb = rhiCtxD->srb(bindings);
    renderer.rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);

    cb->debugMarkEnd();
}

static void rhiPrepareSkyBox_helper(QSSGRhiContext *rhiCtx,
                                    QSSGPassKey passKey,
                                    QSSGRenderLayer &layer,
                                    QSSGRenderCamera &inCamera,
                                    QSSGRenderer &renderer,
                                    QSSGReflectionMapEntry *entry = nullptr,
                                    QSSGRenderTextureCubeFace cubeFace = QSSGRenderTextureCubeFaceNone)
{
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
    const bool cubeMapMode = layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap;
    const QSSGRenderImageTexture lightProbeTexture =
            cubeMapMode ? renderer.contextInterface()->bufferManager()->loadRenderImage(layer.skyBoxCubeMap, QSSGBufferManager::MipModeDisable)
                        : renderer.contextInterface()->bufferManager()->loadRenderImage(layer.lightProbe, QSSGBufferManager::MipModeBsdf);
    const bool hasValidTexture = lightProbeTexture.m_texture != nullptr;
    if (hasValidTexture) {
        if (cubeFace == QSSGRenderTextureCubeFaceNone)
            layer.skyBoxIsRgbe8 = lightProbeTexture.m_flags.isRgbe8();

        QSSGRhiShaderResourceBindingList bindings;

        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear,
                                                 QRhiSampler::Linear,
                                                 cubeMapMode ? QRhiSampler::None : QRhiSampler::Linear, // cube map doesn't have mipmaps
                                                 QRhiSampler::Repeat,
                                                 QRhiSampler::ClampToEdge,
                                                 QRhiSampler::Repeat });
        int samplerBinding = 1; //the shader code is hand-written, so we don't need to look that up
        const int ubufSize = 2 * 4 * 3 * sizeof(float) + 2 * 4 * 4 * sizeof(float) + 2 * sizeof(float); // 2x mat3 + 2x mat4 + 2 floats
        bindings.addTexture(samplerBinding,
                            QRhiShaderResourceBinding::FragmentStage,
                            lightProbeTexture.m_texture, sampler);

        const auto cubeFaceIdx = QSSGBaseTypeHelpers::indexOfCubeFace(cubeFace);
        const quintptr entryIdx = quintptr(cubeFace != QSSGRenderTextureCubeFaceNone) * cubeFaceIdx;
        QSSGRhiDrawCallData &dcd = rhiCtxD->drawCallData({ passKey, nullptr, entry, entryIdx });

        QRhi *rhi = rhiCtx->rhi();
        if (!dcd.ubuf) {
            dcd.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
            dcd.ubuf->create();
        }

        const QMatrix4x4 &inverseProjection = inCamera.projection.inverted();
        const QMatrix4x4 &viewMatrix = inCamera.globalTransform;
        QMatrix4x4 viewProjection(Qt::Uninitialized); // For cube mode
        inCamera.calculateViewProjectionWithoutTranslation(0.1f, 5.0f, viewProjection);

        float adjustY = rhi->isYUpInNDC() ? 1.0f : -1.0f;
        const float exposure = layer.lightProbeSettings.probeExposure;
        // orientation
        const QMatrix3x3 &rotationMatrix(layer.lightProbeSettings.probeOrientation);
        const float blurAmount = layer.skyboxBlurAmount;
        const float maxMipLevel = float(lightProbeTexture.m_mipmapCount - 2);

        const QVector4D skyboxProperties = {
            adjustY,
            exposure,
            blurAmount,
            maxMipLevel
        };

        char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        memcpy(ubufData, viewMatrix.constData(), 44);
        memcpy(ubufData + 48, inverseProjection.constData(), 64);
        memcpy(ubufData + 112, rotationMatrix.constData(), 12);
        memcpy(ubufData + 128, (char *)rotationMatrix.constData() + 12, 12);
        memcpy(ubufData + 144, (char *)rotationMatrix.constData() + 24, 12);
        memcpy(ubufData + 160, &skyboxProperties, 16);
        memcpy(ubufData + 176, viewProjection.constData(), 64); //###
        dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd.ubuf);

        if (cubeFace != QSSGRenderTextureCubeFaceNone) {
            const auto cubeFaceIdx = QSSGBaseTypeHelpers::indexOfCubeFace(cubeFace);
            entry->m_skyBoxSrbs[cubeFaceIdx] = rhiCtxD->srb(bindings);
        } else {
            layer.skyBoxSrb = rhiCtxD->srb(bindings);
        }

        if (cubeMapMode)
            renderer.rhiCubeRenderer()->prepareCube(rhiCtx, nullptr);
        else
            renderer.rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    }
}

void RenderHelpers::rhiPrepareSkyBox(QSSGRhiContext *rhiCtx,
                                     QSSGPassKey passKey,
                                     QSSGRenderLayer &layer,
                                     QSSGRenderCamera &inCamera,
                                     QSSGRenderer &renderer)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare skybox"));

    rhiPrepareSkyBox_helper(rhiCtx, passKey, layer, inCamera, renderer);

    cb->debugMarkEnd();
}

void RenderHelpers::rhiPrepareSkyBoxForReflectionMap(QSSGRhiContext *rhiCtx,
                                                     QSSGPassKey passKey,
                                                     QSSGRenderLayer &layer,
                                                     QSSGRenderCamera &inCamera,
                                                     QSSGRenderer &renderer,
                                                     QSSGReflectionMapEntry *entry,
                                                     QSSGRenderTextureCubeFace cubeFace)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare skybox for reflection cube map"));

    rhiPrepareSkyBox_helper(rhiCtx, passKey, layer, inCamera, renderer, entry, cubeFace);

    cb->debugMarkEnd();
}

bool RenderHelpers::rhiPrepareDepthPass(QSSGRhiContext *rhiCtx,
                                        QSSGPassKey passKey,
                                        const QSSGRhiGraphicsPipelineState &basePipelineState,
                                        QRhiRenderPassDescriptor *rpDesc,
                                        QSSGLayerRenderData &inData,
                                        const QSSGRenderableObjectList &sortedOpaqueObjects,
                                        const QSSGRenderableObjectList &sortedTransparentObjects,
                                        int samples)
{
    static const auto rhiPrepareDepthPassForObject = [](QSSGRhiContext *rhiCtx,
                                                        QSSGPassKey passKey,
                                                        QSSGLayerRenderData &inData,
                                                        QSSGRenderableObject *obj,
                                                        QRhiRenderPassDescriptor *rpDesc,
                                                        QSSGRhiGraphicsPipelineState *ps) {
        QSSGRhiShaderPipelinePtr shaderPipeline;
        QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);

        const bool isOpaqueDepthPrePass = obj->depthWriteMode == QSSGDepthDrawMode::OpaquePrePass;
        QSSGShaderFeatures featureSet;
        featureSet.set(QSSGShaderFeatures::Feature::DepthPass, true);
        if (isOpaqueDepthPrePass)
            featureSet.set(QSSGShaderFeatures::Feature::OpaqueDepthPrePass, true);

        QSSGRhiDrawCallData *dcd = nullptr;
        if (obj->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || obj->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
            QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
            const void *modelNode = &subsetRenderable.modelContext.model;
            dcd = &rhiCtxD->drawCallData({ passKey, modelNode, &subsetRenderable.material, 0 });
        }

        if (obj->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset) {
            QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
            const auto &material = static_cast<const QSSGRenderDefaultMaterial &>(subsetRenderable.getMaterial());
            ps->cullMode = QSSGRhiHelpers::toCullMode(material.cullMode);

            shaderPipeline = shadersForDefaultMaterial(ps, subsetRenderable, featureSet);
            if (shaderPipeline) {
                shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
                char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
                updateUniformsForDefaultMaterial(*shaderPipeline, rhiCtx, inData, ubufData, ps, subsetRenderable, *inData.camera, nullptr, nullptr);
                dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
            } else {
                return false;
            }
        } else if (obj->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
            QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));

            const auto &customMaterial = static_cast<const QSSGRenderCustomMaterial &>(subsetRenderable.getMaterial());

            ps->cullMode = QSSGRhiHelpers::toCullMode(customMaterial.m_cullMode);

            QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.renderer->contextInterface()->customMaterialSystem().get());
            shaderPipeline = customMaterialSystem.shadersForCustomMaterial(ps, customMaterial, subsetRenderable, inData.getDefaultMaterialPropertyTable(), featureSet);

            if (shaderPipeline) {
                shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
                char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
                customMaterialSystem.updateUniformsForCustomMaterial(*shaderPipeline, rhiCtx, inData, ubufData, ps, customMaterial, subsetRenderable,
                                                                     *inData.camera, nullptr, nullptr);
                dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
            } else {
                return false;
            }
        }

        // the rest is common, only relying on QSSGSubsetRenderableBase, not the subclasses
        if (obj->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || obj->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
            QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
            auto &ia = QSSGRhiInputAssemblerStatePrivate::get(*ps);
            ia = subsetRenderable.subset.rhi.ia;

            int instanceBufferBinding = setupInstancing(&subsetRenderable, ps, rhiCtx, inData.cameraData->direction, inData.cameraData->position);
            QSSGRhiHelpers::bakeVertexInputLocations(&ia, *shaderPipeline, instanceBufferBinding);

            QSSGRhiShaderResourceBindingList bindings;
            bindings.addUniformBuffer(0, RENDERER_VISIBILITY_ALL, dcd->ubuf);

            // Depth and SSAO textures, in case a custom material's shader code does something with them.
            addDepthTextureBindings(rhiCtx, shaderPipeline.get(), bindings);

            if (isOpaqueDepthPrePass) {
                addOpaqueDepthPrePassBindings(rhiCtx,
                                              shaderPipeline.get(),
                                              subsetRenderable.firstImage,
                                              bindings,
                                              (obj->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset));
            }

            QRhiShaderResourceBindings *srb = rhiCtxD->srb(bindings);

            subsetRenderable.rhiRenderData.depthPrePass.pipeline = rhiCtxD->pipeline(*ps,
                                                                                     rpDesc,
                                                                                     srb);
            subsetRenderable.rhiRenderData.depthPrePass.srb = srb;
        }

        return true;
    };

    // Phase 1 (prepare) for the Z prepass or the depth texture generation.
    // These renders opaque (Z prepass), or opaque and transparent (depth
    // texture), objects with depth test/write enabled, and color write
    // disabled, using a very simple set of shaders.

    QSSGRhiGraphicsPipelineState ps = basePipelineState; // viewport and others are filled out already
    // We took a copy of the pipeline state since we do not want to conflict
    // with what rhiPrepare() collects for its own use. So here just change
    // whatever we need.

    ps.samples = samples;
    ps.flags |= { QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled, QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled };
    ps.targetBlend.colorWrite = {};

    for (const QSSGRenderableObjectHandle &handle : sortedOpaqueObjects) {
        if (!rhiPrepareDepthPassForObject(rhiCtx, passKey, inData, handle.obj, rpDesc, &ps))
            return false;
    }

    for (const QSSGRenderableObjectHandle &handle : sortedTransparentObjects) {
        if (!rhiPrepareDepthPassForObject(rhiCtx, passKey, inData, handle.obj, rpDesc, &ps))
            return false;
    }

    return true;
}

void RenderHelpers::rhiRenderDepthPass(QSSGRhiContext *rhiCtx,
                                       const QSSGRhiGraphicsPipelineState &pipelineState,
                                       const QSSGRenderableObjectList &sortedOpaqueObjects,
                                       const QSSGRenderableObjectList &sortedTransparentObjects,
                                       bool *needsSetViewport)
{
    static const auto rhiRenderDepthPassForImp = [](QSSGRhiContext *rhiCtx,
                                                    const QSSGRhiGraphicsPipelineState &pipelineState,
                                                    const QSSGRenderableObjectList &objects,
                                                    bool *needsSetViewport) {
        for (const auto &oh : objects) {
            QSSGRenderableObject *obj = oh.obj;

                 // casts to SubsetRenderableBase so it works for both default and custom materials
            if (obj->type == QSSGRenderableObject::Type::DefaultMaterialMeshSubset || obj->type == QSSGRenderableObject::Type::CustomMaterialMeshSubset) {
                QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
                QSSGSubsetRenderable *subsetRenderable(static_cast<QSSGSubsetRenderable *>(obj));

                QRhiBuffer *vertexBuffer = subsetRenderable->subset.rhi.vertexBuffer->buffer();
                QRhiBuffer *indexBuffer = subsetRenderable->subset.rhi.indexBuffer
                        ? subsetRenderable->subset.rhi.indexBuffer->buffer()
                        : nullptr;

                QRhiGraphicsPipeline *ps = subsetRenderable->rhiRenderData.depthPrePass.pipeline;
                if (!ps)
                    return;

                QRhiShaderResourceBindings *srb = subsetRenderable->rhiRenderData.depthPrePass.srb;
                if (!srb)
                    return;

                Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderCall);
                cb->setGraphicsPipeline(ps);
                cb->setShaderResources(srb);

                if (*needsSetViewport) {
                    cb->setViewport(pipelineState.viewport);
                    *needsSetViewport = false;
                }

                QRhiCommandBuffer::VertexInput vertexBuffers[2];
                int vertexBufferCount = 1;
                vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
                quint32 instances = 1;
                if (subsetRenderable->modelContext.model.instancing()) {
                    instances = subsetRenderable->modelContext.model.instanceCount();
                    vertexBuffers[1] = QRhiCommandBuffer::VertexInput(subsetRenderable->instanceBuffer, 0);
                    vertexBufferCount = 2;
                }

                if (indexBuffer) {
                    cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, subsetRenderable->subset.rhi.indexBuffer->indexFormat());
                    cb->drawIndexed(subsetRenderable->subset.count, instances, subsetRenderable->subset.offset);
                    QSSGRHICTX_STAT(rhiCtx, drawIndexed(subsetRenderable->subset.count, instances));
                } else {
                    cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
                    cb->draw(subsetRenderable->subset.count, instances, subsetRenderable->subset.offset);
                    QSSGRHICTX_STAT(rhiCtx, draw(subsetRenderable->subset.count, instances));
                }
                Q_QUICK3D_PROFILE_END_WITH_IDS(QQuick3DProfiler::Quick3DRenderCall, (subsetRenderable->subset.count | quint64(instances) << 32),
                                                 QVector<int>({subsetRenderable->modelContext.model.profilingId,
                                                  subsetRenderable->material.profilingId}));
            }
        }
    };

    rhiRenderDepthPassForImp(rhiCtx, pipelineState, sortedOpaqueObjects, needsSetViewport);
    rhiRenderDepthPassForImp(rhiCtx, pipelineState, sortedTransparentObjects, needsSetViewport);
}

bool RenderHelpers::rhiPrepareDepthTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex)
{
    QRhi *rhi = rhiCtx->rhi();
    bool needsBuild = false;

    if (!renderableTex->texture) {
        QRhiTexture::Format format = QRhiTexture::D32F;
        if (!rhi->isTextureFormatSupported(format))
            format = QRhiTexture::D16;
        if (!rhi->isTextureFormatSupported(format))
            qWarning("Depth texture not supported");
        // the depth texture is always non-msaa, even if multisampling is used in the main pass
        renderableTex->texture = rhiCtx->rhi()->newTexture(format, size, 1, QRhiTexture::RenderTarget);
        needsBuild = true;
    } else if (renderableTex->texture->pixelSize() != size) {
        renderableTex->texture->setPixelSize(size);
        needsBuild = true;
    }

    if (needsBuild) {
        if (!renderableTex->texture->create()) {
            qWarning("Failed to build depth texture (size %dx%d, format %d)",
                     size.width(), size.height(), int(renderableTex->texture->format()));
            renderableTex->reset();
            return false;
        }
        renderableTex->resetRenderTarget();
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setDepthTexture(renderableTex->texture);
        renderableTex->rt = rhi->newTextureRenderTarget(rtDesc);
        renderableTex->rt->setName(QByteArrayLiteral("Depth texture"));
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->create()) {
            qWarning("Failed to build render target for depth texture");
            renderableTex->reset();
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE
