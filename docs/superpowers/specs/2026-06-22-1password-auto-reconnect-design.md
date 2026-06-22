# 1Password Auto-Reconnect Design

## Goal

Add unattended reconnect for the Dockerized GlobalProtect client. When the VPN connection drops unexpectedly, the client should start a bounded reconnect flow, fetch username, password, and TOTP from 1Password CLI, and complete the login without requiring the user to open noVNC.

Manual noVNC login remains available as the default and as a fallback.

## Non-Goals

- Do not store VPN credentials, TOTP codes, cookies, or 1Password tokens in the repository.
- Do not automate arbitrary identity providers without configured selectors.
- Do not remove the existing Qt login dialogs or noVNC access path.

## Configuration

The feature is opt-in through environment variables:

- `GPAGENT_AUTO_RECONNECT=1`: enable automatic reconnect.
- `GPAGENT_OP_ITEM`: 1Password item name or reference for the VPN login.
- `GPAGENT_OP_VAULT`: vault name when required by service-account access.
- `OP_SERVICE_ACCOUNT_TOKEN_FILE`: path to a mounted Docker secret containing the service account token.
- `GPAGENT_LOGIN_USERNAME_SELECTOR`: CSS selector for the SAML username field.
- `GPAGENT_LOGIN_PASSWORD_SELECTOR`: CSS selector for the SAML password field.
- `GPAGENT_LOGIN_TOTP_SELECTOR`: CSS selector for the SAML TOTP field.
- `GPAGENT_LOGIN_SUBMIT_SELECTOR`: CSS selector for the submit button.

Docker startup glue reads `OP_SERVICE_ACCOUNT_TOKEN_FILE`, exports `OP_SERVICE_ACCOUNT_TOKEN`, and then launches supervisord. The token file must be mounted from an untracked local path or Docker secret.

## Architecture

### Credential Provider

Add a small provider responsible only for retrieving secrets from `op`. It fetches:

- username via `op item get "$GPAGENT_OP_ITEM" --vault "$GPAGENT_OP_VAULT" --fields username`
- password via the same command with `password`
- TOTP via `op item get "$GPAGENT_OP_ITEM" --vault "$GPAGENT_OP_VAULT" --otp`

The provider returns values to the login flow in memory and never logs secret output. Command failures are reported with redacted diagnostics.

### Reconnect Manager

Add reconnect coordination around VPN state changes. When `GPService` reports a disconnect that was not caused by explicit user action, the manager schedules reconnect attempts with bounded exponential backoff. A practical default is five attempts with 30s, 60s, 120s, 240s, and 300s delays.

Explicit disconnect, quit, or reset disables the active retry loop until the user starts a new connection.

### Login Automation

For normal username/password login, bypass manual dialog entry when automation is enabled and credentials are available.

For SAML login, use the existing `QWebEngineView` path. After page load, inject JavaScript with `runJavaScript()` to fill the configured selectors and submit the form. The TOTP is generated immediately before injection to reduce expiry risk. If any selector is missing or injection fails, show the existing login window so the user can complete login through noVNC.

## Error Handling

- Missing `op`, missing token, denied vault access, or missing item fields disables the current automated attempt and logs a redacted error.
- TOTP generation happens per attempt; stale codes are not reused.
- Retry exhaustion leaves the client disconnected and visible through noVNC for manual recovery.
- The reconnect manager must distinguish unexpected disconnects from user-requested disconnects to avoid reconnect loops.

## Security

The service account should have read-only access to the minimum vault or item needed for VPN login. `.secrets/` and any local token files must remain untracked. Logs must never include credentials, generated TOTP codes, service account tokens, auth cookies, or full SAML form payloads.

## Verification Plan

1. Build locally with CMake and confirm `gpagent` still compiles.
2. Build the Docker image and confirm `op` is available in the runtime image.
3. Run with automation disabled and verify the existing manual noVNC login still works.
4. Run with automation enabled and a test 1Password item; force a disconnect and verify the client reconnects without manual credential entry.
5. Test selector failure and `op` failure paths to confirm they fall back to visible manual login without leaking secrets.
