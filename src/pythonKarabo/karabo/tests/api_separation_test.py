# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from functools import partial

import karabo.bound_api as bound_pkg
import karabo.common as common_pkg
import karabo.macro_api as macro_pgk
import karabo.middlelayer_api as middlelayer_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, check_for_star_imports,
    run_checker_on_package)


def test_bound_no_middlelayer_imports():
    for forbidden in ('karabo.middlelayer', 'karabo.middlelayer_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(bound_pkg, checker)


def test_bound_no_macro_imports():
    for forbidden in ('karabo.macro', 'karabo.macro_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(bound_pkg, checker)


def test_common_no_bound_imports():
    for forbidden in ('karabo.bound', 'karabo.bound_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(common_pkg, checker)


def test_common_no_middlelayer_imports():
    for forbidden in ('karabo.middlelayer', 'karabo.middlelayer_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(common_pkg, checker)


def test_common_no_star_imports():
    run_checker_on_package(common_pkg, check_for_star_imports)


def test_middlelayer_no_bound_imports():
    for forbidden in ('karabo.bound', 'karabo.bound_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(middlelayer_pkg, checker)


def test_middlelayer_no_macro_imports():
    for forbidden in ('karabo.macro', 'karabo.macro_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(middlelayer_pkg, checker)


def test_macro_no_bound_imports():
    for forbidden in ('karabo.bound', 'karabo.bound_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(macro_pgk, checker)
