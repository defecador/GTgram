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
#ifndef CALLCAMERAGRABBER_H
#define CALLCAMERAGRABBER_H

// Camera grabber for video calls. A QObject that lives on the GUI thread
// (constructed on tgcalls' media thread, then moved) since QCamera needs a
// Qt event loop: opens a QCamera + headless QAbstractVideoSurface, and for
// each delivered frame converts NV21/NV12/YUV420P/YV12/YUYV/UYVY/RGB32 into a
// webrtc::VideoFrame (I420, via libyuv) handed to a callback - set by
// SailfishVideoCapturer to feed the outgoing track source's broadcaster.

#include <QObject>
#include <QPointer>
#include <QMutex>
#include <functional>
#include <atomic>
#include <utility>

// Forward decl only - moc chokes on webrtc's video_frame.h.
namespace webrtc { class VideoFrame; }

class QCamera;
class QVideoFrame;
class CallCameraSurface;

class CallCameraGrabber : public QObject
{
    Q_OBJECT
public:
    explicit CallCameraGrabber(QObject *parent = nullptr);
    ~CallCameraGrabber() override;

    // Set before start(); invoked on the camera's own delivery thread.
    void setFrameCallback(std::function<void(const webrtc::VideoFrame &)> cb);
    // Last known resolution (0,0 until the first frame arrives).
    std::pair<int, int> resolution() const;

    // Called by CallCameraSurface on the delivery thread: converts and forwards.
    void handleFrame(const QVideoFrame &frame);

public slots:
    void start(bool front);
    void stop();

private:
    QPointer<QCamera> m_camera;
    CallCameraSurface *m_surface;
    std::function<void(const webrtc::VideoFrame &)> m_cb;
    QMutex m_cbMutex;
    std::atomic<int> m_width;
    std::atomic<int> m_height;
    // Rotation (degrees) reported on each outgoing frame - see the constants
    // in callcameragrabber.cpp for why these are hardcoded and unverified.
    std::atomic<int> m_rotation;
    unsigned m_frameCount; // throttles the periodic frame-rate log line
};

#endif // CALLCAMERAGRABBER_H
