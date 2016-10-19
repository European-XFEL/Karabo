from functools import partial

import karabo.middlelayer_api as middlelayer_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, run_checker_on_package)


def test_middlelayer_no_bound_imports():
    for forbidden in ('karabo.bound', 'karabo.bound_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(middlelayer_pkg, checker)
