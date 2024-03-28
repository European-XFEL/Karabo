# -*- coding: utf-8 -*-
"""
Modified compare script from conda as the `compare` feature is not yet released
and the comparison of the current environment matching the specs file is what
Karabo needs (the inverse of the current implementatoion).

Original script (conda/cli/main_compare.py)
# Copyright (C) 2012 Anaconda, Inc
# SPDX-License-Identifier: BSD-3-Clause

Modified by carinanc
"""

from __future__ import (
    absolute_import, division, print_function, unicode_literals)

import logging
import os

from conda.core.prefix_data import PrefixData
from conda_env import specs

try:
    from conda_env.exceptions import SpecNotFound
except ImportError:
    try:
        from conda_env.specs import SpecNotFound
    except ImportError:
        from conda.exceptions import SpecNotFound

from conda.models.match_spec import MatchSpec

from .utils import get_conda_prefix

# Add a new level such that conda will not complain
TRACE = 5  # TRACE LOG LEVEL
logging.addLevelName(TRACE, "TRACE")
logging.Logger.trace = lambda message, *args, **kwargs: None


def get_packages(prefix):
    if not os.path.isdir(prefix):
        from conda.exceptions import EnvironmentLocationNotFound
        raise EnvironmentLocationNotFound(prefix)

    return sorted(PrefixData(prefix, pip_interop_enabled=True).iter_records(),
                  key=lambda x: x.name)


def _get_name_tuple(pkg):
    return pkg.name, pkg


def _to_str(pkg):
    return "%s==%s=%s" % (pkg.name, pkg.version, pkg.build)


def compare_packages(active_pkgs, specification_pkgs):
    output = []
    res = 0
    ok = True
    for name, pkg in active_pkgs.items():
        if name in specification_pkgs:
            spec = specification_pkgs[name]
            if not spec.match(pkg):
                ok = False
                output.append(
                    "{} found but mismatch. Specification pkg: {}, Running pkg: {}"
                    .format(name, str(spec), _to_str(pkg)))
        else:
            ok = False
            output.append("{} not found".format(name))
    if ok:
        output.append("Success. All the packages in the \
specification file are present in the environment \
with matching version and build string.")
    else:
        res = 1
    return res, output


def execute(env_name, filename):
    # 1. Get activate packages from the specified environment name
    # 1.1 Get the environment prefix (absolute path) from the name
    prefix = get_conda_prefix(env_name)
    # Check if prefix is still None:
    if prefix is None:
        message = f"Cannot compare, the environment {env_name} can't be found."
        raise RuntimeError(message)

    # 1.2 Get the list of active packages
    active_pkgs = dict(map(_get_name_tuple, get_packages(prefix)))

    # 2. Get specification packages
    # 2.1. Detect the environment specs from the requirements file
    try:
        spec = specs.detect(filename=filename)
        env = spec.environment
    except SpecNotFound:
        message = f"Cannot compare, the file {filename} cannot be read."
        raise RuntimeError(message)

    # 2.2. Get the dependencies from the specifications
    specification_pkgs = []
    if 'conda' in env.dependencies:
        specification_pkgs = specification_pkgs + env.dependencies['conda']
    if 'pip' in env.dependencies:
        specification_pkgs = specification_pkgs + env.dependencies['pip']

    # 2.3 Create a dictionary with {name: MatchSpec(pkg)}
    spec_pkgs = {}
    for pkg in specification_pkgs:
        match_spec = MatchSpec(pkg)
        spec_pkgs[match_spec.name] = match_spec

    # 3. Compare activate and specification packages
    exitcode, output = compare_packages(active_pkgs, spec_pkgs)
    return exitcode, output
