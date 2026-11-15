/*
    Copyright (C) 2026 Guillermo Torres and other contributors

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
import QtQuick 2.6
import Sailfish.Silica 1.0
import "../js/emojidata.js" as EmojiData

// Emoji keyboard for the message input. Tapping an emoji emits emojiPicked;
// the chat page inserts it at the cursor. Recently used emojis are kept in
// appSettings.recentEmojis (newline-separated "emoji\tfile" pairs).
Item {
    id: emojiPickerItem
    anchors.fill: parent

    signal emojiPicked(string emoji)

    readonly property int maxRecentEmojis: 32
    // -1 = recently used, otherwise an index into EmojiData.groups
    property int currentGroup: 0
    property var recentEmojis: parseRecent(appSettings.recentEmojis)

    function parseRecent(stored) {
        var result = [];
        if (!stored) {
            return result;
        }
        var entries = stored.split("\n");
        for (var i = 0; i < entries.length; i++) {
            var parts = entries[i].split("\t");
            if (parts.length >= 2 && parts[0] !== "") {
                result.push([parts[0], parts[1]]);
            }
        }
        return result;
    }

    function pick(emoji, fileName) {
        var updated = [[emoji, fileName]];
        for (var i = 0; i < recentEmojis.length && updated.length < maxRecentEmojis; i++) {
            if (recentEmojis[i][0] !== emoji) {
                updated.push(recentEmojis[i]);
            }
        }
        var stored = [];
        for (var j = 0; j < updated.length; j++) {
            stored.push(updated[j][0] + "\t" + updated[j][1]);
        }
        // Keep the visible list as it is while the picker is open, so the
        // emoji under the finger doesn't jump around in the recent tab.
        appSettings.recentEmojis = stored.join("\n");
        emojiPicked(emoji);
    }

    property var frozenRecent: []
    Component.onCompleted: {
        frozenRecent = recentEmojis;
        if (frozenRecent.length > 0) {
            currentGroup = -1;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.overlayBackgroundColor
        opacity: 0.85
    }

    SilicaFlickable {
        id: groupTabs
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: Theme.itemSizeSmall
        contentWidth: groupTabRow.width
        flickableDirection: Flickable.HorizontalFlick
        clip: true

        Row {
            id: groupTabRow
            height: parent.height

            BackgroundItem {
                visible: emojiPickerItem.frozenRecent.length > 0
                width: Theme.itemSizeSmall
                height: groupTabs.height
                highlighted: down || emojiPickerItem.currentGroup === -1
                onClicked: {
                    emojiPickerItem.currentGroup = -1;
                    emojiGrid.positionViewAtBeginning();
                }
                Icon {
                    anchors.centerIn: parent
                    source: "image://theme/icon-m-clock"
                    highlighted: parent.highlighted
                }
            }

            Repeater {
                model: EmojiData.groups.length
                BackgroundItem {
                    width: Theme.itemSizeSmall
                    height: groupTabs.height
                    highlighted: down || emojiPickerItem.currentGroup === index
                    onClicked: {
                        emojiPickerItem.currentGroup = index;
                        emojiGrid.positionViewAtBeginning();
                    }
                    Image {
                        anchors.centerIn: parent
                        width: Theme.iconSizeSmallPlus
                        height: Theme.iconSizeSmallPlus
                        sourceSize.width: width
                        sourceSize.height: height
                        source: "../js/emoji/" + EmojiData.groups[index].emojis[0][1] + ".svg"
                        asynchronous: true
                        opacity: parent.highlighted ? 1 : 0.6
                    }
                }
            }
        }
    }

    Separator {
        id: tabSeparator
        anchors.top: groupTabs.bottom
        width: parent.width
        color: Theme.primaryColor
        horizontalAlignment: Qt.AlignHCenter
    }

    SilicaGridView {
        id: emojiGrid
        anchors {
            top: tabSeparator.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        // Fit a whole number of cells into the width
        cellWidth: width / Math.max(1, Math.floor(width / Theme.itemSizeSmall))
        cellHeight: cellWidth
        model: emojiPickerItem.currentGroup === -1 ? emojiPickerItem.frozenRecent : EmojiData.groups[emojiPickerItem.currentGroup].emojis

        delegate: BackgroundItem {
            width: emojiGrid.cellWidth
            height: emojiGrid.cellHeight
            onClicked: emojiPickerItem.pick(modelData[0], modelData[1])
            Image {
                anchors.centerIn: parent
                width: Theme.iconSizeMedium
                height: Theme.iconSizeMedium
                sourceSize.width: width
                sourceSize.height: height
                source: "../js/emoji/" + modelData[1] + ".svg"
                asynchronous: true
            }
        }

        VerticalScrollDecorator {}
    }
}
