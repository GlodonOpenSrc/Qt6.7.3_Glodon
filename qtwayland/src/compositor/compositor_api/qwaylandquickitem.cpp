// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwaylandquickitem.h"
#include "qwaylandquickitem_p.h"
#include "qwaylandquicksurface.h"
#include "qwaylandinputmethodcontrol.h"
#include "qwaylandtextinput.h"
#include "qwaylandtextinputv3.h"
#include "qwaylandqttextinputmethod.h"
#include "qwaylandquickoutput.h"
#include <QtWaylandCompositor/qwaylandcompositor.h>
#include <QtWaylandCompositor/qwaylandseat.h>
#include <QtWaylandCompositor/qwaylandbufferref.h>
#if QT_CONFIG(draganddrop)
#include <QtWaylandCompositor/QWaylandDrag>
#endif
#include <QtWaylandCompositor/private/qwlclientbufferintegration_p.h>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>

#if QT_CONFIG(opengl)
#  include <QtOpenGL/QOpenGLTexture>
#  include <QtGui/QOpenGLFunctions>
#endif

#include <QtGui/QKeyEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>
#include <QtQuick/qsgtexture.h>

#include <QtCore/QFile>
#include <QtCore/QMutexLocker>
#include <QtCore/QMutex>

#include <wayland-server-core.h>
#include <QThread>

#if QT_CONFIG(opengl)
#include <QtGui/private/qshaderdescription_p.h>
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(opengl)
static const struct {
    const char * const vertexShaderSourceFile;
    const char * const fragmentShaderSourceFile;
    GLenum textureTarget;
    int planeCount;
    bool canProvideTexture;
    QSGMaterial::Flags materialFlags;
    QSGMaterialType materialType;
} bufferTypes[] = {
    // BufferFormatEgl_Null
    { "", "", 0, 0, false, {}, {} },

    // BufferFormatEgl_RGB
    {
        ":/qt-project.org/wayland/compositor/shaders/surface.vert.qsb",
        ":/qt-project.org/wayland/compositor/shaders/surface_rgbx.frag.qsb",
        GL_TEXTURE_2D, 1, true,
        QSGMaterial::Blending,
        {}
    },

    // BufferFormatEgl_RGBA
    {
        ":/qt-project.org/wayland/compositor/shaders/surface.vert.qsb",
        ":/qt-project.org/wayland/compositor/shaders/surface_rgba.frag.qsb",
        GL_TEXTURE_2D, 1, true,
        QSGMaterial::Blending,
        {}
    },

    // BufferFormatEgl_EXTERNAL_OES
    {
        ":/qt-project.org/wayland/compositor/shaders/surface.vert.qsb",
        ":/qt-project.org/wayland/compositor/shaders/surface_oes_external.frag",
        GL_TEXTURE_EXTERNAL_OES, 1, false,
        QSGMaterial::Blending,
        {}
    },

    // BufferFormatEgl_Y_U_V
    {
        ":/qt-project.org/wayland/compositor/shaders/surface.vert.qsb",
        ":/qt-project.org/wayland/compositor/shaders/surface_y_u_v.frag.qsb",
        GL_TEXTURE_2D, 3, false,
        QSGMaterial::Blending,
        {}
    },

    // BufferFormatEgl_Y_UV
    {
        ":/qt-project.org/wayland/compositor/shaders/surface.vert.qsb",
        ":/qt-project.org/wayland/compositor/shaders/surface_y_uv.frag.qsb",
        GL_TEXTURE_2D, 2, false,
        QSGMaterial::Blending,
        {}
    },

    // BufferFormatEgl_Y_XUXV
    {
        ":/qt-project.org/wayland/compositor/shaders/surface.vert.qsb",
        ":/qt-project.org/wayland/compositor/shaders/surface_y_xuxv.frag.qsb",
        GL_TEXTURE_2D, 2, false,
        QSGMaterial::Blending,
        {}
    }
};

QWaylandBufferMaterialShader::QWaylandBufferMaterialShader(QWaylandBufferRef::BufferFormatEgl format)
{
    Q_UNUSED(format);
    setShaderFileName(VertexStage, QString::fromLatin1(bufferTypes[format].vertexShaderSourceFile));
    auto fragmentShaderSourceFile = QString::fromLatin1(bufferTypes[format].fragmentShaderSourceFile);

    if (format == QWaylandBufferRef::BufferFormatEgl_EXTERNAL_OES)
        setupExternalOESShader(fragmentShaderSourceFile);
    else
        setShaderFileName(FragmentStage, fragmentShaderSourceFile);
}

void QWaylandBufferMaterialShader::setupExternalOESShader(const QString &shaderFilename)
{
#if QT_CONFIG(opengl)
    QFile shaderFile(shaderFilename);
    if (!shaderFile.open(QIODevice::ReadOnly)) {
        qCWarning(qLcWaylandCompositor) << "Cannot find external OES shader file:" << shaderFilename;
        return;
    }
    QByteArray FS = shaderFile.readAll();

    static const char *FS_GLES_PREAMBLE =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n";
    static const char *FS_GL_PREAMBLE =
            "#version 120\n"
            "#extension GL_OES_EGL_image_external : require\n";
    QByteArray fsGLES = FS_GLES_PREAMBLE + FS;
    QByteArray fsGL = FS_GL_PREAMBLE + FS;

    QShaderDescription desc;
    QShaderDescriptionPrivate *descData = QShaderDescriptionPrivate::get(&desc);

    QShaderDescription::InOutVariable texCoordInput;
    texCoordInput.name = "v_texcoord";
    texCoordInput.type = QShaderDescription::Vec2;
    texCoordInput.location = 0;

    descData->inVars = { texCoordInput };

    QShaderDescription::InOutVariable fragColorOutput;
    fragColorOutput.name = "gl_FragColor";
    fragColorOutput.type = QShaderDescription::Vec4;
    fragColorOutput.location = 0;

    descData->outVars = { fragColorOutput };

    QShaderDescription::BlockVariable matrixBlockVar;
    matrixBlockVar.name = "qt_Matrix";
    matrixBlockVar.type = QShaderDescription::Mat4;
    matrixBlockVar.offset = 0;
    matrixBlockVar.size = 64;

    QShaderDescription::BlockVariable opacityBlockVar;
    opacityBlockVar.name = "qt_Opacity";
    opacityBlockVar.type = QShaderDescription::Float;
    opacityBlockVar.offset = 64;
    opacityBlockVar.size = 4;

    QShaderDescription::UniformBlock ubufStruct;
    ubufStruct.blockName = "buf";
    ubufStruct.structName = "ubuf";
    ubufStruct.size = 64 + 4;
    ubufStruct.binding = 0;
    ubufStruct.members = { matrixBlockVar, opacityBlockVar };

    descData->uniformBlocks = { ubufStruct };

    QShaderDescription::InOutVariable samplerTex0;
    samplerTex0.name = "tex0";
    samplerTex0.type = QShaderDescription::SamplerExternalOES;
    samplerTex0.binding = 1;

    descData->combinedImageSamplers = { samplerTex0 };

    QShader shaderPack;
    shaderPack.setStage(QShader::FragmentStage);
    shaderPack.setDescription(desc);
    shaderPack.setShader(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs)), QShaderCode(fsGLES));
    shaderPack.setShader(QShaderKey(QShader::GlslShader, QShaderVersion(120)), QShaderCode(fsGL));

    setShader(FragmentStage, shaderPack);
