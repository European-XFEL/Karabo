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
# flake8: noqa
from qtpy import QtModuleNotInstalledError

from .hints import (
    DoubleSpinBox, ElidingLabel, FrameWidget, Label, LineEdit, SpinBox,
    SvgWidget)
from .popup import TextPopupWidget
from .range_slider import RangeSlider
from .toolbar import ToolBar

try:
    from .scintilla_editor import CodeBook
except QtModuleNotInstalledError:
    # for Qt5 compatibility
    from .codeeditor import CodeBook
