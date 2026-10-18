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

// Background of the Iris theme: the theme's own surface plus a faint lattice,
// so contrast does not depend on the system ambience. Put it as the first
// child of a page. It is a Loader, so nothing is instantiated while another
// theme is active. The lattice is painted once on a Canvas (no blur effects).

import QtQuick 2.6
import Sailfish.Silica 1.0
import "."

Loader {
    z: -1
    anchors.fill: parent
    active: IrisTheme.active

    sourceComponent: Item {
        Rectangle {
            anchors.fill: parent
            color: IrisTheme.surface
        }

        Canvas {
            id: lattice
            anchors.fill: parent
            opacity: 0.6
            renderStrategy: Canvas.Cooperative

            readonly property real step: Math.max(24, Math.round(Screen.width / 9))
            readonly property color lineColor: IrisTheme.grid

            onLineColorChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = lineColor;
                ctx.lineWidth = 1;
                ctx.beginPath();
                var x, y;
                // Half-pixel offset keeps 1px lines crisp instead of smeared over two rows
                for (x = lattice.step; x < width; x += lattice.step) {
                    ctx.moveTo(Math.round(x) + 0.5, 0);
                    ctx.lineTo(Math.round(x) + 0.5, height);
                }
                for (y = lattice.step; y < height; y += lattice.step) {
                    ctx.moveTo(0, Math.round(y) + 0.5);
                    ctx.lineTo(width, Math.round(y) + 0.5);
                }
                ctx.stroke();
            }
        }
    }
}