#else
    Q_UNUSED(shaderFilename);
#endif
}

bool QWaylandBufferMaterialShader::updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 68);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }

    return changed;
}

void QWaylandBufferMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                      QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);

    QWaylandBufferMaterial *material = static_cast<QWaylandBufferMaterial *>(newMaterial);
    switch (binding) {
    case 1:
        *texture = material->m_scenegraphTextures.at(0);
        break;
    case 2:
        *texture = material->m_scenegraphTextures.at(1);
        break;
    case 3:
        *texture = material->m_scenegraphTextures.at(2);
        break;
    default:
        return;
    }

    // This is for the shared memory case, and is a no-op for others,
    // this is where the upload from the QImage happens when not yet done.
    // ### or is this too late? (if buffer.image() disappears in the meantime then this is the wrong...)
    if (*texture)
        (*texture)->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
}

QWaylandBufferMaterial::QWaylandBufferMaterial(QWaylandBufferRef::BufferFormatEgl format)
    : m_format(format)
{
    setFlag(bufferTypes[m_format].materialFlags);
}

QWaylandBufferMaterial::~QWaylandBufferMaterial()
{
    qDeleteAll(m_scenegraphTextures);
}

void QWaylandBufferMaterial::setTextureForPlane(int plane,
                                                QOpenGLTexture *texture,
                                                QSGTexture *scenegraphTexture)
{
    if (plane < 0 || plane >= bufferTypes[m_format].planeCount) {
        qWarning("plane index is out of range");
        return;
    }

    texture->bind();
    setTextureParameters(texture->target());

    ensureTextures(plane - 1);

    if (m_textures.size() <= plane) {
        m_textures << texture;
        m_scenegraphTextures << scenegraphTexture;
    } else {
        delete m_scenegraphTextures[plane];

        m_textures[plane] = texture;
        m_scenegraphTextures[plane] = scenegraphTexture;
    }
}

void QWaylandBufferMaterial::bind()
{
    ensureTextures(bufferTypes[m_format].planeCount);

    switch (m_textures.size()) {
    case 3:
        if (m_textures[2])
            m_textures[2]->bind(2);
        Q_FALLTHROUGH();
    case 2:
        if (m_textures[1])
            m_textures[1]->bind(1);
        Q_FALLTHROUGH();
    case 1:
        if (m_textures[0])
            m_textures[0]->bind(0);
    }
}

QSGMaterialType *QWaylandBufferMaterial::type() const
{
    return const_cast<QSGMaterialType *>(&bufferTypes[m_format].materialType);
}

QSGMaterialShader *QWaylandBufferMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QWaylandBufferMaterialShader(m_format);
}


