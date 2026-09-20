/*
    This file is part of GTgram.

    GTgram is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GTgram is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GTgram. If not, see <http://www.gnu.org/licenses/>.

    Video capture/rendering in this directory is adapted from
    RooTelegram-SailfishOS (https://github.com/RootGPT-YouTube/RooTelegram-SailfishOS,
    GPLv3), which built it on top of tgcalls (LGPLv3,
    https://github.com/TelegramMessenger/tgcalls).
*/
#include "callvideorenderer.h"

#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QVideoFrame>
#include <QPointer>
#include <QMetaObject>
#include <QTransform>

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "api/video/i420_buffer.h"
#include "libyuv.h"

// webrtc-side sink: receives frames on webrtc's own thread, converts
// I420->RGB, and marshals the result to the renderer (GUI thread). Held via
// QPointer so a renderer destroyed mid-call doesn't leave a dangling target.
class CallVideoRenderer::SinkBridge : public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    explicit SinkBridge(CallVideoRenderer *renderer) : m_renderer(renderer) {}

    void OnFrame(const webrtc::VideoFrame &frame) override
    {
        webrtc::scoped_refptr<webrtc::I420BufferInterface> buf = frame.video_frame_buffer()->ToI420();
        if (!buf) {
            return;
        }
        const int w = buf->width();
        const int h = buf->height();
        if (w <= 0 || h <= 0) {
            return;
        }
        QImage img(w, h, QImage::Format_RGB32);
        libyuv::I420ToARGB(buf->DataY(), buf->StrideY(),
                           buf->DataU(), buf->StrideU(),
                           buf->DataV(), buf->StrideV(),
                           img.bits(), img.bytesPerLine(), w, h);
        // Apply whatever rotation the frame carries - the remote end may send
        // a different value than what CallCameraGrabber sets on ours.
        if (frame.rotation() != webrtc::kVideoRotation_0) {
            QTransform t;
            t.rotate(frame.rotation());
            img = img.transformed(t);
        }
        if (m_renderer) {
            QMetaObject::invokeMethod(m_renderer, "presentImage", Qt::QueuedConnection,
                                      Q_ARG(QImage, img));
        }
    }

private:
    QPointer<CallVideoRenderer> m_renderer;
};

CallVideoRenderer::CallVideoRenderer(QObject *parent)
    : QObject(parent)
    , m_surface(nullptr)
    , m_hasFrame(false)
{
}

CallVideoRenderer::~CallVideoRenderer()
{
    reset();
}

void CallVideoRenderer::setVideoSurface(QAbstractVideoSurface *surface)
{
    if (m_surface == surface) {
        return;
    }
    if (m_surface && m_surface->isActive()) {
        m_surface->stop();
    }
    m_surface = surface;
    m_size = QSize();
    emit videoSurfaceChanged();
}

std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> CallVideoRenderer::sink()
{
    if (!m_sink) {
        m_sink = std::make_shared<SinkBridge>(this);
    }
    return m_sink;
}

void CallVideoRenderer::reset()
{
    m_sink.reset();
    if (m_surface && m_surface->isActive()) {
        m_surface->stop();
    }
    m_size = QSize();
    if (m_hasFrame) {
        m_hasFrame = false;
        emit hasFrameChanged();
    }
}

void CallVideoRenderer::presentImage(const QImage &image)
{
    if (!m_surface || image.isNull()) {
        return;
    }
    if (!m_surface->isActive() || m_size != image.size()) {
        if (m_surface->isActive()) {
            m_surface->stop();
        }
        QVideoSurfaceFormat fmt(image.size(), QVideoFrame::Format_RGB32);
        m_surface->start(fmt);
        m_size = image.size();
    }
    m_surface->present(QVideoFrame(image));
    if (!m_hasFrame) {
        m_hasFrame = true;
        emit hasFrameChanged();
    }
}
