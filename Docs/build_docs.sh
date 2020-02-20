#!/usr/bin/env bash

set -o errexit
set -x

cd /build
cp Docs/conf.py ${TMP_RST_DIR}
doxygen
${DOXYREST_BIN_DIR}/doxyrest $DOXYGEN_XML_DIR/index.xml -o $TMP_RST_DIR/index.rst -F $DOXYREST_FRAME_DIR -f c_index.rst.in
sphinx-build -b html $TMP_RST_DIR $OUTPUT_HTML_DIR
