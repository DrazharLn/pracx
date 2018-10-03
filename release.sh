#!/bin/sh

echo "Tag a release and put on github"

set -e

version="$1"

git tag "$version"
git push --tags
hub release create "$version" -a bin/prac.dll -a bin/prax.dll -a bin/pracxpatch.exe -a bin/PRACX.v"$version".exe -F "resources/PRACX Change Log.txt" -e
