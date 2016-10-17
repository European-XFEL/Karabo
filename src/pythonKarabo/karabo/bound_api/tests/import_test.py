from functools import partial

import karabo.bound_api as bound_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, run_checker_on_package)


def test_bound_no_middlelayer_imports():
    for forbidden in ('karabo.middlelayer', 'karabo.middlelayer_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(bound_pkg, checker)
