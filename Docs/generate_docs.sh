#!/usr/bin/env bash

set -euo pipefail

readonly PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && /bin/pwd -P)

docker build "${PROJECT_ROOT}" -t executordocs
docker run -v "${PROJECT_ROOT}/..":/build --rm executordocs
docker rmi executordocs
