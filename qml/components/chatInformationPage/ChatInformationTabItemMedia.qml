/*
    Copyright (C) 2026 GTgram contributors

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

import "../"
import "../../pages"

// Shared photos and videos of the chat, as a grid
ChatInformationTabItemBase {
    id: tabBase
    loadingText: qsTr("Loading media…")
    loading: shared.loading
    loadingVisible: shared.loading && shared.count === 0

    onActiveChanged: if (active) shared.maybeFetchMore(mediaView)

    ChatInformationSharedMessages {
        id: shared
        filter: "searchMessagesFilterPhotoAndVideo"
        onLoadingChanged: if (!loading) fillTimer.start()
    }
    Timer {
        // after a page arrived and the grid laid it out
        id: fillTimer
        interval: 100
        onTriggered: if (tabBase.active) shared.maybeFetchMore(mediaView)
    }

    SilicaGridView {
        id: mediaView
        model: shared.model
        clip: true
        height: tabBase.height
        width: tabBase.width
        readonly property int columns: Math.max(3, Math.round(width / (Screen.width / 3)))
        cellWidth: Math.floor(width / columns)
        cellHeight: cellWidth

        function handleScrollIntoView(force) {
            if (!dragging && !quickScrollAnimating) {
                if (!atYBeginning) {
                    pageContent.scrollDown()
                } else {
                    pageContent.scrollUp(force);
                }
            }
        }
        onDraggingChanged: handleScrollIntoView()
        onAtYBeginningChanged: handleScrollIntoView()
        onQuickScrollAnimatingChanged: handleScrollIntoView(true)
        onAtYEndChanged: if (tabBase.active && atYEnd) shared.maybeFetchMore(mediaView)

        delegate: BackgroundItem {
            id: tile
            width: mediaView.cellWidth
            height: mediaView.cellHeight
            readonly property var message: shared.messages[model.idx]
            readonly property bool isVideo: !!message && message.content["@type"] === "messageVideo"

            Loader {
                anchors {
                    fill: parent
                    margins: Theme.paddingSmall / 2
                }
                asynchronous: true
                sourceComponent: tile.isVideo ? videoComponent : photoComponent
            }
            Component {
                id: photoComponent
                TDLibPhoto {
                    photo: tile.message.content.photo
                    highlighted: tile.highlighted
                }
            }
            Component {
                id: videoComponent
                Item {
                    TDLibThumbnail {
                        anchors.fill: parent
                        thumbnail: tile.message.content.video.thumbnail
                        minithumbnail: tile.message.content.video.minithumbnail
                        highlighted: tile.highlighted
                    }
                    Rectangle {
                        anchors.fill: durationLabel
                        anchors.margins: -Theme.paddingSmall / 2
                        radius: Theme.paddingSmall
                        color: Theme.rgba("black", 0.5)
                    }
                    Label {
                        id: durationLabel
                        anchors {
                            right: parent.right
                            bottom: parent.bottom
                            margins: Theme.paddingSmall
                        }
                        font.pixelSize: Theme.fontSizeTiny
                        color: "white"
                        text: "▶ " + Format.formatDuration(tile.message.content.video.duration,
                                                           tile.message.content.video.duration >= 3600 ? Formatter.DurationLong : Formatter.DurationShort)
                    }
                }
            }

            onClicked: {
                pageStack.push(Qt.resolvedUrl("../../pages/MediaAlbumPage.qml"), {
                                   "messages": shared.messages.slice(),
                                   "index": model.idx
                               })
            }
        }

        footer: Item {
            width: mediaView.width
            height: shared.loading && shared.count > 0 ? Theme.itemSizeLarge : Theme.paddingMedium
            BusyIndicator {
                anchors.centerIn: parent
                size: BusyIndicatorSize.Small
                running: shared.loading && shared.count > 0
            }
        }

        ViewPlaceholder {
            y: Theme.paddingLarge
            enabled: shared.complete && shared.count === 0
            text: qsTr("No photos or videos shared in this chat yet.")
        }

        VerticalScrollDecorator {}
    }

    Component.onCompleted: if (active) shared.fetchMore()
}
