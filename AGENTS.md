# Repository Guidelines

## Project Structure & Module Organization

This repository packages a Qt 5 GlobalProtect VPN GUI client as a Dockerized SOCKS5/noVNC service. Core C++ sources live in `src/`, public headers in `include/`, and Qt Designer forms in `src/*.ui`. Build configuration is in `CMakeLists.txt` and `version.h.in`. Docker assets are under `docker/`, local container configuration is under `config/`, and `docker-compose.yml` defines the default service. Vendored dependencies are in `ThirdParty/`; avoid editing them unless updating the dependency itself.

## Build, Test, and Development Commands

- `git submodule update --init --recursive`: fetch required vendored submodules after cloning.
- `mkdir -p build && cd build && cmake -G Ninja .. && cmake --build .`: configure and build the `gpagent` binary locally.
- `sudo cmake --install .`: install the locally built binary and desktop metadata.
- `docker compose up -d --build`: build and run the containerized client locally.
- `docker compose logs -f globalprotect`: inspect VPN, noVNC, and SOCKS runtime logs.

The GitHub workflow in `.github/workflows/docker-build.yml` builds and publishes multi-architecture images from `docker/Dockerfile`.

## Coding Style & Naming Conventions

Use C++17 and match the existing Qt style. Keep headers in `include/` and implementations in `src/` with matching lowercase filenames such as `gpclient.h` and `gpclient.cpp`. Classes use PascalCase, methods and variables use lower camel case, and Qt slots follow `on_<widget>_<signal>()` where generated connections are used. Preserve four-space indentation in C++ and YAML indentation in workflow and Compose files.

## Testing Guidelines

There is no dedicated automated test suite in the current tree. For C++ changes, run a clean CMake build. For Docker or runtime changes, run `docker compose up -d --build`, open `http://localhost:8083`, and verify the SOCKS5 proxy on `127.0.0.1:10081`. Include relevant log excerpts when reporting failures, but redact portals, usernames, cookies, and credentials.

After implementing any feature, run an end-to-end check that exercises the complete affected workflow before reporting the work as complete. Unit tests and builds are required but not sufficient for runtime behavior. For UI, Docker, authentication, reconnect, proxy, or VPN changes, verify the feature through the running container and record the pass/fail evidence using redacted logs or command summaries. If an end-to-end check cannot be run, state the blocker explicitly and provide the closest completed verification.

## End-to-End VPN Runtime Test Workflow

When asked to run an end-to-end VPN test, verify the whole path rather than stopping at UI login. Keep all organization-specific values in an untracked local Compose override and all secrets in untracked local files or environment variables. Never commit portals, account names, vault names, item names, selectors tied to a private IdP, screenshots, logs, cookies, TOTP codes, service-account tokens, or generated VPN config.

Create a local override from this template and replace every placeholder locally:

```yaml
services:
  globalprotect:
    environment:
      - GPAGENT_AUTO_RECONNECT=1
      - GPAGENT_OP_ITEM=<1password-item-title-or-id>
      - GPAGENT_OP_VAULT=<1password-vault-name-or-id>
      - OP_SERVICE_ACCOUNT_TOKEN_FILE=/run/secrets/op_service_account_token
      - GPAGENT_LOGIN_USERNAME_SELECTOR=<username-input-css-selector>
      - GPAGENT_LOGIN_PASSWORD_SELECTOR=<password-input-css-selector>
      - GPAGENT_LOGIN_TOTP_SELECTOR=<totp-input-css-selector>
      - GPAGENT_LOGIN_SUBMIT_SELECTOR=<submit-button-css-selector>
    secrets:
      - op_service_account_token

secrets:
  op_service_account_token:
    file: ./.secrets/op_service_account_token
```

Before starting the container, confirm 1Password access without exposing values:

```bash
export OP_SERVICE_ACCOUNT_TOKEN="$(cat .secrets/op_service_account_token)"
op item get "$GPAGENT_OP_ITEM" --vault "$GPAGENT_OP_VAULT" --fields label=username --reveal
op item get "$GPAGENT_OP_ITEM" --vault "$GPAGENT_OP_VAULT" --fields label=password --reveal | wc -c
op item get "$GPAGENT_OP_ITEM" --vault "$GPAGENT_OP_VAULT" --otp | wc -c
```

Report only lengths or redacted values. If a concealed password appears too long, check that the code and manual probes use `--fields label=password --reveal`; otherwise `op` may return a masked placeholder instead of the real password. If manual browser inspection is needed, capture screenshots only into ignored paths and delete them before preparing a commit.

Run the full path with:

```bash
docker compose -f docker-compose.yml -f <local-override.yml> down
docker compose -f docker-compose.yml -f <local-override.yml> up -d --build globalprotect
docker compose -f docker-compose.yml -f <local-override.yml> logs -f --tail=200 globalprotect
```

Success requires evidence for all checkpoints: browser-based SAML login succeeds, gateway login uses a non-empty host, `openconnect` starts, logs show `ESP session established`, `tun0` has an assigned address, and Dante listens on `1080` inside the container. Confirm with `pgrep -a openconnect`, `ifconfig tun0`, `pgrep -a danted`, and `netstat -lntp | grep -E '(:1080|:8083)'`. In final reports and commits, include only generic status, redacted excerpts, and command names.

## Commit & Pull Request Guidelines

Recent history uses short imperative messages, often with `fix:` and `feat:` prefixes. Prefer focused commits like `fix: update docker build platform` or `feat: improve noVNC clipboard support`. Pull requests should explain the user-visible change, list validation commands, and call out any Docker image, port, capability, or VPN configuration impact. Include screenshots only for UI/noVNC-visible changes.

## Security & Configuration Tips

Do not commit real VPN portals, credentials, cookies, private keys, or generated config containing secrets. Keep local state under `config/` only when it is safe to share, and prefer environment variables or untracked local files for sensitive values.