void QWaylandBufferMaterial::setTextureParameters(GLenum target)
{
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    gl->glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

//TODO move this into a separate centralized texture management class
void QWaylandBufferMaterial::ensureTextures(int count)
{
    for (int plane = m_textures.size(); plane < count; plane++) {
        m_textures << nullptr;
        m_scenegraphTextures << nullptr;
    }
}

void QWaylandBufferMaterial::setBufferRef(QWaylandQuickItem *surfaceItem, const QWaylandBufferRef &ref)
{
    m_bufferRef = ref;
    for (int plane = 0; plane < bufferTypes[ref.bufferFormatEgl()].planeCount; plane++) {
        if (auto texture = ref.toOpenGLTexture(plane)) {
            QQuickWindow::CreateTextureOptions opt;
            QWaylandQuickSurface *waylandSurface = qobject_cast<QWaylandQuickSurface *>(surfaceItem->surface());
            if (waylandSurface != nullptr && waylandSurface->useTextureAlpha() && !waylandSurface->isOpaque())
                opt |= QQuickWindow::TextureHasAlphaChannel;
            QSGTexture *scenegraphTexture;
            if (ref.bufferFormatEgl() == QWaylandBufferRef::BufferFormatEgl_EXTERNAL_OES) {
                scenegraphTexture = QNativeInterface::QSGOpenGLTexture::fromNativeExternalOES(texture->textureId(),
                                                                                              surfaceItem->window(),
                                                                                              ref.size(),
                                                                                              opt);
            } else {
                scenegraphTexture = QNativeInterface::QSGOpenGLTexture::fromNative(texture->textureId(),
                                                                                   surfaceItem->window(),
                                                                                   ref.size(),
                                                                                   opt);
            }
            scenegraphTexture->setFiltering(surfaceItem->smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
            setTextureForPlane(plane, texture, scenegraphTexture);
        }
    }

    bind();
}
#endif // QT_CONFIG(opengl)

QMutex *QWaylandQuickItemPrivate::mutex = nullptr;

class QWaylandSurfaceTextureProvider : public QSGTextureProvider
{
public:
    QWaylandSurfaceTextureProvider()
    {
    }

    ~QWaylandSurfaceTextureProvider() override
    {
        delete m_sgTex;
    }

    void setBufferRef(QWaylandQuickItem *surfaceItem, const QWaylandBufferRef &buffer)
    {
        Q_ASSERT(QThread::currentThread() == thread());
        m_ref = buffer;
        delete m_sgTex;
        m_sgTex = nullptr;
        if (m_ref.hasBuffer()) {
            if (buffer.isSharedMemory()) {
                m_sgTex = surfaceItem->window()->createTextureFromImage(buffer.image());
            } else {
#if QT_CONFIG(opengl)
                QQuickWindow::CreateTextureOptions opt;
                QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(surfaceItem->surface());
                if (surface && surface->useTextureAlpha()  && !surface->isOpaque()) {
                    opt |= QQuickWindow::TextureHasAlphaChannel;
                }

                auto texture = buffer.toOpenGLTexture();
                GLuint textureId = texture->textureId();
                auto size = surface->bufferSize();
                m_sgTex = QNativeInterface::QSGOpenGLTexture::fromNative(textureId, surfaceItem->window(), size, opt);
#else
                qCWarning(qLcWaylandCompositor) << "Without OpenGL support only shared memory textures are supported";
#endif
            }
        }
        emit textureChanged();
    }

    QSGTexture *texture() const override
    {
        if (m_sgTex)
            m_sgTex->setFiltering(m_smooth ? QSGTexture::Linear : QSGTexture::Nearest);
        return m_sgTex;
    }

    void setSmooth(bool smooth) { m_smooth = smooth; }
private:
    bool m_smooth = false;
    QSGTexture *m_sgTex = nullptr;
    QWaylandBufferRef m_ref;
};

/*!
 * \qmltype WaylandQuickItem
 * \instantiates QWaylandQuickItem
 * \inqmlmodule QtWayland.Compositor
 * \since 5.8
 * \brief Provides a Qt Quick item that represents a WaylandView.
 *
 * Qt Quick-based Wayland compositors can use this type to display a client's
 * contents on an output device. It passes user input to the
 * client.
 */

/*!
 * \class QWaylandQuickItem
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandQuickItem class provides a Qt Quick item representing a QWaylandView.
 *
 * When writing a QWaylandCompositor in Qt Quick, this class can be used to display a
 * client's contents on an output device and will pass user input to the
 * client.
 */

/*!
 * Constructs a QWaylandQuickItem with the given \a parent.
 */
QWaylandQuickItem::QWaylandQuickItem(QQuickItem *parent)
    : QWaylandQuickItem(*new QWaylandQuickItemPrivate(), parent)
{
}

/*!
 * \internal
 */
QWaylandQuickItem::QWaylandQuickItem(QWaylandQuickItemPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
{
    d_func()->init();
    connect(this, &QQuickItem::activeFocusChanged, this, &QWaylandQuickItem::updateFocus);
}

/*!
 * Destroy the QWaylandQuickItem.
 */
QWaylandQuickItem::~QWaylandQuickItem()
{
    Q_D(QWaylandQuickItem);
    disconnect(this, &QQuickItem::windowChanged, this, &QWaylandQuickItem::updateWindow);
    disconnect(this, &QQuickItem::activeFocusChanged, this, &QWaylandQuickItem::updateFocus);
    QMutexLocker locker(d->mutex);
    if (d->provider) {
        disconnect(d->texProviderConnection);
        d->provider->deleteLater();
    }
}

/*!
 * \qmlproperty WaylandCompositor QtWayland.Compositor::WaylandQuickItem::compositor
 *
 * This property holds the compositor for the surface rendered by this WaylandQuickItem.
 */

/*!
 * \property QWaylandQuickItem::compositor
 *
 * This property holds the compositor for the surface rendered by this QWaylandQuickItem.
 */
QWaylandCompositor *QWaylandQuickItem::compositor() const
{
    Q_D(const QWaylandQuickItem);
    return d->view->surface() ? d->view->surface()->compositor() : nullptr;
}

/*!
 * Returns the view rendered by this QWaylandQuickItem.
 */
QWaylandView *QWaylandQuickItem::view() const
{
    Q_D(const QWaylandQuickItem);
    return d->view.data();
}

/*!
 * \qmlproperty WaylandSurface QtWayland.Compositor::WaylandQuickItem::surface
 *
 * This property holds the surface rendered by this WaylandQuickItem.
 */

/*!
 * \property QWaylandQuickItem::surface
 *
 * This property holds the surface rendered by this QWaylandQuickItem.
 */

QWaylandSurface *QWaylandQuickItem::surface() const
{
    Q_D(const QWaylandQuickItem);
    return d->view->surface();
}

void QWaylandQuickItem::setSurface(QWaylandSurface *surface)
{
    Q_D(QWaylandQuickItem);
    QWaylandSurface *oldSurf = d->view->surface();
    QWaylandCompositor *oldComp = d->view->surface() ? d->view->surface()->compositor() : nullptr;
    d->view->setSurface(surface);
    QWaylandCompositor *newComp = d->view->surface() ? d->view->surface()->compositor() : nullptr;
    if (oldComp != newComp)
        emit compositorChanged();
    if (oldSurf != surface)
        emit surfaceChanged();

    updateFocus();
    update();
}

/*!
 * \qmlproperty enum QtWayland.Compositor::WaylandQuickItem::origin
 *
 * This property holds the origin of the QWaylandQuickItem.
 */

/*!
 * \property QWaylandQuickItem::origin
 *
 * This property holds the origin of the QWaylandQuickItem.
 */
QWaylandSurface::Origin QWaylandQuickItem::origin() const
{
    Q_D(const QWaylandQuickItem);
    return d->origin;
}

bool QWaylandQuickItem::isTextureProvider() const
{
    Q_D(const QWaylandQuickItem);
    return QQuickItem::isTextureProvider() || d->provider;
}

/*!
 * Returns the texture provider of this QWaylandQuickItem.
 */
QSGTextureProvider *QWaylandQuickItem::textureProvider() const
{
    Q_D(const QWaylandQuickItem);

    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    return d->provider;
}

/*!
 * \internal
 */
void QWaylandQuickItem::mousePressEvent(QMouseEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (!d->shouldSendInputEvents()) {
        event->ignore();
        return;
    }

    if (!inputRegionContains(event->position())) {
        event->ignore();
        return;
    }

    QWaylandSeat *seat = compositor()->seatFor(event);

    if (d->focusOnClick)
        takeFocus(seat);

    seat->sendMouseMoveEvent(d->view.data(), mapToSurface(event->position()), event->scenePosition());
    seat->sendMousePressEvent(event->button());
    d->hoverPos = event->position();
}

/*!
 * \internal
 */
void QWaylandQuickItem::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
#if QT_CONFIG(draganddrop)
        if (d->isDragging) {
            QWaylandQuickOutput *currentOutput = qobject_cast<QWaylandQuickOutput *>(view()->output());
            //TODO: also check if dragging onto other outputs
            QWaylandQuickItem *targetItem = qobject_cast<QWaylandQuickItem *>(currentOutput->pickClickableItem(mapToScene(event->position())));
            QWaylandSurface *targetSurface = targetItem ? targetItem->surface() : nullptr;
            if (targetSurface) {
                QPointF position = mapToItem(targetItem, event->position());
                QPointF surfacePosition = targetItem->mapToSurface(position);
                seat->drag()->dragMove(targetSurface, surfacePosition);
            }
        } else
#endif // QT_CONFIG(draganddrop)
        {
            seat->sendMouseMoveEvent(d->view.data(), mapToSurface(event->position()), event->scenePosition());
            d->hoverPos = event->position();
        }
    } else {
        emit mouseMove(event->scenePosition());
        event->ignore();
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
#if QT_CONFIG(draganddrop)
        if (d->isDragging) {
            d->isDragging = false;
            seat->drag()->drop();
        } else
#endif
        {
            seat->sendMouseReleaseEvent(event->button());
        }
    } else {
        emit mouseRelease();
        event->ignore();
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (!inputRegionContains(event->position())) {
        event->ignore();
        return;
    }
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
        seat->sendMouseMoveEvent(d->view.data(), event->position(), mapToScene(event->position()));
        d->hoverPos = event->position();
    } else {
        event->ignore();
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (surface()) {
        if (!inputRegionContains(event->position())) {
            event->ignore();
            return;
        }
    }
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
        if (event->position() != d->hoverPos) {
            seat->sendMouseMoveEvent(d->view.data(), mapToSurface(event->position()), mapToScene(event->position()));
            d->hoverPos = event->position();
        }
    } else {
        event->ignore();
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
        seat->setMouseFocus(nullptr);
    } else {
        event->ignore();
    }
}

#if QT_CONFIG(wheelevent)
/*!
 * \internal
 */
void QWaylandQuickItem::wheelEvent(QWheelEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        if (!inputRegionContains(event->position())) {
            event->ignore();
            return;
        }

        QWaylandSeat *seat = compositor()->seatFor(event);
        // TODO: fix this to send a single event, when diagonal scrolling is supported
        if (event->angleDelta().x() != 0)
            seat->sendMouseWheelEvent(Qt::Horizontal, event->angleDelta().x());
        if (event->angleDelta().y() != 0)
            seat->sendMouseWheelEvent(Qt::Vertical, event->angleDelta().y());
    } else {
        event->ignore();
    }
}
#endif

