/*
    Copyright (C) 2026 RootGPT, adapted for GTgram

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

// Step-by-step instructions for notifications through UnifiedPush/Foghorn (BETA).
// One block of text per section keeps the number of strings to translate small.

import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: unifiedPushHelpPage
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height

        VerticalScrollDecorator {}

        Column {
            id: contentColumn
            width: parent.width
            bottomPadding: Theme.paddingLarge
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("UnifiedPush (BETA)")
            }

            Repeater {
                model: [
                    { header: qsTr("What it is"),
                      body: qsTr("Normally GTgram only shows notifications while it is running. With this option a small separate app, Foghorn, wakes GTgram up when a new message arrives, even if GTgram is closed. GTgram then starts without a window, downloads the message, shows the notification and closes again by itself. A notification may arrive a few seconds later than usual.<br><br><b>This is a BETA feature</b>, and Foghorn itself is young software: if notifications stop arriving, turn the option off.") },
                    { header: qsTr("1. Install Foghorn"),
                      body: qsTr("Install <b>Foghorn</b> from OpenRepos (for example with the Storeman app): <a href=\"https://openrepos.net/content/projectmoon/foghorn\">openrepos.net/content/projectmoon/foghorn</a><br>Open Foghorn once and check that its service is running.") },
                    { header: qsTr("2. Leave Foghorn on Mozilla"),
                      body: qsTr("Foghorn's default push service, <b>Mozilla</b>, works well with Telegram: you don't have to change anything.<br><br><b>Don't choose ntfy:</b> in tests by RooTelegram, Telegram almost never delivered its wake-ups through the public ntfy.sh server. If you had changed it, go back to Mozilla in Foghorn's settings: GTgram notices it and registers again by itself.") },
                    { header: qsTr("3. Open GTgram from its icon"),
                      body: qsTr("After installing or updating GTgram, open it once by tapping its icon. SailfishOS may ask you to allow the app's permissions, including the new \"UnifiedPush\" one: accept. Without this step Foghorn cannot start GTgram in the background.") },
                    { header: qsTr("4. Turn on the option"),
                      body: qsTr("In GTgram go to Settings, Behavior, and turn on <b>Notifications via UnifiedPush (BETA)</b>. After a few seconds the status below the switch should say that it is active and show the name of the server (for example updates.push.services.mozilla.com).") },
                    { header: qsTr("5. Try it"),
                      body: qsTr("Close GTgram completely (close it from the home screen, don't just minimize it). Ask someone to write to you: the notification should appear within a few seconds.<br><br>If Telegram is open on another device (another phone, a computer), Telegram itself waits a little before waking up this phone: then the notification may arrive up to about half a minute later. That is normal.") },
                    { header: qsTr("How it behaves"),
                      body: qsTr("• While the app window is open, or minimized, GTgram works as usual.<br>• When a message arrives with the app closed, GTgram starts without a window for 20-60 seconds, shows the notifications and closes.<br>• Tapping a notification while the app is closed starts GTgram from scratch, so the chat takes a few seconds to open.") },
                    { header: qsTr("If it doesn't work"),
                      body: qsTr("• Status \"no distributor\": Foghorn is not installed or its service is stopped.<br>• Status \"error\": open GTgram from its icon (step 3), then use \"Try again\".<br>• No notifications although the status is active: check that Foghorn uses Mozilla (step 2), that GTgram is listed among Foghorn's registered apps, and the Internet connection.<br>• When in doubt: turn the option off. Everything goes back to how it was before.") },
                    { header: qsTr("Privacy"),
                      body: qsTr("The wake-up that Telegram sends through the push service is encrypted, and GTgram doesn't even read it: it uses it only as a signal, then downloads the messages directly from Telegram, encrypted as usual. Telegram and Mozilla only learn the address where to deliver the wake-ups: whoever learned it could only wake GTgram up, not read anything.") },
                    { header: qsTr("Turning it off"),
                      body: qsTr("Turn the switch off: GTgram tells Telegram to forget the address and unregisters from Foghorn.") },
                    { header: qsTr("Thanks"),
                      body: qsTr("Thanks to <b>projectmoon</b>, the developer of Foghorn, for bringing UnifiedPush to SailfishOS, and to <b>RootGPT</b>, whose RooTelegram implementation this is based on.") }
                ]
                delegate: Column {
                    width: contentColumn.width
                    spacing: Theme.paddingSmall
                    SectionHeader {
                        text: modelData.header
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        textFormat: Text.StyledText
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.highlightColor
                        linkColor: Theme.primaryColor
                        text: modelData.body
                        onLinkActivated: Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }
}
