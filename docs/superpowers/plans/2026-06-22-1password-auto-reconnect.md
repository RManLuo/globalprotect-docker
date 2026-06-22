# 1Password Auto-Reconnect Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build opt-in unattended reconnect that retrieves VPN username, password, and TOTP from 1Password CLI and drives the existing Qt login flow.

**Architecture:** Add small testable helpers for 1Password credential retrieval, reconnect retry policy, and SAML JavaScript generation. Wire those helpers into `StandardLoginWindow`, `SAMLLoginWindow`, `GPClient`, Docker startup, and documentation without removing the manual noVNC fallback.

**Tech Stack:** C++17, Qt 5 Core/Widgets/WebEngine/Test, CMake/CTest, Docker Compose, 1Password CLI.

---

## File Structure

- Create `include/credentialprovider.h` and `src/credentialprovider.cpp` for redacted `op` command execution.
- Create `include/autoreconnectpolicy.h` and `src/autoreconnectpolicy.cpp` for retry/backoff decisions.
- Create `include/samlloginautomation.h` and `src/samlloginautomation.cpp` for selector validation and JavaScript generation.
- Create `tests/test_credentialprovider.cpp`, `tests/test_autoreconnectpolicy.cpp`, and `tests/test_samlloginautomation.cpp`.
- Modify `CMakeLists.txt` to enable CTest and build test executables.
- Modify `src/standardloginwindow.cpp`, `src/samlloginwindow.cpp`, `src/gpclient.cpp`, and `include/gpclient.h` for feature wiring.
- Modify `docker/Dockerfile`, add `docker/entrypoint.sh`, and update `docker-compose.yml` for `op` and token-file support.
- Update `README.md` and `.gitignore` for secure local configuration.

## Tasks

### Task 1: Test Infrastructure

- [ ] Add test executables to `CMakeLists.txt`.
- [ ] Add placeholder tests that include the planned headers.
- [ ] Run `g++ -std=c++17 -Iinclude tests/test_credentialprovider.cpp -o /tmp/credentialprovider_test` and verify failure because production headers do not exist.
- [ ] Keep CMake test wiring minimal and do not touch runtime behavior.

### Task 2: Credential Provider

- [ ] Write failing tests for successful username/password/TOTP retrieval through a fake `op` executable.
- [ ] Write failing tests for missing item configuration and redacted error messages.
- [ ] Implement `OnePasswordCredentialProvider` with `GPAGENT_OP_PATH`, `GPAGENT_OP_ITEM`, and optional `GPAGENT_OP_VAULT`.
- [ ] Verify the credential provider tests pass.

### Task 3: Reconnect Policy

- [ ] Write failing tests for disabled mode, bounded delays, retry exhaustion, and explicit disconnect suppression.
- [ ] Implement `AutoReconnectPolicy`.
- [ ] Verify the reconnect policy tests pass.

### Task 4: SAML Login Automation

- [ ] Write failing tests for required selector validation and JavaScript escaping.
- [ ] Implement `SamlLoginAutomation::buildScript()`.
- [ ] Verify the SAML automation tests pass.

### Task 5: Qt Integration

- [ ] Wire `OnePasswordCredentialProvider` into normal login autofill and auto-submit when enabled.
- [ ] Wire `SamlLoginAutomation` into `SAMLLoginWindow::onLoadFinished()` with fallback to visible manual login.
- [ ] Wire `AutoReconnectPolicy` into `GPClient::onVPNDisconnected()` while suppressing explicit disconnect, quit, reset, and gateway-switch loops.
- [ ] Verify all unit tests still pass.

### Task 6: Docker And Docs

- [ ] Install `1password-cli` in the runtime Docker image using 1Password's apt repository.
- [ ] Add an entrypoint that exports `OP_SERVICE_ACCOUNT_TOKEN` from `OP_SERVICE_ACCOUNT_TOKEN_FILE`.
- [ ] Update Compose examples without committing real secrets.
- [ ] Update README with setup, selector, and security guidance.
- [ ] Verify Dockerfile syntax and test/build commands as far as the local environment allows.
