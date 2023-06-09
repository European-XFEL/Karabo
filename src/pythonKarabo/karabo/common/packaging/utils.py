# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import re

_BASE = r"\d+\.\d+\.\d+"
_RELEASE = r"(a|b|rc)\d+"
_DEV = r"\.dev\d+"

_BASE_VERSION_REGEX = re.compile(f"^({_BASE})({_RELEASE})?")
_FULL_VERSION_REGEX = re.compile(
    f"^({_BASE})({_RELEASE}({_DEV})?)?"
)


def _apply_regex(regex, version):
    result = regex.match(version)
    if result:
        return result.group()
    else:
        result = ""
    return result


def extract_base_version(version: str) -> str:
    """
    Extracts the base version of a setuptools_scm version string.
    """
    return _apply_regex(_BASE_VERSION_REGEX, version)


def extract_full_version(version: str) -> str:
    """
    Extracts the full version of a setuptools_scm version string.
    """
    return _apply_regex(_FULL_VERSION_REGEX, version)
