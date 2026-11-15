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
import "../../js/twemoji.js" as Emoji
import "../../js/functions.js" as Functions

// Messages of the chat that contain links, newest first
ChatInformationTabItemBase {
    id: tabBase
    loadingText: qsTr("Loading links…")
    loading: shared.loading
    loadingVisible: shared.loading && shared.count === 0

    onActiveChanged: if (active) shared.maybeFetchMore(linksView)

    // All links in a message: plain URLs, links behind text, and the link preview's URL
    function getLinks(message) {
        var links = [];
        var content = message.content || {};
        var formattedText = content.text || content.caption;
        if (formattedText && formattedText.entities) {
            for (var i = 0; i < formattedText.entities.length; i++) {
                var entity = formattedText.entities[i];
                var link = "";
                if (entity.type["@type"] === "textEntityTypeUrl") {
                    link = formattedText.text.substr(entity.offset, entity.length);
                } else if (entity.type["@type"] === "textEntityTypeTextUrl") {
                    link = entity.type.url;
                }
                if (link && links.indexOf(link) === -1) {
                    links.push(link);
                }
            }
        }
        var linkPreview = Functions.getLinkPreview(content);
        if (links.length === 0 && linkPreview && linkPreview.url) {
            links.push(linkPreview.url);
        }
        return links;
    }

    function getDomain(link) {
        var match = /^(?:[a-z][a-z0-9+.-]*:\/\/)?(?:www\.)?([^\/?#:]+)/i.exec(link || "");
        return match ? match[1] : (link || "");
    }

    function getPlainText(value) {
        // TDLib turned the link preview description from a string into a formattedText
        if (!value) {
            return "";
        }
        return typeof value === "string" ? value : (value.text || "");
    }

    ChatInformationSharedMessages {
        id: shared
        filter: "searchMessagesFilterUrl"
        onLoadingChanged: if (!loading) fillTimer.start()
    }
    Timer {
        id: fillTimer
        interval: 100
        onTriggered: if (tabBase.active) shared.maybeFetchMore(linksView)
    }

    SilicaListView {
        id: linksView
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
        onAtYEndChanged: if (tabBase.active && atYEnd) shared.maybeFetchMore(linksView)

        delegate: ListItem {
            id: linkItem
            readonly property var message: shared.messages[model.idx]
            readonly property var links: tabBase.getLinks(message)
            readonly property string mainLink: links.length > 0 ? links[0] : ""
            readonly property var linkPreview: Functions.getLinkPreview(message.content)
            readonly property var previewImageFile: linkPreview ? Functions.getLinkPreviewImageFile(linkPreview, Theme.itemSizeMedium) : null
            readonly property string messageText: tabBase.getPlainText(message.content.text || message.content.caption)
            readonly property string titleText: (linkPreview && (linkPreview.title || linkPreview.site_name)) || tabBase.getDomain(mainLink)
            readonly property string descriptionText: {
                var description = linkPreview ? tabBase.getPlainText(linkPreview.description) : "";
                // Don't repeat the bare link when the message is nothing but the link
                var text = messageText.trim() !== mainLink ? messageText : "";
                return description || text;
            }

            width: linksView.width
            contentHeight: Math.max(Theme.itemSizeLarge, labelsColumn.height + 2 * Theme.paddingMedium)

            Item {
                id: thumbnailItem
                width: Theme.itemSizeMedium
                height: Theme.itemSizeMedium
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    top: parent.top
                    topMargin: Theme.paddingMedium
                }

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.paddingSmall
                    color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                    visible: !previewImage.visible
                    Label {
                        anchors.centerIn: parent
                        text: tabBase.getDomain(linkItem.mainLink).charAt(0).toUpperCase()
                        font.pixelSize: Theme.fontSizeLarge
                        color: linkItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                }
                TDLibImage {
                    id: previewImage
                    anchors.fill: parent
                    fileInformation: linkItem.previewImageFile || {}
                    highlighted: linkItem.highlighted
                }
            }

            Column {
                id: labelsColumn
                anchors {
                    left: thumbnailItem.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: Emoji.emojify(Functions.enhanceHtmlEntities(linkItem.titleText), font.pixelSize)
                    textFormat: Text.StyledText
                    font.pixelSize: Theme.fontSizeSmall
                    truncationMode: TruncationMode.Fade
                    color: linkItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    visible: text.length > 0
                    text: Emoji.emojify(Functions.enhanceHtmlEntities(linkItem.descriptionText.replace(/\s+/g, " ")), font.pixelSize)
                    textFormat: Text.StyledText
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: linkItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
                Label {
                    width: parent.width
                    text: linkItem.mainLink + (linkItem.links.length > 1 ? "  +" + (linkItem.links.length - 1) : "")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    color: Theme.highlightColor
                }
                Label {
                    width: parent.width
                    text: Format.formatDate(new Date(linkItem.message.date * 1000), Formatter.DateMedium)
                    font.pixelSize: Theme.fontSizeTiny
                    color: linkItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }

            onClicked: {
                if (mainLink) {
                    Functions.handleLink(mainLink);
                }
            }

            menu: Component {
                ContextMenu {
                    Repeater {
                        // the first link is what a tap opens anyway
                        model: linkItem.links.length > 1 ? linkItem.links : []
                        MenuItem {
                            text: modelData
                            truncationMode: TruncationMode.Fade
                            onClicked: Functions.handleLink(modelData)
                        }
                    }
                    MenuItem {
                        text: qsTr("Copy link")
                        visible: linkItem.mainLink !== ""
                        onClicked: {
                            Clipboard.text = linkItem.mainLink;
                            appNotification.show(qsTr("Link copied to clipboard."));
                        }
                    }
                    MenuItem {
                        visible: !!shared.chatPage
                        text: qsTr("Show in chat")
                        onClicked: shared.showInChat(linkItem.message.id)
                    }
                }
            }
        }

        footer: Item {
            width: linksView.width
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
            text: qsTr("No links shared in this chat yet.")
        }

        VerticalScrollDecorator {}
    }

    Component.onCompleted: if (active) shared.fetchMore()
}
