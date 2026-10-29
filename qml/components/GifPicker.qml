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
import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

// A panel with the user's saved GIFs and a search through Telegram's @gif bot.
// Tapping a GIF emits gifPicked with what is needed to send it.
Item {
    id: gifPickerOverlayItem
    anchors.fill: parent

    // The chat the GIF will be sent to; the inline bot query needs it.
    property string chatId
    readonly property string botUserName: "gif"

    property var savedGifs: []
    property var foundGifs: []
    property var gifBotInformation: null
    property string nextOffset: ""
    property bool searchLoading: false
    readonly property bool searching: gifSearchField.text.trim().length > 0
    // Query that the shown search results belong to, to drop late answers
    property string requestedQuery: ""
    property string inlineQueryId: ""

    // Either {kind: "saved", fileId} or {kind: "search", queryId, resultId}
    signal gifPicked(var gif)

    Component.onCompleted: {
        tdLibWrapper.getSavedAnimations();
    }

    function startSearch() {
        var query = gifSearchField.text.trim();
        gifPickerOverlayItem.foundGifs = [];
        gifPickerOverlayItem.nextOffset = "";
        if (query.length === 0) {
            gifPickerOverlayItem.searchLoading = false;
            return;
        }
        gifPickerOverlayItem.searchLoading = true;
        gifPickerOverlayItem.requestedQuery = query;
        if (gifBotInformation) {
            requestResults("");
        } else {
            // Find the @gif bot first; the reply arrives in onChatReceived below
            tdLibWrapper.searchPublicChat(botUserName, false);
        }
    }

    function requestResults(offset) {
        tdLibWrapper.getInlineQueryResults(gifBotInformation.id, chatId, {}, gifPickerOverlayItem.requestedQuery, offset,
            "gifPicker|" + gifPickerOverlayItem.requestedQuery + "|" + offset);
    }

    Connections {
        target: tdLibWrapper
        onSavedAnimationsReceived: {
            gifPickerOverlayItem.savedGifs = animations;
        }
        onChatReceived: {
            if (chat["@extra"] !== "searchPublicChat:" + gifPickerOverlayItem.botUserName) {
                return;
            }
            var botInformation = chat.type ? tdLibWrapper.getUserInformation(chat.type.user_id) : null;
            if (botInformation && botInformation.type && botInformation.type["@type"] === "userTypeBot" && botInformation.type.is_inline) {
                gifPickerOverlayItem.gifBotInformation = botInformation;
                if (gifPickerOverlayItem.searching) {
                    gifPickerOverlayItem.requestResults("");
                }
            } else {
                gifPickerOverlayItem.searchLoading = false;
            }
        }
        onInlineQueryResults: {
            if (extra.indexOf("gifPicker|" + gifPickerOverlayItem.requestedQuery + "|") !== 0) {
                return;
            }
            gifPickerOverlayItem.searchLoading = false;
            var gifs = [];
            for (var i = 0; i < results.length; i++) {
                if (results[i]["@type"] === "inlineQueryResultAnimation") {
                    gifs.push({
                        animation: results[i].animation,
                        queryId: inlineQueryId,
                        resultId: results[i].id
                    });
                }
            }
            gifPickerOverlayItem.inlineQueryId = inlineQueryId;
            // Keep the query id on every entry of this answer
            for (var j = 0; j < gifs.length; j++) {
                gifs[j].queryId = inlineQueryId;
            }
            var offsetPart = extra.substring(("gifPicker|" + gifPickerOverlayItem.requestedQuery + "|").length);
            gifPickerOverlayItem.foundGifs = (offsetPart === "") ? gifs : gifPickerOverlayItem.foundGifs.concat(gifs);
            gifPickerOverlayItem.nextOffset = nextOffset;
        }
    }

    Timer {
        id: searchDelay
        interval: 700
        onTriggered: gifPickerOverlayItem.startSearch()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.overlayBackgroundColor
        opacity: 0.7
    }

    SearchField {
        id: gifSearchField
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        placeholderText: qsTr("Search GIFs...")
        EnterKey.iconSource: "image://theme/icon-m-enter-close"
        EnterKey.onClicked: focus = false
        onTextChanged: searchDelay.restart()
    }

    SilicaGridView {
        id: gifGridView
        anchors {
            top: gifSearchField.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        clip: true
        cellWidth: width / (gifPickerOverlayItem.width > gifPickerOverlayItem.height ? 4 : 3)
        cellHeight: cellWidth * 0.75

        model: gifPickerOverlayItem.searching ? gifPickerOverlayItem.foundGifs : gifPickerOverlayItem.savedGifs

        delegate: BackgroundItem {
            id: gifItem
            width: gifGridView.cellWidth
            height: gifGridView.cellHeight
            // Saved GIFs are animation objects; search hits wrap the animation
            readonly property var animationObject: gifPickerOverlayItem.searching ? modelData.animation : modelData

            onClicked: {
                if (gifPickerOverlayItem.searching) {
                    gifPickerOverlayItem.gifPicked({ kind: "search", queryId: modelData.queryId, resultId: modelData.resultId });
                } else {
                    gifPickerOverlayItem.gifPicked({ kind: "saved", fileId: modelData.animation.id });
                }
            }

            TDLibThumbnail {
                anchors {
                    fill: parent
                    margins: Theme.paddingSmall / 2
                }
                thumbnail: gifItem.animationObject.thumbnail
                minithumbnail: gifItem.animationObject.minithumbnail
                fillMode: Image.PreserveAspectCrop
                highlighted: gifItem.down
            }
        }

        // Ask for the next page of search results when the end is reached
        onAtYEndChanged: {
            if (atYEnd && gifPickerOverlayItem.searching && gifPickerOverlayItem.nextOffset !== "" && !gifPickerOverlayItem.searchLoading && gifPickerOverlayItem.gifBotInformation) {
                gifPickerOverlayItem.searchLoading = true;
                gifPickerOverlayItem.requestResults(gifPickerOverlayItem.nextOffset);
            }
        }

        ViewPlaceholder {
            enabled: gifGridView.count === 0 && !gifPickerOverlayItem.searchLoading
            text: gifPickerOverlayItem.searching ? qsTr("No GIFs found") : qsTr("You have no saved GIFs")
            hintText: gifPickerOverlayItem.searching ? "" : qsTr("Search above to find some")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Medium
            running: gifPickerOverlayItem.searchLoading
        }

        VerticalScrollDecorator {}
    }
}
