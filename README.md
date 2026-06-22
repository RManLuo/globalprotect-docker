# GlobalProtect VPN client (GUI) in a Docker container

This is an implementation of GlobalProtect VPN client (GUI), which runs in a Docker container and exposes the VPN connection to the users as a SOCKS5 proxy.

Technically, the Docker container runs a fork of [GlobalProtect-openconnect](https://github.com/yuezk/GlobalProtect-openconnect), redesigned to come as a single executable, without client-server separation.

<img src="screenshots/screenshot1.png"><img src="screenshots/screenshot2.png">

## Features

- Similar user experience as the official client in macOS.
- Supports both SAML and non-SAML authentication modes.
- Supports automatically selecting the preferred gateway from the multiple gateways.
- Supports switching gateway from the system tray menu manually.
- Memorizes credentials and authenticates automatically without a dialog.

# Docker
 
```
git clone --recurse-submodules https://github.com/RManLuo/globalprotect-docker.git
cd globalprotect-docker
docker-compose up -d
```
 
On the first run, navigate to `http://localhost:8083` in the web browser to provide authentication credentials. On subsequent invocations, the container will  try to use the cached credentials.

When the connection is established, configure your applications to use the provided SOCKS5 proxy (SOCKS5 Port: 10081 by default). For example, Firefox:

<img src="screenshots/screenshot3.png">

## Optional 1Password auto-reconnect

The container can reconnect automatically after an unexpected VPN disconnect by reading the login username, password, and TOTP from 1Password CLI. This is disabled by default and should be configured with local-only files.

### 1Password item requirements

Create a 1Password Login item that contains:

- `username`: your identity-provider username.
- `password`: your identity-provider password.
- One-time password/TOTP field: the same TOTP seed used by your authenticator app.

Create a 1Password service account with read-only access to only the vault that contains this item. Store the service account token outside git:

```
mkdir -p .secrets
printf '%s' 'ops_...' > .secrets/op_service_account_token
chmod 600 .secrets/op_service_account_token
```

Never commit `.secrets/`, service account tokens, generated TOTP codes, VPN cookies, or real credentials.

### Local Compose override

Copy the committed example to an untracked local override file, then replace the placeholders:

```bash
cp docker-compose.auto-reconnect.example.yml docker-compose.auto-reconnect.yml
```

The local file should look like this:

```yaml
services:
  globalprotect:
    environment:
      - GPAGENT_AUTO_RECONNECT=1
      - GPAGENT_PORTAL=<vpn-portal-host>
      - GPAGENT_OP_ITEM=<1password-item-title-or-id>
      - GPAGENT_OP_VAULT=<1password-vault-name-or-id>
      - OP_SERVICE_ACCOUNT_TOKEN_FILE=/run/secrets/op_service_account_token
      - GPAGENT_LOGIN_USERNAME_SELECTOR=<username-input-css-selector>
      - GPAGENT_LOGIN_PASSWORD_SELECTOR=<password-input-css-selector>
      - GPAGENT_LOGIN_TOTP_SELECTOR=<totp-input-css-selector>
      - GPAGENT_LOGIN_SUBMIT_SELECTOR=<submit-button-css-selector>
      - GPAGENT_SAML_THROTTLE_SLEEP_SECONDS=300
    secrets:
      - op_service_account_token

secrets:
  op_service_account_token:
    file: ./.secrets/op_service_account_token
```

`GPAGENT_PORTAL` pre-fills the portal and starts the first connection attempt. The selectors must match your identity-provider login pages. Use browser developer tools in the noVNC window to inspect the username, password, TOTP, and submit controls. Common examples are `#username`, `input[name="identifier"]`, `input[type="password"]`, `input[autocomplete="one-time-code"]`, and `button[type="submit"]`.

Some identity providers show username, password, and TOTP on separate screens. In that case, keep all relevant selectors configured; the automation fills only the visible matching field on each page.

If the identity provider reports too many login attempts, the container waits for `GPAGENT_SAML_THROTTLE_SLEEP_SECONDS` seconds and restarts login from the beginning with a fresh prelogin request. The default is 300 seconds.

### Monash login template

For Monash GlobalProtect, `docker-compose.auto-reconnect.example.yml` already includes the current selector template. Keep the personal values as placeholders in git and fill them only in your local `docker-compose.auto-reconnect.yml`:

```yaml
services:
  globalprotect:
    environment:
      - GPAGENT_AUTO_RECONNECT=1
      - GPAGENT_PORTAL=vpn.gp.monash.edu
      - GPAGENT_OP_ITEM=<your-1password-item-title-or-id>
      - GPAGENT_OP_VAULT=<your-1password-vault-name-or-id>
      - OP_SERVICE_ACCOUNT_TOKEN_FILE=/run/secrets/op_service_account_token
      - GPAGENT_LOGIN_USERNAME_SELECTOR=input[name="identifier"]
      - GPAGENT_LOGIN_PASSWORD_SELECTOR=input[name="credentials.passcode"]
      - GPAGENT_LOGIN_TOTP_SELECTOR=input[name="credentials.passcode"]
      - GPAGENT_LOGIN_SUBMIT_SELECTOR=input[type="submit"]
      - GPAGENT_SAML_THROTTLE_SLEEP_SECONDS=60
    secrets:
      - op_service_account_token

secrets:
  op_service_account_token:
    file: ./.secrets/op_service_account_token
```

The password and TOTP pages both use `input[name="credentials.passcode"]`; the automation decides which value to fill based on the visible page context. If the identity-provider page changes or automation cannot find a field, open noVNC at `http://localhost:8083` and complete or inspect the login there.

### Verify 1Password access

Check the configured item before starting the container. Do not print the password or TOTP value in logs or issue reports.

```bash
export OP_SERVICE_ACCOUNT_TOKEN="$(cat .secrets/op_service_account_token)"
op item get "<1password-item-title-or-id>" --vault "<1password-vault-name-or-id>" --fields label=username --reveal
op item get "<1password-item-title-or-id>" --vault "<1password-vault-name-or-id>" --fields label=password --reveal | wc -c
op item get "<1password-item-title-or-id>" --vault "<1password-vault-name-or-id>" --otp | wc -c
```

The password command intentionally uses `--fields label=password --reveal`. Without `--reveal`, 1Password may return a concealed placeholder instead of the real password.

### Run with auto-login enabled

```
docker compose -f docker-compose.yml -f docker-compose.auto-reconnect.yml up -d --build
docker compose -f docker-compose.yml -f docker-compose.auto-reconnect.yml logs -f globalprotect
```

The VPN is ready when the logs show that `openconnect` established the tunnel and the SOCKS5 proxy is listening on `127.0.0.1:10081`. If login fails, inspect the noVNC window at `http://localhost:8083`, adjust the selectors in the local override, and restart the container.

## Manual Installation

Prerequisites:

```
sudo apt-get install -y \
     build-essential \
     qtbase5-dev \
     libqt5websockets5-dev \
     qtwebengine5-dev \
     qttools5-dev \
     qt5keychain-dev \
     openconnect
```

Building:

```
git clone --recurse-submodules https://github.com/dmikushin/globalprotect-docker.git
cd globalprotect-docker
mkdir build
cd build
cmake -G Ninja ..
cmake --build .
sudo cmake --install .
```

Without client-server separation, the binary must be executed with elevated priviledges:

```
sudo ./gpagent
```

## Troubleshooting

Run `docker-compose logs` in the Terminal and collect the logs.
