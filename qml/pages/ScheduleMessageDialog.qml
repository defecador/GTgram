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

Dialog {
    id: scheduleDialog
    allowedOrientations: Orientation.All

    property string headerTitle: qsTr("Schedule message")
    property string acceptButtonText: qsTr("Schedule")
    // A JS Date. Starts at the first full hour that is at least half an hour away.
    property var sendDate: {
        var date = new Date(Date.now() + 30 * 60 * 1000);
        date.setHours(date.getHours() + 1, 0, 0, 0);
        return date;
    }
    readonly property int sendTimestamp: Math.floor(sendDate.getTime() / 1000)

    property var currentTime: new Date()
    readonly property bool isInFuture: sendDate.getTime() > currentTime.getTime() + 60 * 1000
    // Telegram rejects anything scheduled further ahead than that
    readonly property bool isWithinAYear: sendDate.getTime() < currentTime.getTime() + 365 * 24 * 60 * 60 * 1000

    canAccept: isInFuture && isWithinAYear

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: scheduleDialog.headerTitle
                acceptText: scheduleDialog.acceptButtonText
            }

            ValueButton {
                label: qsTr("Date")
                value: Format.formatDate(scheduleDialog.sendDate, Formatter.DateLong)
                onClicked: {
                    var picker = pageStack.push("Sailfish.Silica.DatePickerDialog", { date: scheduleDialog.sendDate });
                    picker.accepted.connect(function() {
                        var picked = new Date(scheduleDialog.sendDate.getTime());
                        picked.setFullYear(picker.year, picker.month - 1, picker.day);
                        scheduleDialog.sendDate = picked;
                    });
                }
            }

            ValueButton {
                label: qsTr("Time")
                value: Format.formatDate(scheduleDialog.sendDate, Formatter.TimeValue)
                onClicked: {
                    var picker = pageStack.push("Sailfish.Silica.TimePickerDialog", {
                        hour: scheduleDialog.sendDate.getHours(),
                        minute: scheduleDialog.sendDate.getMinutes()
                    });
                    picker.accepted.connect(function() {
                        var picked = new Date(scheduleDialog.sendDate.getTime());
                        picked.setHours(picker.hour, picker.minute, 0, 0);
                        scheduleDialog.sendDate = picked;
                    });
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - ( 2 * Theme.horizontalPageMargin )
                visible: !scheduleDialog.canAccept
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                text: scheduleDialog.isInFuture ? qsTr("Messages can only be scheduled up to a year ahead.")
                                                : qsTr("Pick a time in the future.")
            }
        }

        VerticalScrollDecorator {}
    }

    Timer {
        // Keeps canAccept honest while the dialog stays open
        interval: 15000
        repeat: true
        running: scheduleDialog.status === PageStatus.Active
        onTriggered: scheduleDialog.currentTime = new Date()
    }
}
