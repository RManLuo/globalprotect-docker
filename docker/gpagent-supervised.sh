#!/bin/sh
set -eu

gpagent_binary="${GPAGENT_BINARY:-/usr/bin/gpagent}"

if [ -n "${GPAGENT_PORTAL:-}" ]; then
    exec "$gpagent_binary" "$GPAGENT_PORTAL" --now -platform vnc:port=8998
fi

exec "$gpagent_binary" --now -platform vnc:port=8998
