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
*/

// Tokens of the optional "Iris" theme: indigo-violet and white, with sand as
// the accent for unread/attention marks, all set in JetBrains Mono.
//
// Like the system-independent theme it is architecturally modelled on
// (RooTelegram's "Barbara"), Iris carries its OWN light and dark palette with
// the same semantic roles and picks one from Theme.colorScheme, so contrast
// holds with both light and dark ambiences.
//
// Singleton declared in qml/components/qmldir. Pages see it through
// `import "../components"`; files inside this folder need `import "."`
// (a directory's implicit import does not carry singletons).

pragma Singleton
import QtQuick 2.6
import Sailfish.Silica 1.0
import WerkWolf.GTgram 1.0

QtObject {
    id: iris

    // The only question components need to ask.
    readonly property bool active: appSettings.appTheme === AppSettings.ThemeIris

    // LightOnDark = light text on dark background = dark ambience.
    readonly property bool dark: Theme.colorScheme === Theme.LightOnDark

    // --- Colours ------------------------------------------------------------
    readonly property color surface:      dark ? "#12101c" : "#f6f5fc"   // page background
    readonly property color panel:        dark ? "#1a1729" : "#ffffff"   // header, composer, opaque panels
    readonly property color glass:        dark ? Qt.rgba(0.20, 0.17, 0.36, 0.55)
                                               : Qt.rgba(1, 1, 1, 0.92)  // cards, other's bubble, chips
    readonly property color glassBorder:  dark ? "#34305a" : "#dcd8f2"
    readonly property color accent:       dark ? "#8a7cff" : "#5a4fd6"   // blue-violet
    readonly property color onAccent:     dark ? "#0f0c24" : "#ffffff"
    readonly property color attention:    dark ? "#d8b987" : "#8f6a2e"   // sand: unread bar and badges
    readonly property color onAttention:  dark ? "#1b1405" : "#ffffff"
    readonly property color ink:          dark ? "#f1eefc" : "#17142e"   // titles and message text
    readonly property color inkSecondary: dark ? "#a9a3d1" : "#5b5780"   // previews, times, captions
    readonly property color danger:       dark ? "#ff7b8a" : "#b3243a"
    readonly property color grid:         dark ? "#1e1b33" : "#e6e3f6"   // background lattice

    readonly property color panelTranslucent: dark ? Qt.rgba(0.102, 0.090, 0.161, 0.88)
                                                   : Qt.rgba(1, 1, 1, 0.92)
    readonly property color accentBorder: dark ? "#4a4396" : "#b4adf0"
    readonly property color bubbleOwn:    dark ? Qt.rgba(0.541, 0.486, 1.0, 0.20)
                                               : Qt.rgba(0.353, 0.310, 0.839, 0.12)
    readonly property color bubbleOther:  dark ? Qt.rgba(0.20, 0.17, 0.36, 0.60)
                                               : Qt.rgba(1, 1, 1, 0.94)
    readonly property color accentWash:   Theme.rgba(accent, dark ? 0.12 : 0.08)

    // --- Typography ---------------------------------------------------------
    // JetBrains Mono is bundled (qml/fonts/): it is not on Sailfish OS devices.
    // All four faces are registered under the same family so font.bold /
    // font.italic pick the right one.
    property var _regular: FontLoader { source: Qt.resolvedUrl("../fonts/JetBrainsMono-Regular.ttf") }
    property var _bold: FontLoader { source: Qt.resolvedUrl("../fonts/JetBrainsMono-Bold.ttf") }
    property var _italic: FontLoader { source: Qt.resolvedUrl("../fonts/JetBrainsMono-Italic.ttf") }
    property var _boldItalic: FontLoader { source: Qt.resolvedUrl("../fonts/JetBrainsMono-BoldItalic.ttf") }

    readonly property string fontFamily: _regular.status === FontLoader.Ready ? _regular.name : "monospace"

    // --- Shape --------------------------------------------------------------
    readonly property real radiusCard:   Math.round(Theme.paddingMedium * 0.6)
    readonly property real radiusBubble: Math.round(Theme.paddingMedium * 0.8)
    readonly property real borderWidth:  1
    readonly property real unreadBarWidth: Math.max(3, Math.round(Theme.paddingSmall / 2))

    readonly property int pressDuration: 150
}
