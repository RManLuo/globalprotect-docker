#!/bin/sh
set -eu

if [ -n "${OP_SERVICE_ACCOUNT_TOKEN_FILE:-}" ]; then
    if [ ! -r "$OP_SERVICE_ACCOUNT_TOKEN_FILE" ]; then
        echo "OP_SERVICE_ACCOUNT_TOKEN_FILE is set but cannot be read" >&2
        exit 1
    fi
    export OP_SERVICE_ACCOUNT_TOKEN="$(cat "$OP_SERVICE_ACCOUNT_TOKEN_FILE")"
fi

exec "$@"
