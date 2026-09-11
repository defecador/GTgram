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
import "../js/debug.js" as Debug

// Up to TDLib 1.8.35, a message carried its own can_be_edited, can_be_deleted_...
// and can_be_forwarded flags. They were moved into a separate messageProperties
// object that has to be requested per message, so anything gated on those flags
// has to load them first.
QtObject {
    id: propertiesLoader

    property var chatId
    property var messageId
    // Load as soon as the ids are known, for callers that need the properties
    // before any user interaction. Others call load() when they need them.
    property bool autoLoad: false

    // Stays empty until TDLib answers - callers treat "unknown" as "not allowed".
    property var properties: ({})
    readonly property bool loaded: !!properties["@type"]

    property bool loading

    property var _tdLibConnections: Connections {
        target: tdLibWrapper
        // chatId/messageId here are the signal's arguments, hence the explicit ids below.
        // Callers hand us ids as either numbers or strings, so compare them as strings.
        onMessagePropertiesReceived: {
            if (String(chatId) === String(propertiesLoader.chatId)
                    && String(messageId) === String(propertiesLoader.messageId)) {
                propertiesLoader.properties = messageProperties;
                propertiesLoader.loading = false;
            }
        }
    }

    function load() {
        if (loading || loaded || !chatId || !messageId) {
            return;
        }
        Debug.log("[MessagePropertiesLoader] Obtaining message properties for ", chatId, messageId);
        loading = true;
        tdLibWrapper.getMessageProperties(Number(chatId), Number(messageId));
    }

    function reset() {
        properties = ({});
        loading = false;
    }

    onMessageIdChanged: {
        reset();
        if (autoLoad) {
            load();
        }
    }

    Component.onCompleted: {
        if (autoLoad) {
            load();
        }
    }
}
