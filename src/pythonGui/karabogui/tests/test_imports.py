# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from functools import partial

import karabogui as gui_pkg
from karabo.testing.import_checker import (
    check_for_disallowed_module_imports, check_for_star_imports,
    run_checker_on_package)


def test_no_star_imports():
    run_checker_on_package(gui_pkg, check_for_star_imports)


def test_no_middlelayer_imports():
    for forbidden in ("karabo.middlelayer", "karabo.middlelayer_api"):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(gui_pkg, checker)


def test_no_gui_binding_imports():
    for forbidden in ("PyQt5", "PyQt6", "PySide", "PySide2"):
        checker = partial(check_for_disallowed_module_imports, forbidden)
        run_checker_on_package(gui_pkg, checker)
