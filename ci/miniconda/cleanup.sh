#!/usr/bin/env bash

# The CI runner for MacOS is not a virtual container
# but is a physical system that is executes commands
# in a common environemnt through a gitlab runner.
# This script makes sure that the conda environment
# is completely clean.

pushd $CONDA_PREFIX
# clean the conda-bld directory
rm -fr conda-bld
mkdir conda-bld

# remove all environments
rm -fr envs
mkdir envs

popd
