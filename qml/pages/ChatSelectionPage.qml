/*
    Copyright (C) 2020 Sebastian J. Wolf and other contributors

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
import WerkWolf.GTgram 1.0
import "../components"

import "../js/twemoji.js" as Emoji
import "../js/functions.js" as Functions

Dialog {
    id: chatSelectionPage
    allowedOrientations: Orientation.All
    canAccept: false
    acceptDestinationAction: PageStackAction.Replace
    acceptDestinationReplaceTarget: pageStack.find( function(page){
        // This crazy workaround is presented to you by a bug introduced with SFOS 4.0.1
        // See https://forum.sailfishos.org/t/4-0-1-45-pagestack-find-not-working-properly-anymore-in-a-dialog/4723 for details.
        chatSelectionPage.currentDepth = chatSelectionPage.currentDepth - 1;
        return(chatSelectionPage.currentDepth === 0);
    } )
    property int myUserId: tdLibWrapper.getUserInformation().id
    property alias headerTitle: pageHeader.title
    property alias headerDescription: pageHeader.description

    property var currentDepth: pageStack.depth

    /*
        payload dependent on chatSelectionPage.state
         - forwardMessages: {fromChatId, messageIds, neededPermissions, silently}
         - shareResources: {resources, neededPermissions}
    */
    property var payload: ({})

    onAccepted: {
        switch(chatSelectionPage.state) {
        case "forwardMessages":
            acceptDestinationInstance.forwardMessages(payload.fromChatId, payload.messageIds, payload.silently)
            break;
        case "fillTextArea": // ReplyMarkupButtons: inlineKeyboardButtonTypeSwitchInline
            acceptDestinationInstance.setMessageText(payload.text)
            break;
        // shareResources: nothing more to do here, pendingSharedResources
        // was already handed to ChatPage via acceptDestinationProperties
        // below and it picks that up once active.
        // future uses of chat selection can be processed here
        }
    }

    PageHeader {
        id: pageHeader
        title: qsTr("Select Chat")
        width: parent.width
    }

    SearchField {
        id: chatSearchField
        anchors.top: pageHeader.bottom
        width: parent.width
        placeholderText: qsTr("Search chats...")

        EnterKey.iconSource: "image://theme/icon-m-enter-close"
        EnterKey.onClicked: focus = false
    }

    ChatPermissionFilterModel {
        id: permittedChatsModel
        tdlib: tdLibWrapper
        sourceModel: chatListModel
        requirePermissions: chatSelectionPage.payload.neededPermissions
    }

    TextFilterModel {
        id: searchedChatsModel
        // Only route through this proxy while actually searching, so the
        // unfiltered list keeps the view's cheap native row-move handling
        // (see the note on chatCategoryModel in OverviewPage.qml).
        sourceModel: chatSearchField.text === "" ? null : permittedChatsModel
        filterRoleName: "title"
        filterText: chatSearchField.text
    }

    SilicaListView {
        id: chatListView

        anchors {
            top: chatSearchField.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        clip: true

        model: chatSearchField.text === "" ? permittedChatsModel : searchedChatsModel

        move: Transition {
            NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.InOutCubic }
        }
        moveDisplaced: Transition {
            NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.InOutCubic }
        }

        delegate: ChatListViewItem {
            ownUserId: chatSelectionPage.myUserId
            onClicked: {
                var chat = tdLibWrapper.getChat(display.id);
                switch(chatSelectionPage.state) {
                case "forwardMessages":
                case "fillTextArea":
                    chatSelectionPage.acceptDestinationProperties = { "chatInformation" :  chat};
                    chatSelectionPage.acceptDestination = Qt.resolvedUrl("../pages/ChatPage.qml");
                    break;
                case "shareResources":
                    chatSelectionPage.acceptDestinationProperties = {
                        "chatInformation" : chat,
                        "pendingSharedResources" : payload.resources
                    };
                    chatSelectionPage.acceptDestination = Qt.resolvedUrl("../pages/ChatPage.qml");
                    break;
                }
                chatSelectionPage.canAccept = true;
                chatSelectionPage.accept();
            }
        }

        ViewPlaceholder {
            enabled: chatListView.count === 0
            text: chatSearchField.text === "" ? qsTr("You don't have any chats yet.") : qsTr("No chats found.")
        }

        VerticalScrollDecorator {}
    }
}
