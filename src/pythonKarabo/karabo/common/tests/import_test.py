from functools import partial

import karabo.common as common_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, check_for_star_imports,
    run_checker_on_package)


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
