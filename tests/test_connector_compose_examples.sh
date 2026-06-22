#!/bin/sh
set -eu

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tailscale_file="$repo_root/docker-compose.tailscale.example.yml"
cloudflare_file="$repo_root/docker-compose.cloudflare.example.yml"
auto_reconnect_example="$repo_root/docker-compose.auto-reconnect.example.yml"
readme="$repo_root/README.md"
gitignore="$repo_root/.gitignore"

[ -f "$tailscale_file" ] || fail "missing Tailscale Compose example"
[ -f "$cloudflare_file" ] || fail "missing Cloudflare Compose example"

grep -q 'network_mode: "service:globalprotect"' "$tailscale_file" \
    || fail "Tailscale sidecar must share the GlobalProtect network namespace"
grep -q "TS_USERSPACE=false" "$tailscale_file" \
    || fail "Tailscale must use kernel networking for subnet routing"
grep -q "TS_ROUTES=" "$tailscale_file" \
    || fail "Tailscale example must require TS_ROUTES"
grep -q "net.ipv4.ip_forward=1" "$tailscale_file" \
    || fail "Tailscale example must enable IPv4 forwarding"
grep -q "/dev/net/tun" "$tailscale_file" \
    || fail "Tailscale example must mount /dev/net/tun"
grep -q "^\\.tailscale-state/$" "$gitignore" \
    || fail ".tailscale-state/ must be ignored"

grep -q 'network_mode: "service:globalprotect"' "$cloudflare_file" \
    || fail "Cloudflare sidecar must share the GlobalProtect network namespace"
grep -q "cloudflare/cloudflared:latest" "$cloudflare_file" \
    || fail "Cloudflare example must use the official cloudflared image"
grep -q "token-file /run/secrets/cloudflare_tunnel_token" "$cloudflare_file" \
    || fail "Cloudflare example must use token-file with a Docker secret"
grep -q "cloudflare_tunnel_token:" "$cloudflare_file" \
    || fail "Cloudflare example must define the token secret"

! grep -q "tailscale:" "$auto_reconnect_example" \
    || fail "auto-reconnect example must not include the Tailscale sidecar"
! grep -q "cloudflared:" "$auto_reconnect_example" \
    || fail "auto-reconnect example must not include the Cloudflare sidecar"

grep -q "docker-compose.auto-reconnect.example.yml" "$readme" \
    || fail "README must document the auto-reconnect override"
grep -q "docker-compose.tailscale.example.yml" "$readme" \
    || fail "README must document the Tailscale override"
grep -q "docker-compose.cloudflare.example.yml" "$readme" \
    || fail "README must document the Cloudflare override"
grep -q "Combine the overrides" "$readme" \
    || fail "README must explain how to combine override files"
grep -q "Auto-reconnect + Tailscale + Cloudflare" "$readme" \
    || fail "README must show the full combined mode"
grep -q "https://www.1password.dev/service-accounts/get-started" "$readme" \
    || fail "README must link to 1Password service account setup"
grep -q "https://tailscale.com/docs/features/subnet-routers" "$readme" \
    || fail "README must link to Tailscale subnet router setup"
grep -q "https://login.tailscale.com/admin/machines" "$readme" \
    || fail "README must link to the Tailscale Machines admin page"
grep -q "https://developers.cloudflare.com/cloudflare-one/networks/connectors/cloudflare-tunnel/get-started/create-remote-tunnel/" "$readme" \
    || fail "README must link to Cloudflare Tunnel dashboard setup"
grep -q "https://developers.cloudflare.com/cloudflare-one/networks/routes/add-routes/" "$readme" \
    || fail "README must link to Cloudflare route setup"
grep -q "https://developers.cloudflare.com/cloudflare-one/networks/connectors/cloudflare-tunnel/private-net/cloudflared/connect-cidr/" "$readme" \
    || fail "README must link to Cloudflare IP/CIDR private network setup"
grep -q "https://developers.cloudflare.com/cloudflare-one/team-and-resources/devices/cloudflare-one-client/configure/route-traffic/split-tunnels/" "$readme" \
    || fail "README must link to Cloudflare Split Tunnels setup"
grep -q "https://developers.cloudflare.com/cloudflare-one/team-and-resources/devices/cloudflare-one-client/deployment/manual-deployment/" "$readme" \
    || fail "README must link to Cloudflare One Client enrollment"

TS_AUTHKEY="tskey-auth-test" \
TS_ROUTES="10.0.0.0/8" \
TS_HOSTNAME="globalprotect-vpn-test" \
docker compose -f "$repo_root/docker-compose.yml" -f "$tailscale_file" config >/dev/null

docker compose -f "$repo_root/docker-compose.yml" -f "$cloudflare_file" config >/dev/null
