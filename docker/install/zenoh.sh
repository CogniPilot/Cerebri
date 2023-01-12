#!/bin/bash
set -e
WGET_ARGS="-q --show-progress --progress=bar:force:noscroll --no-check-certificate"
ZENOH_VERSION="0.7.0-rc"

mkdir -p /tmp/zenoh
cd /tmp/zenoh
wget ${WGET_ARGS} https://github.com/eclipse-zenoh/zenoh/releases/download/${ZENOH_VERSION}/zenoh-${ZENOH_VERSION}-x86_64-unknown-linux-gnu.zip
wget ${WGET_ARGS} https://github.com/eclipse-zenoh/zenoh-plugin-dds/releases/download/${ZENOH_VERSION}/zplugin-dds-${ZENOH_VERSION}-x86_64-unknown-linux-gnu.zip
cd /tmp/zenoh
unzip zenoh-${ZENOH_VERSION}-x86_64-unknown-linux-gnu.zip
unzip zplugin-dds-${ZENOH_VERSION}-x86_64-unknown-linux-gnu.zip
sudo install ./zenohd /usr/local/bin
sudo install ./libzplugin_rest.so /usr/local/bin
sudo install ./libzplugin_storage_manager.so /usr/local/bin
sudo rm -rf /tmp/zenoh

cd ~
python3 -m venv ~/.venv-zenoh
source ~/.venv-zenoh/bin/activate
pip3 install eclipse-zenoh==${ZENOH_VERSION} cyclonedds pycdr2 
