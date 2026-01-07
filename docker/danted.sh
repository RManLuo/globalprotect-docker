#!/usr/bin/bash
# Wait for tun0 address to become available and use it in /etc/danted.conf
PIDFILE=/var/run/danted.pid
CURRENT_IPV4=
CURRENT_IPV6=

start_danted() {
    sed "s|##TUN0_IPV4##|$IPV4|; s|##TUN0_IPV6##|$IPV6|" /etc/danted.conf.in > /etc/danted.conf
    /usr/sbin/danted &
    echo $! > "$PIDFILE"
    echo "danted started with PID $(cat $PIDFILE)"
}

stop_danted() {
    if [ -f "$PIDFILE" ]; then
        PID=$(cat "$PIDFILE")
        if kill -0 "$PID" 2>/dev/null; then
            echo "Stopping danted PID $PID"
            kill "$PID"
            wait "$PID" 2>/dev/null || true
        fi
        rm -f "$PIDFILE"
    else
        pkill -f /usr/sbin/danted 2>/dev/null || true
    fi
}

while true; do
    IPV4=$(ip -4 addr show tun0 2>/dev/null | grep inet | awk '{print $2}' | sed "s|/.*||")
    IPV6=$(ip -6 addr show tun0 2>/dev/null | grep inet6 | grep global || true)
    IPV6=$(echo "$IPV6" | awk '{print $2}' | sed "s|/.*||")

    if [ -n "$IPV4" ] || [ -n "$IPV6" ]; then
        if [ "$IPV4" != "$CURRENT_IPV4" ] || [ "$IPV6" != "$CURRENT_IPV6" ]; then
            echo "Tun0 IP change detected: $CURRENT_IPV4/$CURRENT_IPV6 -> $IPV4/$IPV6"
            stop_danted
            start_danted
            CURRENT_IPV4=$IPV4
            CURRENT_IPV6=$IPV6
        fi
    else
        if [ -n "$CURRENT_IPV4" ] || [ -n "$CURRENT_IPV6" ]; then
            echo "Tun0 disappeared; stopping danted"
            stop_danted
            CURRENT_IPV4=
            CURRENT_IPV6=
        else
            echo "Waiting for VPN to become ready"
        fi
    fi

    sleep 10
done