#!/usr/bin/env bash

set -o errexit
set -x

cd /build
cp Docs/conf.py ${TMP_RST_DIR}
doxygen
${DOXYREST_BIN_DIR}/doxyrest -c Docs/doxyrest-config.lua
sphinx-build -b html $TMP_RST_DIR $OUTPUT_HTML_DIR
cp logo_transparent.png $OUTPUT_HTML_DIR
