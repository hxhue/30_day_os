#!/bin/bash

# This should be run in WSL.

set -xe
~/kati/ckati --ninja
perl -pe 's/\\\\\s+/ /g' build.ninja > build.ninja.1
mv build.ninja.1 build.ninja
ninja -t compdb > compile_commands.json