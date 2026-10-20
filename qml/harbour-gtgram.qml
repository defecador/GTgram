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
import Sailfish.Share 1.0
import Nemo.DBus 2.0
import "pages"
import "components"
import "./js/functions.js" as Functions

ApplicationWindow
{
    id: appWindow

    initialPage: Qt.resolvedUrl("pages/OverviewPage.qml")
    cover: Qt.resolvedUrl("pages/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
    property var pendingShareResources: []

    Connections {
        target: dBusAdaptor
        onPleaseOpenMessage: {
            appWindow.activate();
        }
        onPleaseOpenUrl: {
            appWindow.activate();
        }
    }

    // "Share" integration (harbour-gtgram.desktop's X-Share-Methods): other
    // apps (Gallery, etc) invoke this over D-Bus rather than launching us
    // with command-line arguments, so it needs its own receiver distinct
    // from dBusAdaptor above (GTgram's own notification-action endpoint).
    function openShareDialog(resources) {
        if (!resources || resources.length === 0) {
            return;
        }
        var normalizedResources = [];
        var requiredPermissions = [];
        var hasTextResources = false;
        var hasFileResources = false;
        for (var i = 0; i < resources.length; i += 1) {
            var resource = resources[i];
            if (!resource) {
                continue;
            }
            if (typeof resource === 'string') {
                var asString = resource.toString();
                if (asString.indexOf("file://") === 0 || asString.indexOf("/") === 0) {
                    normalizedResources.push({
                        type: "file",
                        filePath: asString.indexOf("file://") === 0 ? asString.substring(7) : asString
                    });
                    hasFileResources = true;
                } else {
                    normalizedResources.push({ type: "text", data: asString });
                    hasTextResources = true;
                }
                continue;
            }
            var filePath = resource.filePath ? resource.filePath.toString() : "";
            if (filePath !== "") {
                normalizedResources.push({ type: "file", filePath: filePath });
                hasFileResources = true;
                continue;
            }
            var resourceData = resource.data ? resource.data.toString() : "";
            if (resourceData !== "") {
                normalizedResources.push({ type: "text", data: resourceData });
                hasTextResources = true;
            }
        }
        if (normalizedResources.length === 0) {
            return;
        }
        if (hasFileResources) {
            requiredPermissions = [
                "can_send_media_messages",
                "can_send_other_messages",
                "can_send_documents",
                "can_send_photos",
                "can_send_videos"
            ];
        } else if (hasTextResources) {
            requiredPermissions = [ "can_send_basic_messages" ];
        }
        pageStack.push(Qt.resolvedUrl("pages/ChatSelectionPage.qml"), {
            myUserId: tdLibWrapper.getUserInformation().id,
            headerDescription: qsTr("Send shared content"),
            payload: {
                resources: normalizedResources,
                neededPermissions: requiredPermissions
            },
            state: "shareResources"
        });
    }

    Timer {
        id: shareDispatchTimer
        interval: 0
        running: false
        repeat: false
        onTriggered: {
            if (appWindow.pendingShareResources && appWindow.pendingShareResources.length > 0) {
                var nextResources = appWindow.pendingShareResources;
                appWindow.pendingShareResources = [];
                appWindow.openShareDialog(nextResources);
            }
        }
    }

    ShareAction {
        id: shareActionParser
    }

    DBusAdaptor {
        service: "de.ygriega.gtgram"
        path: "/share/gtgram_share"
        iface: "org.sailfishos.share"

        function share(shareConfiguration) {
            shareActionParser.loadConfiguration(shareConfiguration);
            appWindow.activate();
            var queuedResources = [];
            if (shareActionParser.resources && shareActionParser.resources.length > 0) {
                for (var i = 0; i < shareActionParser.resources.length; i += 1) {
                    queuedResources.push(shareActionParser.resources[i]);
                }
            }
            appWindow.pendingShareResources = queuedResources;
            shareDispatchTimer.restart();
        }
    }

    Connections {
        target: tdLibWrapper
        onOpenFileExternally: {
            Qt.openUrlExternally(filePath);
        }
        onTgUrlFound: {
            Functions.handleLink(tgUrl);
        }
    }

    AppNotification {
        id: appNotification
        parent: pageStack.currentPage
    }

    Component.onCompleted: {
        Functions.setGlobals({
            tdLibWrapper: tdLibWrapper,
            appNotification: appNotification
        });
    }

    // Guards against a build without voice calls (voiceCallsAvailable is only
    // ever true when CallManager actually exists) and lets onCallUpdated
    // below be a no-op rather than error out on the missing property.
    readonly property bool voiceCallsEnabled: (typeof voiceCallsAvailable !== 'undefined') && voiceCallsAvailable === true

    Connections {
        target: tdLibWrapper
        onCallUpdated: {
            if (!appWindow.voiceCallsEnabled) {
                return;
            }
            var state = (call && call.state && call.state["@type"]) ? call.state["@type"] : "";
            var callId = call.id;
            var outgoing = call.is_outgoing === true;
            var ongoing = (state === "callStatePending" || state === "callStateExchangingKeys" || state === "callStateReady");
            if (ongoing) {
                if (callOverlay.callId !== callId) {
                    var userInfo = tdLibWrapper.getUserInformation(call.user_id.toString());
                    var name = userInfo ? Functions.getUserName(userInfo) : "";
                    callOverlay.callerName = name !== "" ? name : qsTr("Unknown caller");
                    callOverlay.callerPhoto = (userInfo && userInfo.profile_photo) ? userInfo.profile_photo.small : ({});
                    callOverlay.callId = callId;
                    callOverlay.outgoing = outgoing;
                    callOverlay.isVideo = call.is_video === true;
                    callOverlay.videoEnabled = true;
                    callOverlay.muted = false;
                    // Default to speaker for video (holding the phone to your
                    // ear defeats the point of seeing the screen); earpiece
                    // for voice, as before. Actually applied to PulseAudio
                    // once connected, below.
                    callOverlay.speakerOn = callOverlay.isVideo;
                    callOverlay.connectedAt = 0;
                    callOverlay.elapsed = 0;
                    callOverlay.verifyEmojis = [];
                }
                callOverlay.callState = state;
                if (state === "callStateReady" && callOverlay.connectedAt === 0) {
                    callOverlay.connectedAt = Date.now();
                    if (typeof callManager !== 'undefined') {
                        callManager.setSpeakerphoneOn(callOverlay.speakerOn);
                    }
                }
                if (state === "callStateReady" && call.state && call.state.emojis
                        && callOverlay.verifyEmojis.length === 0) {
                    callOverlay.verifyEmojis = call.state.emojis;
                }
                callOverlay.visible = true;
                appWindow.activate();
                if (callOverlay.savedOrientations === undefined && pageStack.currentPage) {
                    callOverlay.savedOrientations = pageStack.currentPage.allowedOrientations;
                    pageStack.currentPage.allowedOrientations = Orientation.Portrait;
                }
            } else if (callId === callOverlay.callId) {
                if (state === "callStateDiscarded" || state === "callStateError") {
                    var reason = (call.state && call.state.reason && call.state.reason["@type"]) ? call.state.reason["@type"] : "";
                    if (reason === "callDiscardReasonDeclined") {
                        appNotification.show(qsTr("Call declined"));
                    } else if (reason === "callDiscardReasonMissed") {
                        appNotification.show(callOverlay.outgoing ? qsTr("No answer") : qsTr("Missed call"));
                    } else if (reason === "callDiscardReasonDisconnected" || state === "callStateError") {
                        appNotification.show(qsTr("Call failed"));
                    }
                    callOverlay.visible = false;
                    callOverlay.callId = 0;
                    callOverlay.callState = "";
                    callOverlay.verifyEmojis = [];
                    if (callOverlay.savedOrientations !== undefined && pageStack.currentPage) {
                        pageStack.currentPage.allowedOrientations = callOverlay.savedOrientations;
                    }
                    callOverlay.savedOrientations = undefined;
                } else {
                    callOverlay.callState = state;
                }
            }
        }
    }

    CallOverlay {
        id: callOverlay
    }

    // With UnifiedPush a windowless instance quits when idle - but not during a call.
    Binding {
        target: unifiedPushManager
        property: "uiBusy"
        value: callOverlay.callId !== 0
    }
}
