from functools import partial
from platform import system
from unittest import skipIf

import karabogui as gui_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, check_for_star_imports,
    run_checker_on_package)


@skipIf(system() in ("Darwin"), reason="segfault on mac ")  # FIXME
def test_no_star_imports():
    run_checker_on_package(gui_pkg, check_for_star_imports)


@skipIf(system() in ("Darwin"), reason="segfault on mac ")  # FIXME
def test_no_middlelayer_imports():
    for forbidden in ('karabo.middlelayer', 'karabo.middlelayer_api'):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(gui_pkg, checker)
