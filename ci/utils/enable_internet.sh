#!/usr/bin/env bash

PROXY_SERVER=exflwgs06.desy.de:3128/
export http_proxy=http://${PROXY_SERVER}
export https_proxy=https://${PROXY_SERVER}