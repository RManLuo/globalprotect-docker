#!/bin/sh
set -eu

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
wrapper="$repo_root/docker/gpagent-supervised.sh"
supervisor_conf="$repo_root/docker/supervisord.conf"
compose_file="$repo_root/docker-compose.yml"
auto_reconnect_example="$repo_root/docker-compose.auto-reconnect.example.yml"

grep -q "command=/supervisor-log-prefix.sh /gpagent-supervised.sh" "$supervisor_conf" \
    || fail "supervisor must launch gpagent through /gpagent-supervised.sh"

grep -q "GPAGENT_PORTAL=<vpn-portal-host>" "$compose_file" \
    || fail "docker-compose.yml must document the GPAGENT_PORTAL placeholder"

grep -q "GPAGENT_PORTAL=<vpn-portal-host>" "$auto_reconnect_example" \
    || fail "docker-compose.auto-reconnect.example.yml must document the GPAGENT_PORTAL placeholder"

[ -x "$wrapper" ] || fail "wrapper is missing or not executable: $wrapper"

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

fake_gpagent="$tmpdir/gpagent"
cat > "$fake_gpagent" <<'SCRIPT'
#!/bin/sh
for arg do
    printf '[%s]\n' "$arg"
done > "$GPAGENT_TEST_ARGS_FILE"
SCRIPT
chmod +x "$fake_gpagent"

args_file="$tmpdir/args"

GPAGENT_BINARY="$fake_gpagent" \
GPAGENT_TEST_ARGS_FILE="$args_file" \
    "$wrapper"

expected_without_portal='[--now]
[-platform]
[vnc:port=8998]'
actual_without_portal=$(cat "$args_file")
[ "$actual_without_portal" = "$expected_without_portal" ] \
    || fail "unexpected args without GPAGENT_PORTAL: $actual_without_portal"

GPAGENT_PORTAL="vpn.example.edu" \
GPAGENT_BINARY="$fake_gpagent" \
GPAGENT_TEST_ARGS_FILE="$args_file" \
    "$wrapper"

expected_with_portal='[vpn.example.edu]
[--now]
[-platform]
[vnc:port=8998]'
actual_with_portal=$(cat "$args_file")
[ "$actual_with_portal" = "$expected_with_portal" ] \
    || fail "unexpected args with GPAGENT_PORTAL: $actual_with_portal"
