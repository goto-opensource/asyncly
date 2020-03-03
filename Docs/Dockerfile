FROM ubuntu:20.04
MAINTAINER jupp.mueller@logmein.com

RUN apt-get update
RUN apt-get install -y --no-install-recommends curl doxygen python3-minimal python3-sphinx python3-sphinx-rtd-theme tar xz-utils liblua5.2
RUN mkdir /doxyrest; curl -sL https://github.com/vovkos/doxyrest/releases/download/doxyrest-2.0.0/doxyrest-2.0.0-linux-amd64.tar.xz | tar xJf - -C /doxyrest --strip-components=1

ENV DOXYREST_DIR /doxyrest
ENV DOXYREST_BIN_DIR ${DOXYREST_DIR}/bin
ENV DOXYGEN_XML_DIR /doxygen_out/xml
ENV TMP_RST_DIR /tmp
ENV OUTPUT_HTML_DIR /build/_docs

ENV DOXYREST_SPHINX_DIR ${DOXYREST_DIR}/share/doxyrest/sphinx
ENV DOXYREST_FRAME_DIR ${DOXYREST_DIR}/share/doxyrest/frame

ENTRYPOINT /build/Docs/build_docs.sh
