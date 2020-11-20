#!/bin/bash

set -e

if [ -d _build_ci ] ; then
        rm -rf _build_ci
fi
mkdir _build_ci

pushd _build_ci


cmake -DCMAKE_BUILD_TYPE=Release ..
echo "======================================"
echo "                Build"
echo "======================================"
cmake --build .

make

echo "======================================"
echo "         Running unit tests"
echo "======================================"
echo

ctest -V

popd

echo "Test run has finished successfully"
