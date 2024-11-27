#!/bin/sh

echo "beginning setup..."
echo "cloning RIOT OS into ./"
set -x
git clone "https://github.com/RIOT-OS/RIOT.git" --depth 1