/*!
 * \internal
 */
void QWaylandQuickItem::keyPressEvent(QKeyEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
        if (seat->setKeyboardFocus(d->view->surface()))
            seat->sendFullKeyEvent(event);
        else
            qWarning() << "Unable to set keyboard focus, cannot send key press event";
    } else {
        event->ignore();
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        QWaylandSeat *seat = compositor()->seatFor(event);
        seat->sendFullKeyEvent(event);
    } else {
        event->ignore();
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::touchEvent(QTouchEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents() && d->touchEventsEnabled) {
        QWaylandSeat *seat = compositor()->seatFor(event);

        QPointF pointPos;
        const QList<QTouchEvent::TouchPoint> &points = event->points();
        if (!points.isEmpty())
            pointPos = points.at(0).position();

        if (event->type() == QEvent::TouchBegin && !inputRegionContains(pointPos)) {
            event->ignore();
            return;
        }

        event->accept();
        if (seat->mouseFocus() != d->view.data()) {
            seat->sendMouseMoveEvent(d->view.data(), pointPos, mapToScene(pointPos));
        }
        seat->sendFullTouchEvent(surface(), event);

        if (event->type() == QEvent::TouchBegin) {
            d->touchingSeats.append(seat);
        } else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
            d->touchingSeats.removeOne(seat);
        }

        if (event->type() == QEvent::TouchBegin && d->focusOnClick)
            takeFocus(seat);
    } else {
        event->ignore();
    }
}

void QWaylandQuickItem::touchUngrabEvent()
{
    Q_D(QWaylandQuickItem);

    if (d->shouldSendInputEvents())
        for (auto seat : d->touchingSeats)
            seat->sendTouchCancelEvent(surface()->client());

    d->touchingSeats.clear();
}

#if QT_CONFIG(im)
/*!
 * \internal
 */
void QWaylandQuickItem::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QWaylandQuickItem);
    if (d->shouldSendInputEvents()) {
        d->oldSurface->inputMethodControl()->inputMethodEvent(event);
    } else {
        event->ignore();
    }
}
#endif

/*!
 * \internal
 */
void QWaylandQuickItem::surfaceChangedEvent(QWaylandSurface *newSurface, QWaylandSurface *oldSurface)
{
    Q_UNUSED(newSurface);
    Q_UNUSED(oldSurface);
}

void QWaylandQuickItem::handleSubsurfaceAdded(QWaylandSurface *childSurface)
{
    Q_D(QWaylandQuickItem);
    if (d->subsurfaceHandler.isNull()) {
        QWaylandQuickItem *childItem = new QWaylandQuickItem;
        childItem->setSurface(childSurface);
        childItem->setVisible(true);
        childItem->setParentItem(this);
        childItem->setParent(this);
        connect(childSurface, &QWaylandSurface::subsurfacePositionChanged, childItem, &QWaylandQuickItem::handleSubsurfacePosition);
        connect(childSurface, &QWaylandSurface::destroyed, childItem, &QObject::deleteLater);
    } else {
        bool success = QMetaObject::invokeMethod(d->subsurfaceHandler, "handleSubsurfaceAdded", Q_ARG(QWaylandSurface *, childSurface));
        if (!success)
            success = QMetaObject::invokeMethod(d->subsurfaceHandler, "handleSubsurfaceAdded",
                                                Q_ARG(QVariant, QVariant::fromValue(childSurface)));

        if (!success)
            qWarning("QWaylandQuickItem: subsurfaceHandler does not implement handleSubsurfaceAdded()");
    }
}

void QWaylandQuickItem::handlePlaceAbove(QWaylandSurface *referenceSurface)
{
    Q_D(QWaylandQuickItem);
    auto *parent = qobject_cast<QWaylandQuickItem*>(parentItem());
    if (!parent)
        return;

    if (parent->surface() == referenceSurface) {
        d->placeAboveParent();
    } else if (auto *sibling = d->findSibling(referenceSurface)) {
        d->placeAboveSibling(sibling);
    } else {
        qWarning() << "Couldn't find QWaylandQuickItem for surface" << referenceSurface
                   << "when handling wl_subsurface.place_above";
    }
}

void QWaylandQuickItem::handlePlaceBelow(QWaylandSurface *referenceSurface)
{
    Q_D(QWaylandQuickItem);
    QWaylandQuickItem *parent = qobject_cast<QWaylandQuickItem*>(parentItem());
    if (!parent)
        return;

    if (parent->surface() == referenceSurface) {
        d->placeBelowParent();
    } else if (auto *sibling = d->findSibling(referenceSurface)) {
        d->placeBelowSibling(sibling);
    } else {
        qWarning() << "Couldn't find QWaylandQuickItem for surface" << referenceSurface
                   << "when handling wl_subsurface.place_below";
    }
}

void QWaylandQuickItem::updateFocus()
{
    Q_D(const QWaylandQuickItem);
    if (hasActiveFocus() && compositor())
        compositor()->defaultSeat()->setKeyboardFocus(d->view->surface());
}

/*!
  \qmlproperty object QtWayland.Compositor::WaylandQuickItem::subsurfaceHandler

  This property provides a way to override the default subsurface behavior.

  By default, Qt will create a new SurfaceItem as a child of this item, and maintain the correct position.

  To override the default, assign a handler object to this property. The handler should implement
  a handleSubsurfaceAdded(WaylandSurface) function.

  \code
  ShellSurfaceItem {
      subsurfaceHandler: QtObject {
          function handleSubsurfaceAdded(child) {
            // create custom surface item, and connect the subsurfacePositionChanged signal
          }
      }
  }
  \endcode

  The default value of this property is \c null.
 */


QObject *QWaylandQuickItem::subsurfaceHandler() const
{
    Q_D(const QWaylandQuickItem);
    return d->subsurfaceHandler.data();
}

