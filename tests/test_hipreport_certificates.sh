#!/bin/sh
set -eu

fail() {
	echo "FAIL: $*" >&2
	exit 1
}

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
hipreport="$repo_root/config/hipreport.sh"

[ -x "$hipreport" ] || fail "hipreport.sh is missing or not executable"

if ! command -v openssl >/dev/null 2>&1; then
	echo "SKIP: openssl is required for HIP certificate test"
	exit 0
fi

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

cert_file="$tmpdir/hip-certificates.pem"
key_file="$tmpdir/hip-cert.key"
report_file="$tmpdir/hip-report.xml"

openssl req \
	-x509 \
	-newkey rsa:2048 \
	-nodes \
	-subj "/CN=test-hip-cert.example" \
	-days 1 \
	-keyout "$key_file" \
	-out "$cert_file" >/dev/null 2>&1

HIP_CERTIFICATE_PEM_FILE="$cert_file" \
APP_VERSION=6.3.3-638 \
	"$hipreport" \
	--cookie "user=alice&domain=example&computer=test-host" \
	--md5 "0123456789abcdef0123456789abcdef" \
	--client-ip "10.0.0.2" \
	--client-os "Mac" > "$report_file"

grep -q '<entry name="certificate">' "$report_file" \
	|| fail "certificate category missing"
grep -q '<subject>/CN=test-hip-cert.example</subject>' "$report_file" \
	|| fail "certificate subject missing"
grep -q -- '-----BEGIN CERTIFICATE-----' "$report_file" \
	|| fail "certificate PEM missing"
grep -q -- '-----END CERTIFICATE-----' "$report_file" \
	|| fail "certificate PEM terminator missing"
