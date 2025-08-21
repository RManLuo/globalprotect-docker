#!/usr/bin/bash
# Wait for tun0 address to become available and use it in /etc/danted.conf
while true; do
    IPV4=$(ip -4 addr show tun0 | grep inet | awk '{print $2}' | sed "s|/.*||")
    IPV6=$(ip -6 addr show tun0 | grep inet6 | grep global | awk '{print $2}' | sed "s|/.*||")

    if [ "x$IPV4" != "x" ] && [ "x$IPV6" != "x" ]; then
        echo "Starting SOCKS5 proxy to destination $IPV4 (IPv4) and $IPV6 (IPv6) provided by VPN"
        sed "s|##TUN0_IPV4##|$IPV4|; s|##TUN0_IPV6##|$IPV6|" /etc/danted.conf.in > /etc/danted.conf
        /usr/sbin/danted
    fi

    echo "Waiting for VPN to become ready"
    sleep 1
done