# GTgram
A Telegram client for Sailfish OS.

By Guillermo Torres + Vibe coding.

GTgram installs as its own package, with its own data, launcher entry and icon. Beyond the
client it is based on, it adds:

- Chat category tabs in the overview: All, Private, Groups and Channels
- Replying and reacting straight from notifications
- Silent forwarding
- Pulling up at the end of a chat to open the next unread one
- Edit and delete menu entries that work with the bundled TDLib

## License
Licensed under the GNU GPLv3. Source files keep the copyright notices of their original authors.

## Build
GTgram builds with the Sailfish OS SDK's `mb2` for Sailfish OS 5.2, for example inside the
`coderus/sailfishos-platform-sdk` container:

    mb2 -t SailfishOS-5.2.0.15-aarch64 build

Before building:

- Check out the submodules: `git submodule update --init`.
- Provide a compiled [TDLib](https://github.com/tdlib/td) (GTgram currently bundles 1.8.62) in
  `tdlib/<architecture>/lib`, containing at least `libtdjson.so` - for example `tdlib/aarch64/lib`.
- Create `src/tdlibsecrets.h` with your own Telegram API ID and hash, which you get by
  registering an application on [my.telegram.org](https://my.telegram.org):

```
#ifndef TDLIBSECRETS_H
#define TDLIBSECRETS_H
const char TDLIB_API_ID[] = "42424242";
const char TDLIB_API_HASH[] = "1234567890abcdef1234567890abcdef";
#endif // TDLIBSECRETS_H
```

The GitHub Actions workflow in `.github/workflows` builds the RPMs too. It expects the API ID
and hash as repository secrets named `TDLIB_API_ID` and `TDLIB_API_HASH`.

## Debug
By default GTgram only prints a few TDLib messages. To see its own debug output, run a debug
build or filter with `QT_LOGGING_RULES`:

    QT_LOGGING_RULES="gtgram.*=true" harbour-gtgram

Replace `*` with a specific logging category (each `.cpp` file declares its own), or use `JS`
for frontend messages only. Append ` &> gtgram.log` to write the output to a file.

**Debug output will almost certainly contain personal information**, including chat content
and the user IDs and names of you and your chat partners. Don't share it publicly.

## Credits
GTgram is based on [Fernschreiber](https://github.com/Wunderfitz/harbour-fernschreiber) by
Sebastian J. Wolf and [other contributors](https://github.com/Wunderfitz/harbour-fernschreiber#contributions),
including its translators. Thank you for building it.

It also uses:

- The Telegram Database Library (TDLib), available on [GitHub](https://github.com/tdlib/td) under the Boost Software License 1.0 ([license](https://github.com/tdlib/td/blob/master/LICENSE_1_0.txt)).
- Emoji parsing and artwork by [Twemoji](https://github.com/jdecked/twemoji), copyright 2026 by Twemoji contributors; code under the [MIT License](http://opensource.org/licenses/MIT), graphics under [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- Animated stickers by [rlottie](https://github.com/Samsung/rlottie), copyright 2020 Samsung Electronics Co., Ltd. and [other contributors](https://github.com/Samsung/rlottie/blob/master/AUTHORS), under the [MIT License](https://github.com/Samsung/rlottie/blob/master/licenses/COPYING.MIT), with some components under [other licenses](https://github.com/Samsung/rlottie/blob/master/COPYING).
- Reverse geocoding for location attachments by [OpenStreetMap Nominatim](https://wiki.openstreetmap.org/wiki/Nominatim).
