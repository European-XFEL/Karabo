# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
