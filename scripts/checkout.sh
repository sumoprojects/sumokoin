#! /bin/bash
read -p "This will destroy all wallet files outside of test-wallets dir. Are you sure? (y/n)" -n 1 -r
echo
if ! [[ $REPLY =~ ^[Yy]$ ]]; then
    exit 1
fi

set -x
git fetch
git reset --hard origin/dev
rm -rf build

if [ $# -gt 1 ]; then
    git pull $1 $2
else
    echo "!!! Doing vanilla dev build without any pulls"
    sleep 10
fi

make -j
cd build/release/bin
ln -s ../../../test-wallets/* .