void QWaylandQuickItem::setSubsurfaceHandler(QObject *handler)
{
    Q_D(QWaylandQuickItem);
    if (d->subsurfaceHandler.data() != handler) {
        d->subsurfaceHandler = handler;
        emit subsurfaceHandlerChanged();
    }
}

/*!
 * \qmlproperty WaylandOutput QtWayland.Compositor::WaylandQuickItem::output
 *
 * This property holds the output on which this item is displayed.
 */
/*!
 * \property QWaylandQuickItem::output
 *
 * This property holds the output on which this item is displayed.
 */
QWaylandOutput *QWaylandQuickItem::output() const
{
    Q_D(const QWaylandQuickItem);
    return d->view->output();
}

void QWaylandQuickItem::setOutput(QWaylandOutput *output)
{
    Q_D(QWaylandQuickItem);
    d->view->setOutput(output);
}

/*!
 * \qmlproperty bool QtWayland.Compositor::WaylandQuickItem::bufferLocked
 *
 * This property holds whether the item's buffer is currently locked. As long as
 * the buffer is locked, it will not be released and returned to the client.
 *
 * The default is false.
 */
/*!
 * \property QWaylandQuickItem::bufferLocked
 *
 * This property holds whether the item's buffer is currently locked. As long as
 * the buffer is locked, it will not be released and returned to the client.
 *
 * The default is false.
 */
bool QWaylandQuickItem::isBufferLocked() const
{
    Q_D(const QWaylandQuickItem);
    return d->view->isBufferLocked();
}

void QWaylandQuickItem::setBufferLocked(bool locked)
{
    Q_D(QWaylandQuickItem);
    d->view->setBufferLocked(locked);

    // Apply the latest surface size
    if (!locked)
        updateSize();
}

/*!
 * \property QWaylandQuickItem::allowDiscardFrontBuffer
 *
 * By default, the item locks the current buffer until a new buffer is available
 * and updatePaintNode() is called. Set this property to true to allow Qt to release the buffer
 * immediately when the throttling view is no longer using it. This is useful for items that have
 * slow update intervals.
 */
bool QWaylandQuickItem::allowDiscardFrontBuffer() const
{
    Q_D(const QWaylandQuickItem);
    return d->view->allowDiscardFrontBuffer();
}

void QWaylandQuickItem::setAllowDiscardFrontBuffer(bool discard)
{
    Q_D(QWaylandQuickItem);
    d->view->setAllowDiscardFrontBuffer(discard);
}

/*!
 * \qmlmethod WaylandQuickItem::setPrimary()
 *
 * Makes this WaylandQuickItem the primary view for the surface.
 */

/*!
 * Makes this QWaylandQuickItem's view the primary view for the surface.
 *
 * \sa QWaylandSurface::primaryView
 */
void QWaylandQuickItem::setPrimary()
{
    Q_D(QWaylandQuickItem);
    d->view->setPrimary();
}

/*!
 * \internal
 */
void QWaylandQuickItem::handleSurfaceChanged()
{
    Q_D(QWaylandQuickItem);
    if (d->oldSurface) {
        disconnect(d->oldSurface.data(), &QWaylandSurface::hasContentChanged, this, &QWaylandQuickItem::surfaceMappedChanged);
        disconnect(d->oldSurface.data(), &QWaylandSurface::parentChanged, this, &QWaylandQuickItem::parentChanged);
        disconnect(d->oldSurface.data(), &QWaylandSurface::destinationSizeChanged, this, &QWaylandQuickItem::updateSize);
        disconnect(d->oldSurface.data(), &QWaylandSurface::bufferScaleChanged, this, &QWaylandQuickItem::updateSize);
        disconnect(d->oldSurface.data(), &QWaylandSurface::configure, this, &QWaylandQuickItem::updateBuffer);
        disconnect(d->oldSurface.data(), &QWaylandSurface::redraw, this, &QQuickItem::update);
        disconnect(d->oldSurface.data(), &QWaylandSurface::childAdded, this, &QWaylandQuickItem::handleSubsurfaceAdded);
        disconnect(d->oldSurface.data(), &QWaylandSurface::subsurfacePlaceAbove, this, &QWaylandQuickItem::handlePlaceAbove);
        disconnect(d->oldSurface.data(), &QWaylandSurface::subsurfacePlaceBelow, this, &QWaylandQuickItem::handlePlaceBelow);
#if QT_CONFIG(draganddrop)
        disconnect(d->oldSurface.data(), &QWaylandSurface::dragStarted, this, &QWaylandQuickItem::handleDragStarted);
#endif
#if QT_CONFIG(im)
        disconnect(d->oldSurface->inputMethodControl(), &QWaylandInputMethodControl::updateInputMethod, this, &QWaylandQuickItem::updateInputMethod);
#endif
    }
    if (QWaylandSurface *newSurface = d->view->surface()) {
        connect(newSurface, &QWaylandSurface::hasContentChanged, this, &QWaylandQuickItem::surfaceMappedChanged);
        connect(newSurface, &QWaylandSurface::parentChanged, this, &QWaylandQuickItem::parentChanged);
        connect(newSurface, &QWaylandSurface::destinationSizeChanged, this, &QWaylandQuickItem::updateSize);
        connect(newSurface, &QWaylandSurface::bufferScaleChanged, this, &QWaylandQuickItem::updateSize);
        connect(newSurface, &QWaylandSurface::configure, this, &QWaylandQuickItem::updateBuffer);
        connect(newSurface, &QWaylandSurface::redraw, this, &QQuickItem::update);
        connect(newSurface, &QWaylandSurface::childAdded, this, &QWaylandQuickItem::handleSubsurfaceAdded);
        connect(newSurface, &QWaylandSurface::subsurfacePlaceAbove, this, &QWaylandQuickItem::handlePlaceAbove);
        connect(newSurface, &QWaylandSurface::subsurfacePlaceBelow, this, &QWaylandQuickItem::handlePlaceBelow);
#if QT_CONFIG(draganddrop)
        connect(newSurface, &QWaylandSurface::dragStarted, this, &QWaylandQuickItem::handleDragStarted);
#endif
#if QT_CONFIG(im)
        connect(newSurface->inputMethodControl(), &QWaylandInputMethodControl::updateInputMethod, this, &QWaylandQuickItem::updateInputMethod);
#endif

        if (newSurface->origin() != d->origin) {
            d->origin = newSurface->origin();
            emit originChanged();
        }
        if (window()) {
            QWaylandOutput *output = newSurface->compositor()->outputFor(window());
            d->view->setOutput(output);
        }
        for (auto subsurface : QWaylandSurfacePrivate::get(newSurface)->subsurfaceChildren) {
            if (!subsurface.isNull())
                handleSubsurfaceAdded(subsurface.data());
        }

        updateSize();
    }
    surfaceChangedEvent(d->view->surface(), d->oldSurface);
    d->oldSurface = d->view->surface();
#if QT_CONFIG(im)
    updateInputMethod(Qt::ImQueryInput);
#endif
}

