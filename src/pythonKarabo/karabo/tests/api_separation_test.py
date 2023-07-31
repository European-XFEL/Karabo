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
from functools import partial

import karabo.bound_api as bound_pkg
import karabo.common as common_pkg
import karabo.middlelayer as middlelayer_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, check_for_star_imports,
    run_checker_on_package)


def test_bound_no_middlelayer_imports():
    for forbidden in ['karabo.middlelayer']:
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(bound_pkg, checker)


def test_common_no_bound_imports():
    for forbidden in ['karabo.bound', 'karabo.bound_api']:
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(common_pkg, checker)


def test_common_no_middlelayer_imports():
    for forbidden in ['karabo.middlelayer']:
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(common_pkg, checker)


def test_common_no_star_imports():
    run_checker_on_package(common_pkg, check_for_star_imports)


def test_middlelayer_no_bound_imports():
    for forbidden in ['karabo.bound', 'karabo.bound_api']:
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(middlelayer_pkg, checker)
