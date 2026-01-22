ARG MAKE_JOBS=12
ARG CMAKE_BUILD_TYPE=Release

FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update

RUN apt-get install -y \
      build-essential \
      cmake \
      qtbase5-dev \
      libqt5websockets5-dev \
      qtwebengine5-dev \
      qttools5-dev \
      qt5keychain-dev

COPY . /gpagent
RUN cd /gpagent && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/gpagent/install -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} .. && \
    make -j${MAKE_JOBS} && \
    make install

FROM ubuntu:24.04
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update

RUN apt-get -y --no-install-recommends install \
    build-essential \
    libqt5core5a \
    libqt5websockets5 \
    libqt5webenginewidgets5 \
    libqt5keychain1 \
    openconnect \
    ca-certificates

RUN apt-get -y --no-install-recommends install \
    openssh-server \
    sudo \
    vim

# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd && \
    mkdir -p /var/run/sshd && \
    bash -c 'install -m755 <(printf "#!/bin/sh\nexit 0") /usr/sbin/policy-rc.d' && \
    ex +'%s/^#\zeHostKey .*ssh_host_.*_key/\1/g' -scwq /etc/ssh/sshd_config && \
    echo "MaxSessions 2147483647" >> /etc/ssh/sshd_config && \
    RUNLEVEL=1 dpkg-reconfigure openssh-server && \
    ssh-keygen -A -v && \
    update-rc.d ssh defaults

RUN apt-get -y --no-install-recommends install \
    supervisor

# Create an SSH key for SSH login
RUN sudo ssh-keygen -t rsa -q -f "/root/.ssh/id_rsa" -N "" && \
    mv /root/.ssh/id_rsa.pub /root/.ssh/authorized_keys

# We use supervisor instead of systemd to start multiple applications.
COPY ./docker/supervisord.conf /etc/
COPY ./docker/supervisor-log-prefix.sh /

COPY --from=builder /gpagent/install/ /usr

# Allow plain username/password storage in the configuration folder,
# because all keyrings require D-Bus, which we don't have in container.
ENV GPAGENT_ALLOW_PLAIN_PASSWORD=1

RUN apt-get -y --no-install-recommends install \
     git \
     python3 \
     python3-pip \
     python3-numpy

# Setup noVNC to access gpagent via web browser.
# Using latest version for better clipboard and mobile support
WORKDIR /usr/share/
RUN git clone --depth 1 https://github.com/novnc/noVNC.git && \
    cd noVNC && \
    ln -s vnc.html index.html

# Install websockify with clipboard support
RUN pip3 install --no-cache-dir websockify numpy --break-system-packages

# Create symlink for websockify to ensure it's in PATH
RUN ln -sf /usr/local/bin/websockify /usr/bin/websockify || \
    ln -sf $(which websockify) /usr/bin/websockify

# Create custom noVNC config for clipboard and mobile keyboard support
RUN mkdir -p /etc/novnc
COPY ./docker/novnc-config.js /etc/novnc/

# Integrate custom config into noVNC HTML
RUN sed -i '/<\/body>/i \    <script src="/novnc-config.js"></script>' /usr/share/noVNC/vnc.html && \
    cp /etc/novnc/novnc-config.js /usr/share/noVNC/

WORKDIR /root

RUN apt-get -y --no-install-recommends install \
     dante-server \
     net-tools && \
     apt-get clean && \
     rm -rf /var/lib/apt/lists/*

COPY ./docker/danted.conf.in /etc/
COPY ./docker/danted.sh /

# Start supervisord
EXPOSE 8080
EXPOSE 8083
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisord.conf"]

