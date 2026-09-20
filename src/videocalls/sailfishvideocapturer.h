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
#ifndef SAILFISHVIDEOCAPTURER_H
#define SAILFISHVIDEOCAPTURER_H

// tgcalls::VideoCapturerInterface implementation for Sailfish. Owns a
// CallCameraGrabber (a QObject living on the GUI thread) and routes its I420
// frames into the outgoing track source's sink (broadcaster), so the encoder
// picks them up.

#include "VideoCapturerInterface.h" // tgcalls (brings in Instance.h -> VideoState)
#include "api/media_stream_interface.h"
#include "api/video/video_sink_interface.h"
#include "api/scoped_refptr.h"

#include <memory>
#include <utility>
#include <functional>

class CallCameraGrabber;

class SailfishVideoCapturer : public tgcalls::VideoCapturerInterface
{
public:
    SailfishVideoCapturer(
        webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source,
        std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink,
        bool front,
        std::function<void(tgcalls::VideoState)> stateUpdated,
        std::pair<int, int> &outResolution);
    ~SailfishVideoCapturer() override;

    void setState(tgcalls::VideoState state) override;
    void setPreferredCaptureAspectRatio(float aspectRatio) override;
    void setUncroppedOutput(std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink) override;
    int getRotation() override;

private:
    webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> _source;
    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> _sink;
    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> _uncroppedSink;
    CallCameraGrabber *_grabber;
    bool _front;
    tgcalls::VideoState _state;
    std::function<void(tgcalls::VideoState)> _stateUpdated;
};

#endif // SAILFISHVIDEOCAPTURER_H
