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

    Ported from RooTelegram-SailfishOS (GPLv3), voice-only.
*/
import QtQuick 2.6
import Sailfish.Silica 1.0
import "../js/functions.js" as Functions

// Full-screen call overlay, shown/driven from harbour-gtgram.qml's global
// Connections to tdLibWrapper.onCallUpdated - this component only holds the
// UI state, it doesn't listen to TDLib itself, since it needs to appear
// regardless of which page is on top of the stack.
Rectangle {
    id: callOverlay

    property int callId: 0
    property string callerName: ""
    property var callerPhoto: ({})
    property bool outgoing: false
    property string callState: ""
    property bool muted: false
    property bool speakerOn: false
    property double connectedAt: 0
    property int elapsed: 0
    // The 4 verification emoji TDLib derives from the call's shared encryption
    // key (callStateReady.state.emojis) - if they match what the other side
    // sees, the call hasn't been tampered with.
    property var verifyEmojis: []
    // Locks the page underneath to portrait while the overlay (drawn for
    // portrait only) is up, and restores it when the call ends.
    property var savedOrientations: undefined

    readonly property bool ringingIncoming: callState === "callStatePending" && !outgoing
    readonly property bool connected: callState === "callStateReady"

    function toggleMute() {
        muted = !muted;
        if (typeof callManager !== 'undefined') {
            callManager.setMicrophoneMuted(muted);
        }
    }

    function toggleSpeaker() {
        speakerOn = !speakerOn;
        if (typeof callManager !== 'undefined') {
            callManager.setSpeakerphoneOn(speakerOn);
        }
    }

    function accept() {
        tdLibWrapper.acceptVoiceCall(callOverlay.callId, false);
    }

    function decline() {
        tdLibWrapper.discardVoiceCall(callOverlay.callId, false, 0, false, 0);
        callOverlay.visible = false;
    }

    function endCall() {
        tdLibWrapper.discardVoiceCall(callOverlay.callId, false, 0, false, 0);
        callOverlay.visible = false;
    }

    function statusText() {
        switch (callOverlay.callState) {
        case "callStatePending":
            return callOverlay.outgoing ? qsTr("Calling…") : qsTr("Incoming voice call");
        case "callStateExchangingKeys":
            return qsTr("Exchanging encryption keys…");
        case "callStateReady":
            return callOverlay.formatElapsed(callOverlay.elapsed);
        case "callStateHangingUp":
            return qsTr("Ending call…");
        default:
            return "";
        }
    }

    function formatElapsed(s) {
        var m = Math.floor(s / 60);
        var sec = s % 60;
        return (m < 10 ? "0" + m : m) + ":" + (sec < 10 ? "0" + sec : sec);
    }

    visible: false
    anchors.fill: parent
    z: 10000
    color: Qt.rgba(0, 0, 0, 0.96)

    // Absorbs taps so they don't reach the page underneath.
    MouseArea {
        anchors.fill: parent
    }

    Timer {
        interval: 1000
        repeat: true
        running: callOverlay.visible && callOverlay.connected
        onTriggered: callOverlay.elapsed = Math.floor((Date.now() - callOverlay.connectedAt) / 1000)
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingLarge

        ProfileThumbnail {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.itemSizeHuge * 1.5
            height: width
            photoData: callOverlay.callerPhoto
            replacementStringHint: callOverlay.callerName
            optimizeImageSize: false
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: callOverlay.callerName
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeHuge
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeLarge
            text: callOverlay.statusText()
        }

        Column {
            width: parent.width
            spacing: Theme.paddingSmall
            visible: callOverlay.connected && callOverlay.verifyEmojis.length > 0

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge
                Repeater {
                    model: callOverlay.verifyEmojis
                    Label {
                        text: modelData
                        font.pixelSize: Theme.fontSizeExtraLarge * 1.4
                    }
                }
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("If these emoji match your contact's, the call is end-to-end encrypted")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }

        Item {
            width: 1
            height: Theme.paddingLarge
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingLarge * 2
            visible: callOverlay.ringingIncoming

            Button {
                text: qsTr("Decline")
                color: "#ff4444"
                onClicked: callOverlay.decline()
            }
            Button {
                text: qsTr("Accept")
                color: "#44dd44"
                onClicked: callOverlay.accept()
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingLarge * 2
            visible: !callOverlay.ringingIncoming

            Button {
                text: callOverlay.muted ? qsTr("Unmute") : qsTr("Mute")
                enabled: callOverlay.connected
                onClicked: callOverlay.toggleMute()
            }
            Button {
                text: callOverlay.speakerOn ? qsTr("Speaker off") : qsTr("Speaker")
                enabled: callOverlay.connected
                onClicked: callOverlay.toggleSpeaker()
            }
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !callOverlay.ringingIncoming
            text: qsTr("End call")
            color: "#ff4444"
            onClicked: callOverlay.endCall()
        }
    }
}
