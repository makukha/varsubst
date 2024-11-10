FROM python:3.13.0-bookworm
SHELL ["/bin/bash", "-eux", "-o", "pipefail", "-c"]

ARG PACKCC_VERSION=2.0.2
ARG PACKCC_URL=https://github.com/arithy/packcc/releases/download/v${PACKCC_VERSION}/packcc-${PACKCC_VERSION}-linux-x64.tar.gz
ARG PACKCC_SHA256=8a9ce59c3e1be47e2f095572abd0244c2604c6b3583bd2c63ad09c38384b817a
ENV PCC_IMPORT_PATH /usr/local/share/packcc

ARG TASK_VERSION=3.39.2
ARG TASK_URL=https://github.com/go-task/task/releases/download/v${TASK_VERSION}/task_linux_amd64.deb
ARG TASK_SHA256=d4ea84524cc4e0969e84f063fc99b9aba2090ec259c00a2e12d9ed1a182cf6af

WORKDIR /build
COPY requirements.txt ./

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    --mount=type=cache,dst=/root/.cache/pip \
<<EOT
apt-get update

# linux dev tools
apt-get install -y --no-install-recommends ccache gdb jq wget

# packcc
wget -qO /tmp/packcc.tgz "${PACKCC_URL}"
echo "${PACKCC_SHA256} /tmp/packcc.tgz" | sha256sum --check
tar -C /tmp -xf /tmp/packcc.tgz
mv /tmp/packcc-*/bin/packcc /usr/local/bin
mv /tmp/packcc-*/import ${PCC_IMPORT_PATH}

# task
wget -qO /tmp/task.deb "${TASK_URL}"
echo "${TASK_SHA256} /tmp/task.deb" | sha256sum --check
apt-get install -y /tmp/task.deb

# python dev tools
pip install -r requirements.txt

rm -rf /tmp/* /var/tmp/*
EOT

VOLUME /root/.ccache
ENTRYPOINT []
