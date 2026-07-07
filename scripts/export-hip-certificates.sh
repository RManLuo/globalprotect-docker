#!/bin/sh
set -eu

usage() {
	echo "Usage: $0 <certificate-common-name> [output-pem-file]" >&2
	echo "Example: $0 user.ad.example.edu config/hip-certificates.pem" >&2
	exit 2
}

[ "${1:-}" ] || usage

COMMON_NAME=$1
OUTPUT_FILE=${2:-${HIP_CERTIFICATE_PEM_FILE:-config/hip-certificates.pem}}

if ! command -v security >/dev/null 2>&1; then
	echo "security command not found; this exporter must run on macOS" >&2
	exit 1
fi

IDENTITY_HASHES=$(security find-identity -v | awk -v cn="$COMMON_NAME" '
	$0 ~ "\"" cn "\"" {
		print $2
	}
')

if [ -z "$IDENTITY_HASHES" ]; then
	echo "No valid identities found for common name: $COMMON_NAME" >&2
	exit 1
fi

TMPDIR_EXPORT=$(mktemp -d "${TMPDIR:-/tmp}/hip-cert-export.XXXXXX")
trap 'rm -rf "$TMPDIR_EXPORT"' EXIT

CERT_DUMP="$TMPDIR_EXPORT/certificates.txt"
EXPORTED_CERTS="$TMPDIR_EXPORT/hip-certificates.pem"

security find-certificate -Z -a -c "$COMMON_NAME" -p > "$CERT_DUMP"

COUNT=0
for IDENTITY_HASH in $IDENTITY_HASHES; do
	if awk -v want="$IDENTITY_HASH" '
		/^SHA-1 hash:/ {
			matched = ($3 == want)
			in_cert = 0
			next
		}
		/-----BEGIN CERTIFICATE-----/ {
			in_cert = matched
		}
		in_cert {
			print
		}
		/-----END CERTIFICATE-----/ && in_cert {
			found = 1
			exit
		}
		END {
			exit found ? 0 : 1
		}
	' "$CERT_DUMP" >> "$EXPORTED_CERTS"; then
		COUNT=$((COUNT + 1))
	else
		echo "Warning: certificate not found for identity SHA-1 $IDENTITY_HASH" >&2
	fi
done

if [ "$COUNT" -eq 0 ]; then
	echo "No certificates exported for common name: $COMMON_NAME" >&2
	exit 1
fi

mkdir -p "$(dirname "$OUTPUT_FILE")"
install -m 600 "$EXPORTED_CERTS" "$OUTPUT_FILE"
echo "Exported $COUNT certificate(s) to $OUTPUT_FILE"
