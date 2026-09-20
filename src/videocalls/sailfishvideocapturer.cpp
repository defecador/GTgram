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
#include "sailfishvideocapturer.h"
#include "callcameragrabber.h"

#include "VideoCaptureInterface.h" // tgcalls::VideoState enum values

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

SailfishVideoCapturer::SailfishVideoCapturer(
        webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source,
        std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink,
        bool front,
        std::function<void(tgcalls::VideoState)> stateUpdated,
        std::pair<int, int> &outResolution)
    : _source(source)
    , _sink(sink)
    , _grabber(new CallCameraGrabber())
    , _front(front)
    , _state(tgcalls::VideoState::Inactive)
    , _stateUpdated(std::move(stateUpdated))
{
    // Captured frames (delivered on the camera's own thread) get pushed into
    // the track source's broadcaster -> encoder. The sink is thread-safe.
    auto sinkCopy = _sink;
    _grabber->setFrameCallback([sinkCopy](const webrtc::VideoFrame &frame) {
        if (sinkCopy) {
            sinkCopy->OnFrame(frame);
        }
    });

    // The grabber must live on the GUI thread (QCamera needs a Qt event
    // loop): constructed here (tgcalls' media thread), then moved to qApp's.
    if (QCoreApplication::instance()) {
        _grabber->moveToThread(QCoreApplication::instance()->thread());
    }

    // Declared initial resolution (the camera is 4:3); the real one arrives
    // with the first captured frame.
    outResolution = { 1280, 960 };
}

SailfishVideoCapturer::~SailfishVideoCapturer()
{
    // Detach the callback immediately so no more frames arrive during teardown.
    if (_grabber) {
        _grabber->setFrameCallback(nullptr);
        QMetaObject::invokeMethod(_grabber, "stop", Qt::QueuedConnection);
        _grabber->deleteLater(); // destroyed on its own thread (GUI)
        _grabber = nullptr;
    }
}

void SailfishVideoCapturer::setState(tgcalls::VideoState state)
{
    _state = state;
    if (!_grabber) {
        return;
    }
    if (state == tgcalls::VideoState::Active) {
        QMetaObject::invokeMethod(_grabber, "start", Qt::QueuedConnection,
                                  Q_ARG(bool, _front));
    } else {
        QMetaObject::invokeMethod(_grabber, "stop", Qt::QueuedConnection);
    }
    if (_stateUpdated) {
        _stateUpdated(state);
    }
}

void SailfishVideoCapturer::setPreferredCaptureAspectRatio(float /*aspectRatio*/)
{
    // Not implemented - no crop/aspect adaptation for v1.
}

void SailfishVideoCapturer::setUncroppedOutput(
        std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink)
{
    // Extra sink hooked directly to the source (used for the local preview).
    if (_uncroppedSink) {
        _source->RemoveSink(_uncroppedSink.get());
    }
    _uncroppedSink = sink;
    if (_uncroppedSink) {
        _source->AddOrUpdateSink(_uncroppedSink.get(), rtc::VideoSinkWants());
    }
}

int SailfishVideoCapturer::getRotation()
{
    // Rotation is baked into each frame by CallCameraGrabber instead - see
    // its FRONT/BACK_CAMERA_ROTATION_DEGREES constants.
    return 0;
}