/*!
 * Calling this function causes the item to take the focus of the
 * input \a device.
 */
void QWaylandQuickItem::takeFocus(QWaylandSeat *device)
{
    forceActiveFocus();

    if (!surface() || !surface()->client())
        return;

    QWaylandSeat *target = device;
    if (!target) {
        target = compositor()->defaultSeat();
    }
    target->setKeyboardFocus(surface());

    qCDebug(qLcWaylandCompositorInputMethods) << Q_FUNC_INFO << " surface:" << surface()
        << ", client:" << surface()->client()
        << ", textinputprotocol:" << (int)(surface()->client()->textInputProtocols());
    if (surface()->client()->textInputProtocols().testFlag(QWaylandClient::TextInputProtocol::TextInputV2)) {
        QWaylandTextInput *textInput = QWaylandTextInput::findIn(target);
        if (textInput)
            textInput->setFocus(surface());
    }

    if (surface()->client()->textInputProtocols().testFlag(QWaylandClient::TextInputProtocol::TextInputV3)) {
        QWaylandTextInputV3 *textInputV3 = QWaylandTextInputV3::findIn(target);
        if (textInputV3)
            textInputV3->setFocus(surface());
    }

    if (surface()->client()->textInputProtocols().testFlag(QWaylandClient::TextInputProtocol::QtTextInputMethodV1)) {
        QWaylandQtTextInputMethod *textInputMethod = QWaylandQtTextInputMethod::findIn(target);
        if (textInputMethod)
            textInputMethod->setFocus(surface());
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::surfaceMappedChanged()
{
    update();
}

/*!
 * \internal
 */
void QWaylandQuickItem::parentChanged(QWaylandSurface *newParent, QWaylandSurface *oldParent)
{
    Q_UNUSED(oldParent);

    if (newParent) {
        setPaintEnabled(true);
        setVisible(true);
        setOpacity(1);
        setEnabled(true);
    }
}

/*!
 * \internal
 */
void QWaylandQuickItem::updateSize()
{
    Q_D(QWaylandQuickItem);

    // No resize if buffer is locked
    if (isBufferLocked()) {
        qWarning() << "No update on item size as the buffer is currently locked";
        return;
    }

    QSize size(0, 0);
    if (surface())
        size = surface()->destinationSize() * d->scaleFactor();

    setImplicitSize(size.width(), size.height());
}

/*!
 * \qmlproperty bool QtWayland.Compositor::WaylandQuickItem::focusOnClick
 *
 * This property specifies whether the WaylandQuickItem should take focus when
 * it is clicked or touched.
 *
 * The default is \c true.
 */

/*!
 * \property QWaylandQuickItem::focusOnClick
 *
 * This property specifies whether the QWaylandQuickItem should take focus when
 * it is clicked or touched.
 *
 * The default is \c true.
 */
bool QWaylandQuickItem::focusOnClick() const
{
    Q_D(const QWaylandQuickItem);
    return d->focusOnClick;
}

void QWaylandQuickItem::setFocusOnClick(bool focus)
{
    Q_D(QWaylandQuickItem);
    if (d->focusOnClick == focus)
        return;

    d->focusOnClick = focus;
    emit focusOnClickChanged();
}

/*!
 * Returns \c true if the input region of this item's surface contains the
 * position given by \a localPosition.
 */
bool QWaylandQuickItem::inputRegionContains(const QPointF &localPosition) const
{
    if (QWaylandSurface *s = surface())
        return s->inputRegionContains(mapToSurface(localPosition));
    return false;
}

/*!
 * \qmlmethod point WaylandQuickItem::mapToSurface(point point)
 *
 * Maps the given \a point in this item's coordinate system to the equivalent
 * point within the Wayland surface's coordinate system, and returns the mapped
 * coordinate.
 */

/*!
 * Maps the given \a point in this item's coordinate system to the equivalent
 * point within the Wayland surface's coordinate system, and returns the mapped
 * coordinate.
 */
QPointF QWaylandQuickItem::mapToSurface(const QPointF &point) const
{
    Q_D(const QWaylandQuickItem);
    if (!surface() || surface()->destinationSize().isEmpty())
        return point / d->scaleFactor();

    qreal xScale = width() / surface()->destinationSize().width();
    qreal yScale = height() / surface()->destinationSize().height();

    return QPointF(point.x() / xScale, point.y() / yScale);
}

/*!
 * \qmlmethod point WaylandQuickItem::mapFromSurface(point point)
 * \since 5.13
 *
 * Maps the given \a point in the Wayland surfaces's coordinate system to the equivalent
 * point within this item's coordinate system, and returns the mapped coordinate.
 */

/*!
 * Maps the given \a point in the Wayland surfaces's coordinate system to the equivalent
 * point within this item's coordinate system, and returns the mapped coordinate.
 *
 * \since 5.13
 */
QPointF QWaylandQuickItem::mapFromSurface(const QPointF &point) const
{
    Q_D(const QWaylandQuickItem);
    if (!surface() || surface()->destinationSize().isEmpty())
        return point * d->scaleFactor();

    qreal xScale = width() / surface()->destinationSize().width();
    qreal yScale = height() / surface()->destinationSize().height();

    return QPointF(point.x() * xScale, point.y() * yScale);
}

#if QT_CONFIG(im)
QVariant QWaylandQuickItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    return inputMethodQuery(query, QVariant());
}

QVariant QWaylandQuickItem::inputMethodQuery(Qt::InputMethodQuery query, QVariant argument) const
{
    Q_D(const QWaylandQuickItem);

    if (query == Qt::ImEnabled)
        return QVariant((flags() & ItemAcceptsInputMethod) != 0);

    if (d->oldSurface)
        return d->oldSurface->inputMethodControl()->inputMethodQuery(query, argument);

    return QVariant();
}
#endif

/*!
    \qmlproperty bool QtWayland.Compositor::WaylandQuickItem::paintEnabled

    Returns true if the item is hidden, though the texture
    is still updated. As opposed to hiding the item by
    setting \l{Item::visible}{visible} to \c false, setting this property to \c false
    will not prevent mouse or keyboard input from reaching item.
*/

/*!
    \property QWaylandQuickItem::paintEnabled

    Holds \c true if the item is hidden, though the texture
    is still updated. As opposed to hiding the item by
    setting \l{QQuickItem::}{visible} to \c false, setting this property to \c false
    will not prevent mouse or keyboard input from reaching item.
*/
bool QWaylandQuickItem::isPaintEnabled() const
{
    Q_D(const QWaylandQuickItem);
    return d->paintEnabled;
}

void QWaylandQuickItem::setPaintEnabled(bool enabled)
{
    Q_D(QWaylandQuickItem);

    if (enabled != d->paintEnabled) {
        d->paintEnabled = enabled;
        emit paintEnabledChanged();
    }

    update();
}

/*!
    \qmlproperty  bool QtWayland.Compositor::WaylandQuickItem::touchEventsEnabled

    This property holds \c true if touch events are forwarded to the client
    surface, \c false otherwise.
*/

/*!
    \property QWaylandQuickItem::touchEventsEnabled

    This property holds \c true if touch events are forwarded to the client
    surface, \c false otherwise.
*/
bool QWaylandQuickItem::touchEventsEnabled() const
{
    Q_D(const QWaylandQuickItem);
    return d->touchEventsEnabled;
}

void QWaylandQuickItem::updateBuffer(bool hasBuffer)
{
    Q_D(QWaylandQuickItem);
    Q_UNUSED(hasBuffer);
    if (d->origin != surface()->origin()) {
        d->origin = surface()->origin();
        emit originChanged();
    }
}

void QWaylandQuickItem::updateWindow()
{
    Q_D(QWaylandQuickItem);

    QQuickWindow *newWindow = window();
    if (newWindow == d->connectedWindow)
        return;

    if (d->connectedWindow) {
        disconnect(d->connectedWindow, &QQuickWindow::beforeSynchronizing, this, &QWaylandQuickItem::beforeSync);
        disconnect(d->connectedWindow, &QQuickWindow::screenChanged, this, &QWaylandQuickItem::updateSize);
    }

    d->connectedWindow = newWindow;

    if (d->connectedWindow) {
        connect(d->connectedWindow, &QQuickWindow::beforeSynchronizing, this, &QWaylandQuickItem::beforeSync, Qt::DirectConnection);
        connect(d->connectedWindow, &QQuickWindow::screenChanged, this, &QWaylandQuickItem::updateSize); // new screen may have new dpr

        if (compositor()) {
            QWaylandOutput *output = compositor()->outputFor(d->connectedWindow);
            Q_ASSERT(output);
            d->view->setOutput(output);
        }

        updateSize(); // because scaleFactor depends on devicePixelRatio, which may be different for the new window
    }
}

void QWaylandQuickItem::updateOutput()
{
    Q_D(QWaylandQuickItem);
    if (d->view->output() == d->connectedOutput)
        return;

    if (d->connectedOutput)
        disconnect(d->connectedOutput, &QWaylandOutput::scaleFactorChanged, this, &QWaylandQuickItem::updateSize);

    d->connectedOutput = d->view->output();

    if (d->connectedOutput)
        connect(d->connectedOutput, &QWaylandOutput::scaleFactorChanged, this, &QWaylandQuickItem::updateSize);

    updateSize();
}

void QWaylandQuickItem::beforeSync()
{
    Q_D(QWaylandQuickItem);
    if (d->view->advance()) {
        d->newTexture = true;
        update();
    }
}

#if QT_CONFIG(im)
void QWaylandQuickItem::updateInputMethod(Qt::InputMethodQueries queries)
{
    Q_D(QWaylandQuickItem);

    setFlag(QQuickItem::ItemAcceptsInputMethod,
            d->oldSurface ? d->oldSurface->inputMethodControl()->enabled() : false);
    QQuickItem::updateInputMethod(queries | Qt::ImEnabled);
}
#endif

/*!
 * \qmlsignal void QtWayland.Compositor::WaylandQuickItem::surfaceDestroyed()
 *
 * This signal is emitted when the client has destroyed the \c wl_surface associated
 * with the WaylandQuickItem. The handler for this signal is expected to either destroy the
 * WaylandQuickItem immediately or start a close animation and then destroy the Item.
 *
 * If an animation is started, bufferLocked should be set to ensure the item keeps its content
 * until the animation finishes
 *
 * \sa bufferLocked
 */

/*!
 * \fn void QWaylandQuickItem::surfaceDestroyed()
 *
 * This signal is emitted when the client has destroyed the \c wl_surface associated
 * with the QWaylandQuickItem. The handler for this signal is expected to either destroy the
 * QWaylandQuickItem immediately or start a close animation and then destroy the Item.
 *
 * If an animation is started, bufferLocked should be set to ensure the item keeps its content
 * until the animation finishes
 *
 * \sa QWaylandQuickItem::bufferLocked
 */

QSGNode *QWaylandQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_D(QWaylandQuickItem);
    d->lastMatrix = data->transformNode->combinedMatrix();
    const bool bufferHasContent = d->view->currentBuffer().hasContent();

    if (d->view->isBufferLocked() && d->paintEnabled)
        return oldNode;

    if (!bufferHasContent || !d->paintEnabled || !surface()) {
        delete oldNode;
        return nullptr;
    }

    QWaylandBufferRef ref = d->view->currentBuffer();
    const bool invertY = ref.origin() == QWaylandSurface::OriginBottomLeft;
    const QRectF rect = invertY ? QRectF(0, height(), width(), -height())
                                : QRectF(0, 0, width(), height());

    if (ref.isSharedMemory()
#if QT_CONFIG(opengl)
            || bufferTypes[ref.bufferFormatEgl()].canProvideTexture
#endif
    ) {
#if QT_CONFIG(opengl)
        if (oldNode && !d->paintByProvider) {
            // Need to re-create a node
            delete oldNode;
            oldNode = nullptr;
        }
        d->paintByProvider = true;
#endif
        // This case could covered by the more general path below, but this is more efficient (especially when using ShaderEffect items).
        QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);

        if (!node) {
            node = new QSGSimpleTextureNode();
            if (smooth())
                node->setFiltering(QSGTexture::Linear);
            d->newTexture = true;
        }

        if (!d->provider) {
            d->provider = new QWaylandSurfaceTextureProvider();
            if (compositor()) {
                d->texProviderConnection =
                    QObject::connect(
                            compositor(),
                            &QObject::destroyed,
                            this,
                            [this](QObject*) {
                                    auto *itemPriv = QWaylandQuickItemPrivate::get(this);
                                    if (itemPriv->provider) {
                                        itemPriv->provider->deleteLater();
                                        itemPriv->provider = nullptr;
                                    }
                                    disconnect(itemPriv->texProviderConnection); }
                    );
            }
        }

        if (d->newTexture) {
            d->newTexture = false;
            d->provider->setBufferRef(this, ref);
            node->setTexture(d->provider->texture());
        }

        d->provider->setSmooth(smooth());
        node->setRect(rect);

        qreal scale = surface()->bufferScale();
        QRectF source = surface()->sourceGeometry();
        node->setSourceRect(QRectF(source.topLeft() * scale, source.size() * scale));

        return node;
    }

