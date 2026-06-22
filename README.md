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

The container can reconnect automatically after an unexpected VPN disconnect by reading username, password, and TOTP from 1Password CLI. This is disabled by default.

1. Create a 1Password service account with read-only access to the VPN login item.
2. Store the token outside git:

```
mkdir -p .secrets
printf '%s' 'ops_...' > .secrets/op_service_account_token
chmod 600 .secrets/op_service_account_token
```

3. Uncomment the `GPAGENT_AUTO_RECONNECT`, `GPAGENT_OP_*`, selector, and `secrets` examples in `docker-compose.yml`.

The SAML selectors must match your identity-provider page:

```
GPAGENT_LOGIN_USERNAME_SELECTOR=#username
GPAGENT_LOGIN_PASSWORD_SELECTOR=#password
GPAGENT_LOGIN_TOTP_SELECTOR=#totp
GPAGENT_LOGIN_SUBMIT_SELECTOR=button[type=submit]
```

Never commit `.secrets/`, service account tokens, generated TOTP codes, VPN cookies, or real credentials.

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
