from functools import partial

from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, check_for_star_imports,
    run_checker_on_package
)
import karabogui as gui_pkg


def test_no_star_imports():
    run_checker_on_package(gui_pkg, check_for_star_imports)


def test_no_old_imports():
    checker = partial(check_for_disallowed_module_imports, 'karabo_gui')
    run_checker_on_package(gui_pkg, checker, skip_tests=False)