#if QT_CONFIG(opengl)
    Q_ASSERT(!d->provider);

    if (oldNode && d->paintByProvider) {
        // Need to re-create a node
        delete oldNode;
        oldNode = nullptr;
    }
    d->paintByProvider = false;

    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(oldNode);

    if (!node) {
        node = new QSGGeometryNode;
        d->newTexture = true;
    }

    QSGGeometry *geometry = node->geometry();
    QWaylandBufferMaterial *material = static_cast<QWaylandBufferMaterial *>(node->material());

    if (!geometry)
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

    if (!material)
        material = new QWaylandBufferMaterial(ref.bufferFormatEgl());

    if (d->newTexture) {
        d->newTexture = false;
        material->setBufferRef(this, ref);
    }

    const QSize surfaceSize = ref.size() / surface()->bufferScale();
    const QRectF sourceGeometry = surface()->sourceGeometry();
    const QRectF normalizedCoordinates =
            sourceGeometry.isValid()
            ? QRectF(sourceGeometry.x() / surfaceSize.width(),
                     sourceGeometry.y() / surfaceSize.height(),
                     sourceGeometry.width() / surfaceSize.width(),
                     sourceGeometry.height() / surfaceSize.height())
            : QRectF(0, 0, 1, 1);

    QSGGeometry::updateTexturedRectGeometry(geometry, rect, normalizedCoordinates);

    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry, true);

    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial, true);

    return node;
