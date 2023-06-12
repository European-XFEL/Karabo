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
from pyqtgraph import ViewBox
from qtpy.QtCore import Qt

from karabogui.graph.common.api import KaraboViewBox, MouseTool


class KaraboImageViewBox(KaraboViewBox):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setBackgroundColor(None)

    # ---------------------------------------------------------------------
    # mouse events

    def mouseDragEvent(self, event, axis=None):
        if (self.mouse_mode is MouseTool.Picker
                and event.buttons() == Qt.LeftButton):
            event.ignore()
        else:
            super().mouseDragEvent(event, axis)

    # ---------------------------------------------------------------------
    # Public methods

    def set_mouse_tool(self, mode):
        if mode is MouseTool.Picker:
            vb_mode = ViewBox.RectMode
            cursor = Qt.PointingHandCursor
        else:
            super().set_mouse_tool(mode)
            return

        self.setMouseMode(vb_mode)
        self.setCursor(cursor)
        self.mouse_mode = mode
