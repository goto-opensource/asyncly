FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV DOXYREST_DIR /doxyrest
ENV DOXYREST_BIN_DIR ${DOXYREST_DIR}/bin
ENV DOXYGEN_XML_DIR /doxygen_out/xml
ENV TMP_RST_DIR /tmp
ENV OUTPUT_HTML_DIR /build/_docs

ENV DOXYREST_SPHINX_DIR ${DOXYREST_DIR}/share/doxyrest/sphinx
ENV DOXYREST_FRAME_DIR ${DOXYREST_DIR}/share/doxyrest/frame

RUN apt-get update && apt-get install -y --no-install-recommends curl doxygen python3-minimal python3-sphinx python3-sphinx-rtd-theme tar xz-utils liblua5.2
RUN mkdir ${DOXYREST_DIR}
RUN curl -sOL https://github.com/vovkos/doxyrest/releases/download/doxyrest-2.1.3/doxyrest-2.1.3-linux-amd64.tar.xz
RUN tar -xJf doxyrest-2.1.3-linux-amd64.tar.xz -C /doxyrest --strip-components=1

ENTRYPOINT /build/Docs/build_docs.sh
