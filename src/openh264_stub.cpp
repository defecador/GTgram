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

    Stub for openh264 (voice calls only).

    The prebuilt libtg_owt.a references openh264's H264 encoder
    (WelsCreateSVCEncoder / WelsDestroySVCEncoder), but the SailfishOS SDK
    target doesn't provide libopenh264. That encoder is only needed for
    VIDEO calls - voice calls never invoke it. These stubs just satisfy the
    linker and fail cleanly if ever called by mistake.

    Would need replacing with the real libopenh264 if video calls are added.
*/

extern "C" {

// int WelsCreateSVCEncoder(ISVCEncoder** ppEncoder) - cmResultSuccess == 0.
// Returns non-zero (failure) so nothing proceeds with a null encoder; never
// actually called for voice calls.
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
