#!/usr/bin/env bash

if [[ -z "${PROXY_SERVER}" ]]; then
    PROXY_SERVER=exflwgs06.desy.de:3128/
fi
if [[ -z "${http_proxy}" ]]; then
    export http_proxy=http://${PROXY_SERVER}
fi
if [[ -z "${https_proxy}" ]]; then
    # setting as https_proxy=http://something
    # since the version of curl and conda might be incompatible with the
    # SSL configuration of the proxy.
    # https://github.com/curl/curl/issues/2324
    export https_proxy=http://${PROXY_SERVER}
fi
