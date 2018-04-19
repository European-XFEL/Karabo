from karabo.testing.import_checker import (
    check_for_star_imports, run_checker_on_package
)
import karabogui as gui_pkg


def test_no_star_imports():
    run_checker_on_package(gui_pkg, check_for_star_imports)
