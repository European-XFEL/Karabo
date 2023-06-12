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


def get_dialog_ui(ui_name):
    """Retrieve the appropriate `.ui` file from the dialog ui folder
    """
    assert ui_name.endswith(".ui"), f"{ui_name} is not a `.ui` file"

    return op.join(op.abspath(op.dirname(__file__)), "ui", ui_name)
