#!/bin/sh
# Restarts voicecall-manager after installing/updating GTgram.
#
# Needed because our provider plugin (libgtgram-voicecall-plugin.so) is only
# loaded by voicecall-manager when the service itself starts: without this
# restart it would stay inert until the next reboot, and calls wouldn't show
# up above the lock screen.
#
# voicecall-manager is a user SESSION service, while RPM scriptlets run as
# root: reaching it means going through XDG_RUNTIME_DIR. This is best-effort:
# if it fails, the plugin kicks in on the next boot and the app uses the MCE
# fallback path in the meantime (screen on, but no system UI).
#
# If a call is currently in progress, nothing is restarted here: interrupting
# it would be worse than the benefit.
for home in /home/*; do
    [ -d "$home" ] || continue
    user=$(basename "$home")
    uid=$(id -u "$user" 2>/dev/null) || continue
    [ -S "/run/user/$uid/dbus/user_bus_socket" ] || continue

    call_state=$(dbus-send --system --print-reply --dest=com.nokia.mce \
        /com/nokia/mce/request com.nokia.mce.request.get_call_state 2>/dev/null \
        | grep -o '"[a-z]*"' | head -1)
    if [ "$call_state" = '"active"' ] || [ "$call_state" = '"ringing"' ]; then
        echo "call in progress: not restarting voicecall-manager"
        continue
    fi

    su -s /bin/sh -c "XDG_RUNTIME_DIR=/run/user/$uid systemctl --user restart voicecall-manager" "$user" 2>/dev/null || true
done
exit 0
