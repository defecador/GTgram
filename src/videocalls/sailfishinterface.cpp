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
#include "sailfishinterface.h"
#include "sailfishvideotracksource.h"
#include "sailfishvideocapturer.h"

#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "pc/video_track_source_proxy.h"
#include "rtc_base/ref_counted_object.h"

namespace {
// Recovers the sink (broadcaster) from the proxied track source, so the
// capturer has somewhere to push frames.
std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> GetSink(
        const webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> &nativeSource)
{
    auto *proxy = static_cast<webrtc::VideoTrackSourceProxy *>(nativeSource.get());
    auto *internal = static_cast<SailfishVideoTrackSource *>(proxy->internal());
    return internal->sink();
}
} // namespace

std::unique_ptr<webrtc::VideoEncoderFactory> SailfishInterface::makeVideoEncoderFactory(
        bool /*preferHardwareEncoding*/, bool /*isScreencast*/)
{
    return webrtc::CreateBuiltinVideoEncoderFactory();
}

std::unique_ptr<webrtc::VideoDecoderFactory> SailfishInterface::makeVideoDecoderFactory()
{
    return webrtc::CreateBuiltinVideoDecoderFactory();
}

bool SailfishInterface::supportsEncoding(const std::string &codecName)
{
    // VP8/VP9 via the already-bundled libvpx. Native H.264 is only a link-time
    // stub (src/openh264_stub.cpp) - never offer it. FilterAndSortEncoders()
    // erases any codec not returned here from the negotiated format list
    // entirely, so this genuinely keeps H.264 unreached, not just deprioritized.
    return codecName == "VP8" || codecName == "VP9";
}

webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> SailfishInterface::makeVideoSource(
        rtc::Thread *signalingThread, rtc::Thread *workerThread)
{
    const auto trackSource = webrtc::scoped_refptr<SailfishVideoTrackSource>(
        new rtc::RefCountedObject<SailfishVideoTrackSource>());
    return trackSource
        ? webrtc::VideoTrackSourceProxy::Create(signalingThread, workerThread, trackSource)
        : nullptr;
}

void SailfishInterface::adaptVideoSource(
        webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> /*videoSource*/,
        int /*width*/, int /*height*/, int /*fps*/)
{
    // Not implemented - the broadcaster doesn't self-adapt resolution/fps.
}

std::unique_ptr<tgcalls::VideoCapturerInterface> SailfishInterface::makeVideoCapturer(
        webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source,
        std::string deviceId,
        std::function<void(tgcalls::VideoState)> stateUpdated,
        std::function<void(tgcalls::PlatformCaptureInfo)> /*captureInfoUpdated*/,
        std::shared_ptr<tgcalls::PlatformContext> /*platformContext*/,
        std::pair<int, int> &outResolution)
{
    // deviceId "back" selects the rear camera; anything else (including
    // empty, CallManager's default) selects the front camera.
    const bool front = (deviceId != "back");
    return std::make_unique<SailfishVideoCapturer>(
        source, GetSink(source), front, std::move(stateUpdated), outResolution);
}

// The single entry point tgcalls requires (PlatformInterface::SharedInstance()).
// Replaces the one in platform/fake/FakeInterface.cpp, which must not be
// compiled once this file is - both define this same symbol.
namespace tgcalls {
std::unique_ptr<PlatformInterface> CreatePlatformInterface()
{
    return std::make_unique<SailfishInterface>();
}
} // namespace tgcalls