#else
    qCWarning(qLcWaylandCompositor) << "Without OpenGL support only shared memory textures are supported";
    return nullptr;
#endif // QT_CONFIG(opengl)
}

void QWaylandQuickItem::setTouchEventsEnabled(bool enabled)
{
    Q_D(QWaylandQuickItem);
    if (d->touchEventsEnabled != enabled) {
        d->touchEventsEnabled = enabled;
        emit touchEventsEnabledChanged();
    }
}

bool QWaylandQuickItem::inputEventsEnabled() const
{
    Q_D(const QWaylandQuickItem);
    return d->inputEventsEnabled;
}

void QWaylandQuickItem::setInputEventsEnabled(bool enabled)
{
    Q_D(QWaylandQuickItem);
    if (d->inputEventsEnabled != enabled) {
        if (enabled)
            setEnabled(true);
        d->setInputEventsEnabled(enabled);
        emit inputEventsEnabledChanged();
    }
}

void QWaylandQuickItem::lower()
{
    Q_D(QWaylandQuickItem);
    d->lower();
}

void QWaylandQuickItemPrivate::lower()
{
    Q_Q(QWaylandQuickItem);
    QQuickItem *parent = q->parentItem();
    Q_ASSERT(parent);
    QQuickItem *bottom = parent->childItems().constFirst();
    if (q != bottom)
        q->stackBefore(bottom);
}

void QWaylandQuickItem::raise()
{
    Q_D(QWaylandQuickItem);
    d->raise();
}

void QWaylandQuickItemPrivate::raise()
{
    Q_Q(QWaylandQuickItem);
    QQuickItem *parent = q->parentItem();
    Q_ASSERT(parent);
    QQuickItem *top = parent->childItems().constLast();
    if (q != top)
        q->stackAfter(top);
}

void QWaylandQuickItem::sendMouseMoveEvent(const QPointF &position, QWaylandSeat *seat)
{
    if (seat == nullptr)
        seat = compositor()->defaultSeat();

    if (!seat) {
        qWarning() << "No seat, can't send mouse event";
        return;
    }

    seat->sendMouseMoveEvent(view(), position);
}

/*!
 * \internal
 *
 * Sets the position of this item relative to the parent item.
 */
void QWaylandQuickItem::handleSubsurfacePosition(const QPoint &pos)
{
    Q_D(QWaylandQuickItem);
    QQuickItem::setPosition(pos * d->scaleFactor());
}

#if QT_CONFIG(draganddrop)
void QWaylandQuickItem::handleDragStarted(QWaylandDrag *drag)
{
    Q_D(QWaylandQuickItem);
    Q_ASSERT(drag->origin() == surface());
    drag->seat()->setMouseFocus(nullptr);
    d->isDragging = true;
}
#endif

qreal QWaylandQuickItemPrivate::scaleFactor() const
{
    qreal f = view->output() ? view->output()->scaleFactor() : 1;
#if !defined(Q_OS_MACOS)
    if (window)
        f /= window->devicePixelRatio();
#endif
    return f;
}

QWaylandQuickItem *QWaylandQuickItemPrivate::findSibling(QWaylandSurface *surface) const
{
    Q_Q(const QWaylandQuickItem);
    auto *parent = q->parentItem();
    if (!parent)
        return nullptr;

    const auto siblings = q->parentItem()->childItems();
    for (auto *sibling : siblings) {
        auto *waylandItem = qobject_cast<QWaylandQuickItem *>(sibling);
        if (waylandItem && waylandItem->surface() == surface)
            return waylandItem;
    }
    return nullptr;
}

void QWaylandQuickItemPrivate::placeAboveSibling(QWaylandQuickItem *sibling)
{
    Q_Q(QWaylandQuickItem);
    q->stackAfter(sibling);
    q->setZ(sibling->z());
    belowParent = sibling->d_func()->belowParent;
}

void QWaylandQuickItemPrivate::placeBelowSibling(QWaylandQuickItem *sibling)
{
    Q_Q(QWaylandQuickItem);
    q->stackBefore(sibling);
    q->setZ(sibling->z());
    belowParent = sibling->d_func()->belowParent;
}

//### does not handle changes in z value if parent is a subsurface
void QWaylandQuickItemPrivate::placeAboveParent()
{
    Q_Q(QWaylandQuickItem);
    const auto siblings = q->parentItem()->childItems();

    // Stack below first (bottom) sibling above parent
    bool foundSibling = false;
    for (auto it = siblings.cbegin(); it != siblings.cend(); ++it) {
        QWaylandQuickItem *sibling = qobject_cast<QWaylandQuickItem*>(*it);
        if (sibling && !sibling->d_func()->belowParent) {
            q->stackBefore(sibling);
            foundSibling = true;
            break;
        }
    }

    // No other subsurfaces above parent
    if (!foundSibling && siblings.last() != q)
        q->stackAfter(siblings.last());

    q->setZ(q->parentItem()->z());
    belowParent = false;
}

//### does not handle changes in z value if parent is a subsurface
void QWaylandQuickItemPrivate::placeBelowParent()
{
    Q_Q(QWaylandQuickItem);
    const auto siblings = q->parentItem()->childItems();

    // Stack above last (top) sibling below parent
    bool foundSibling = false;
    for (auto it = siblings.crbegin(); it != siblings.crend(); ++it) {
        QWaylandQuickItem *sibling = qobject_cast<QWaylandQuickItem*>(*it);
        if (sibling && sibling->d_func()->belowParent) {
            q->stackAfter(sibling);
            foundSibling = true;
            break;
        }
    }

    // No other subsurfaces below parent
    if (!foundSibling && siblings.first() != q)
        q->stackBefore(siblings.first());

    q->setZ(q->parentItem()->z() - 1.0);
    belowParent = true;
}

QT_END_NAMESPACE

#include "moc_qwaylandquickitem.cpp"
