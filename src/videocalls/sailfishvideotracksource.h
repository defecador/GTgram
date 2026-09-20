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
#ifndef SAILFISHVIDEOTRACKSOURCE_H
#define SAILFISHVIDEOTRACKSOURCE_H

#include "pc/video_track_source.h"
#include "api/video/video_sink_interface.h"
#include "media/base/video_broadcaster.h"

#include <memory>

// The outgoing video track source: a webrtc::VideoTrackSource exposing an
// rtc::VideoBroadcaster as its source. CallCameraGrabber (via
// SailfishVideoCapturer) pushes captured I420 frames into sink()->OnFrame(),
// and the broadcaster forwards them to whatever's subscribed - the encoder,
// plus CallManager's local preview renderer.
class SailfishVideoTrackSource : public webrtc::VideoTrackSource
{
public:
    SailfishVideoTrackSource()
        : webrtc::VideoTrackSource(/*remote=*/false)
        , m_broadcaster(std::make_shared<rtc::VideoBroadcaster>())
    {
    }

    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink()
    {
        return m_broadcaster;
    }

protected:
    rtc::VideoSourceInterface<webrtc::VideoFrame> *source() override
    {
        return m_broadcaster.get();
    }

private:
    std::shared_ptr<rtc::VideoBroadcaster> m_broadcaster;
};

#endif // SAILFISHVIDEOTRACKSOURCE_H
