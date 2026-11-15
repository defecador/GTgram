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
import "../messageContent"
import "../../pages"

// Shared documents of the chat, newest first
ChatInformationTabItemBase {
    id: tabBase
    loadingText: qsTr("Loading files…")
    loading: shared.loading
    loadingVisible: shared.loading && shared.count === 0

    onActiveChanged: if (active) shared.maybeFetchMore(filesView)

    ChatInformationSharedMessages {
        id: shared
        filter: "searchMessagesFilterDocument"
        onLoadingChanged: if (!loading) fillTimer.start()
    }
    Timer {
        id: fillTimer
        interval: 100
        onTriggered: if (tabBase.active) shared.maybeFetchMore(filesView)
    }

    SilicaListView {
        id: filesView
        model: shared.model
        clip: true
        height: tabBase.height
        width: tabBase.width

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
        onAtYEndChanged: if (tabBase.active && atYEnd) shared.maybeFetchMore(filesView)

        delegate: ListItem {
            id: fileItem
            readonly property var message: shared.messages[model.idx]
            width: filesView.width
            contentHeight: documentItem.height + Theme.paddingSmall

            MessageDocument {
                id: documentItem
                rawMessage: fileItem.message
                highlighted: fileItem.highlighted
                tertiaryText: Format.formatDate(new Date(fileItem.message.date * 1000), Formatter.DateMedium)
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
            }

            menu: Component {
                ContextMenu {
                    MenuItem {
                        visible: !!shared.chatPage
                        text: qsTr("Show in chat")
                        onClicked: shared.showInChat(fileItem.message.id)
                    }
                    MenuItem {
                        text: qsTr("Download")
                        visible: !documentItem.file.isDownloadingCompleted && !documentItem.file.isDownloadingActive
                        onClicked: documentItem.file.load()
                    }
                    MenuItem {
                        text: qsTr("Open")
                        visible: documentItem.file.isDownloadingCompleted
                        onClicked: tdLibWrapper.copyFileToDownloads(documentItem.file.path, true)
                    }
                }
            }
        }

        footer: Item {
            width: filesView.width
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
            text: qsTr("No files shared in this chat yet.")
        }

        VerticalScrollDecorator {}
    }

    Component.onCompleted: if (active) shared.fetchMore()
}
