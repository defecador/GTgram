/*
    Copyright (C) 2026 Guillermo Torres

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
import "../js/functions.js" as Functions
import "../js/twemoji.js" as Emoji

Page {
    id: scheduledMessagesPage
    allowedOrientations: Orientation.All

    property var chatInformation
    readonly property var chatId: chatInformation.id
    readonly property var myUserId: tdLibWrapper.getUserInformation().id
    property var scheduledMessages: []
    property bool loading: true

    function reload() {
        tdLibWrapper.getChatScheduledMessages(chatId);
    }

    // "Send when online" messages have no date and sort first
    function sendDateOf(message) {
        return (message.scheduling_state && message.scheduling_state.send_date) || 0;
    }

    function forget(messageId) {
        scheduledMessages = scheduledMessages.filter(function(message) { return message.id !== messageId; });
        // TDLib reports deletions in a form QML can't receive, so check back shortly after
        reloadTimer.restart();
    }

    function sendNow(message) {
        tdLibWrapper.editMessageSchedulingState(chatId, message.id, ({}));
        forget(message.id);
    }

    function reschedule(message) {
        var properties = { headerTitle: qsTr("Reschedule message"), acceptButtonText: qsTr("Reschedule") };
        if (sendDateOf(message) > 0) {
            properties.sendDate = new Date(sendDateOf(message) * 1000);
        }
        var messageId = message.id;
        var dialog = pageStack.push(Qt.resolvedUrl("ScheduleMessageDialog.qml"), properties);
        dialog.accepted.connect(function() {
            tdLibWrapper.editMessageSchedulingState(chatId, messageId, { "sendDate": dialog.sendTimestamp });
            reloadTimer.restart();
        });
    }

    function deleteScheduledMessage(messageId) {
        tdLibWrapper.deleteMessages(chatId, [ messageId ]);
        forget(messageId);
    }

    Component.onCompleted: reload()

    Timer {
        id: reloadTimer
        interval: 1000
        onTriggered: scheduledMessagesPage.reload()
    }

    Connections {
        target: tdLibWrapper
        // chatId here is the signal's argument, hence the page's own id is spelled out
        onScheduledMessagesReceived: {
            if (String(chatId) === String(scheduledMessagesPage.chatId)) {
                scheduledMessagesPage.scheduledMessages = messages.slice().sort(function(first, second) {
                    return scheduledMessagesPage.sendDateOf(first) - scheduledMessagesPage.sendDateOf(second);
                });
                scheduledMessagesPage.loading = false;
            }
        }
        onChatHasScheduledMessagesUpdated: {
            if (chatId === String(scheduledMessagesPage.chatId)) {
                scheduledMessagesPage.reload();
            }
        }
    }

    SilicaListView {
        id: scheduledMessagesList
        anchors.fill: parent
        model: scheduledMessagesPage.scheduledMessages

        header: PageHeader {
            title: qsTr("Scheduled messages")
            description: scheduledMessagesPage.chatInformation.title
        }

        ViewPlaceholder {
            enabled: !scheduledMessagesPage.loading && scheduledMessagesList.count === 0
            text: qsTr("No scheduled messages")
            hintText: qsTr("Long-press the send button to schedule one.")
        }

        delegate: ListItem {
            id: scheduledMessageItem
            contentHeight: scheduledMessageColumn.height + ( 2 * Theme.paddingMedium )
            readonly property var schedulingState: modelData.scheduling_state || ({})

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Send now")
                    onClicked: scheduledMessagesPage.sendNow(modelData)
                }
                MenuItem {
                    text: qsTr("Reschedule")
                    onClicked: scheduledMessagesPage.reschedule(modelData)
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        var messageId = modelData.id;
                        Remorse.itemAction(scheduledMessageItem, qsTr("Deleting message"), function() {
                            scheduledMessagesPage.deleteScheduledMessage(messageId);
                        });
                    }
                }
            }

            Column {
                id: scheduledMessageColumn
                x: Theme.horizontalPageMargin
                width: parent.width - ( 2 * Theme.horizontalPageMargin )
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingSmall

                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                    text: scheduledMessageItem.schedulingState["@type"] === "messageSchedulingStateSendWhenOnline"
                          ? qsTr("When online")
                          : Functions.getScheduledDateText(scheduledMessageItem.schedulingState.send_date)
                }

                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    maximumLineCount: 4
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
                    color: scheduledMessageItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: Emoji.emojify(Functions.enhanceHtmlEntities(Functions.getMessageText(modelData, true, scheduledMessagesPage.myUserId, false)), font.pixelSize)
                }
            }
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: scheduledMessagesPage.loading
    }
}
