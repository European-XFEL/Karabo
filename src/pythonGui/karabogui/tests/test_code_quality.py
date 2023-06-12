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
import os.path as op
import subprocess
import sys

import karabogui


def test_code_quality_flake8():
    # Run flake8 as a module from the current Python interpreter.
    # Running 'flake8' directly assumes that flake8 is in the PATH,
    # which sometimes is not.
    command = [sys.executable, '-m', 'flake8',
               op.dirname(op.abspath(karabogui.__file__))]
    subprocess.check_call(command)
