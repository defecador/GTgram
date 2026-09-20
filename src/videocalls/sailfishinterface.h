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
#ifndef SAILFISHINTERFACE_H
#define SAILFISHINTERFACE_H

// tgcalls::PlatformInterface for Sailfish, replacing the vendored no-op
// FakeInterface now that video is wired up. Provides the builtin VP8/VP9
// encoder/decoder factories, a broadcaster-backed track source, and the
// QtMultimedia-based camera capturer. CreatePlatformInterface() (defined in
// the .cpp) is the single entry point tgcalls calls via
// PlatformInterface::SharedInstance() - and the reason
// platform/fake/FakeInterface.cpp must NOT be compiled alongside this file,
// since both define that same symbol.

#include "platform/PlatformInterface.h"

class SailfishInterface : public tgcalls::PlatformInterface
{
public:
    std::unique_ptr<webrtc::VideoEncoderFactory> makeVideoEncoderFactory(
        bool preferHardwareEncoding = false, bool isScreencast = false) override;
    std::unique_ptr<webrtc::VideoDecoderFactory> makeVideoDecoderFactory() override;
    bool supportsEncoding(const std::string &codecName) override;
    webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> makeVideoSource(
        rtc::Thread *signalingThread, rtc::Thread *workerThread) override;
    void adaptVideoSource(webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource,
                          int width, int height, int fps) override;
    std::unique_ptr<tgcalls::VideoCapturerInterface> makeVideoCapturer(
        webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source,
        std::string deviceId,
        std::function<void(tgcalls::VideoState)> stateUpdated,
        std::function<void(tgcalls::PlatformCaptureInfo)> captureInfoUpdated,
        std::shared_ptr<tgcalls::PlatformContext> platformContext,
        std::pair<int, int> &outResolution) override;
};

#endif // SAILFISHINTERFACE_H
