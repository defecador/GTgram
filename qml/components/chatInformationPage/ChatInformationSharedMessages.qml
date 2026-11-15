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

import "../../js/debug.js" as Debug

// Pages through one kind of shared content of the chat (media, links or files)
// with TDLib's searchChatMessages, oldest pages fetched as the user scrolls.
// The messages are kept in a plain array; the ListModel only holds their indexes,
// so that nested TDLib objects (photo sizes etc.) stay plain JS objects.
Item {
    id: fetcher
    // A TDLib SearchMessagesFilter type, e.g. "searchMessagesFilterPhotoAndVideo"
    property string filter
    property int pageSize: 50

    property var messages: []
    readonly property alias model: indexModel
    readonly property alias count: indexModel.count
    property bool loading
    property bool complete
    property bool started
    property var nextFromMessageId: 0
    property var knownIds: ({})

    readonly property var chatId: chatInformationPage.chatInformation.id
    // The chat information page is attached to the chat itself, so we can jump back into it
    readonly property var chatPage: {
        var previous = pageStack.previousPage(chatInformationPage);
        return (previous && typeof previous.showMessage === "function" && previous.chatInformation
                && String(previous.chatInformation.id) === String(chatId)) ? previous : null;
    }

    function fetchMore() {
        if (loading || complete || !chatId) {
            return;
        }
        loading = true;
        started = true;
        tdLibWrapper.searchChatSharedMessages(chatId, filter, nextFromMessageId, pageSize);
    }

    // Fetch the next page once the view is scrolled (close) to its end, or isn't even full yet
    function maybeFetchMore(view) {
        if (!loading && !complete && (view.atYEnd || view.contentHeight < view.height * 1.5)) {
            fetchMore();
        }
    }

    function showInChat(messageId) {
        if (chatPage) {
            var targetPage = chatPage;
            pageStack.navigateBack();
            targetPage.showMessage(String(messageId), true);
        }
    }

    ListModel {
        id: indexModel
    }

    Connections {
        target: tdLibWrapper
        onSharedMessagesReceived: {
            if (filter !== fetcher.filter || String(chatId) !== String(fetcher.chatId)) {
                return;
            }
            var added = 0;
            for (var i = 0; i < messages.length; i++) {
                var message = messages[i];
                if (fetcher.knownIds[message.id]) {
                    continue;
                }
                fetcher.knownIds[message.id] = true;
                fetcher.messages.push(message);
                indexModel.append({ idx: fetcher.messages.length - 1 });
                added++;
            }
            Debug.log("[SharedMessages]", fetcher.filter, "added", added, "of", totalCount, "next from", nextFromMessageId);
            fetcher.nextFromMessageId = nextFromMessageId;
            if (added === 0 || !nextFromMessageId || (totalCount > 0 && indexModel.count >= totalCount)) {
                fetcher.complete = true;
            }
            fetcher.loading = false;
        }
    }
}
