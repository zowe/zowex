#!/bin/bash
set -e

# Generate resource files with random data
if [ ! -d "resources" ]; then
  mkdir -p resources
  dd if=/dev/urandom of=resources/test10k.bin bs=1K count=10
  dd if=/dev/urandom of=resources/test100k.bin bs=1K count=100
  dd if=/dev/urandom of=resources/test1m.bin bs=1M count=1
  dd if=/dev/urandom of=resources/test10m.bin bs=1M count=10
  dd if=/dev/urandom of=resources/test100m.bin bs=1M count=100
  dd if=/dev/urandom of=resources/test1g.bin bs=1M count=1000
fi

# Create config file if it doesn't exist
if [ ! -f "config.json" ]; then
  cp config.example.json config.json
  echo "Update the properties defined in config.json. Then re-run this script to deploy and build the test binaries."
  exit
fi

# Deploy C++ test sources and build them
ussDir=$(jq -r .ussDir config.json)
zosmfProfile=$(jq -r .zosmfProfile config.json)
npx zowe files upload ftu testb64.cpp ${ussDir}/testb64.cpp --binary --zosmf-p $zosmfProfile
npx zowe files upload ftu testb85.cpp ${ussDir}/testb85.cpp --binary --zosmf-p $zosmfProfile
npx zowe files upload ftu testraw.cpp ${ussDir}/testraw.cpp --binary --zosmf-p $zosmfProfile
sshCmd="cd ${ussDir}; for f in testb64 testb85 testraw; do ibm-clang++64 -std=c++17 -o \$f \$f.cpp; done"
sshProfile=$(jq -r .sshProfile config.json)
if [ "$sshProfile" != "null" ]; then
  npx zowe uss issue ssh "$sshCmd" --ssh-p $sshProfile
else
  echo
  echo "Connect to the ${zosmfProfile} system and run this command:"
  echo "  $sshCmd"
fi
