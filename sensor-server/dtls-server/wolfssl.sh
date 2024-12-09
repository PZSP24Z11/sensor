#!/bin/sh

# Clone the wolfssl repository
git clone https://github.com/wolfSSL/wolfssl.git --depth 1
cd wolfssl

# Run autogen script
./autogen.sh

# Configure DTLS
./configure --enable-dtls

# Build
make

# Install
sudo make install
