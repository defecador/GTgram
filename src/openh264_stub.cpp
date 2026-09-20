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

    Stub for openh264.

    The prebuilt libtg_owt.a references openh264's H264 encoder
    (WelsCreateSVCEncoder / WelsDestroySVCEncoder) from its codec factory
    registration code, but the SailfishOS SDK target doesn't provide
    libopenh264 - so these stubs exist purely to satisfy the linker.

    They stay unreached even with video calls enabled: H.264 is never
    negotiated, because SailfishInterface::supportsEncoding() (see
    src/videocalls/sailfishinterface.cpp) only advertises VP8/VP9, and
    webrtc's FilterAndSortEncoders() erases any codec a platform doesn't
    advertise from the offered list entirely - it never falls through to
    these stubs, it just never asks.
*/

extern "C" {

// int WelsCreateSVCEncoder(ISVCEncoder** ppEncoder) - cmResultSuccess == 0.
// Returns non-zero (failure) so nothing proceeds with a null encoder; never
// actually called, since H.264 is never negotiated (see above).
int WelsCreateSVCEncoder(void **ppEncoder)
{
    if (ppEncoder) {
        *ppEncoder = 0;
    }
    return 1; // != cmResultSuccess
}

void WelsDestroySVCEncoder(void *pEncoder)
{
    (void)pEncoder;
}

} // extern "C"
