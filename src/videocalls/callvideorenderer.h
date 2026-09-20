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
#ifndef CALLVIDEORENDERER_H
#define CALLVIDEORENDERER_H

// Renders one video stream (remote, or the local preview) into QML. Exposes
// a videoSurface consumed directly by QML's VideoOutput, and hands out a
// webrtc sink to attach to tgcalls (Instance::setIncomingVideoOutput() for
// the remote stream, VideoCaptureInterface::setOutput() for the local one).
// Incoming I420 frames are converted to RGB and presented on the GUI thread.

#include <QObject>
#include <QImage>
#include <QSize>
#include <memory>

class QAbstractVideoSurface;

namespace rtc {
template <typename T> class VideoSinkInterface;
}
namespace webrtc { class VideoFrame; }

class CallVideoRenderer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface WRITE setVideoSurface NOTIFY videoSurfaceChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)

public:
    explicit CallVideoRenderer(QObject *parent = nullptr);
    ~CallVideoRenderer() override;

    QAbstractVideoSurface *videoSurface() const { return m_surface; }
    void setVideoSurface(QAbstractVideoSurface *surface);
    bool hasFrame() const { return m_hasFrame; }

    // The webrtc sink to hand to tgcalls (shared_ptr: the caller keeps it alive).
    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink();
    // Clears state at the end of a call: stops the surface, hasFrame -> false.
    void reset();

signals:
    void videoSurfaceChanged();
    void hasFrameChanged();

private slots:
    // Presents on the GUI thread (marshaled from the webrtc delivery thread).
    void presentImage(const QImage &image);

private:
    class SinkBridge;
    QAbstractVideoSurface *m_surface;
    std::shared_ptr<SinkBridge> m_sink;
    QSize m_size;
    bool m_hasFrame;
};

#endif // CALLVIDEORENDERER_H
