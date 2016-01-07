#!/bin/bash

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
fi

$KARABO/bin/.bundle-pythonplugin.sh "$@"

