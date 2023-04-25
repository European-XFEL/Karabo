# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pyqtgraph import ViewBox
from qtpy.QtCore import Qt

from karabogui.graph.common.api import KaraboViewBox, MouseTool


class KaraboImageViewBox(KaraboViewBox):
    def __init__(self, parent=None):
        super(KaraboImageViewBox, self).__init__(parent)
        self.setBackgroundColor(None)

    # ---------------------------------------------------------------------
    # mouse events

    def mouseDragEvent(self, event, axis=None):
        if (self.mouse_mode is MouseTool.Picker
                and event.buttons() == Qt.LeftButton):
            event.ignore()
        else:
            super(KaraboImageViewBox, self).mouseDragEvent(event, axis)

    # ---------------------------------------------------------------------
    # Public methods

    def set_mouse_tool(self, mode):
        if mode is MouseTool.Picker:
            vb_mode = ViewBox.RectMode
            cursor = Qt.PointingHandCursor
        else:
            super(KaraboImageViewBox, self).set_mouse_tool(mode)
            return

        self.setMouseMode(vb_mode)
        self.setCursor(cursor)
        self.mouse_mode = mode
