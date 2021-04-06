from qtpy.QtCore import Qt
from pyqtgraph import ViewBox

from karabogui.graph.common.api import KaraboViewBox, MouseMode


class KaraboImageViewBox(KaraboViewBox):
    def __init__(self, parent=None):
        super(KaraboImageViewBox, self).__init__(parent)
        self.setBackgroundColor(None)

    # ---------------------------------------------------------------------
    # mouse events

    def mouseDragEvent(self, event, axis=None):
        if (self.mouse_mode is MouseMode.Picker
                and event.buttons() == Qt.LeftButton):
            event.ignore()
        else:
            super(KaraboImageViewBox, self).mouseDragEvent(event, axis)

    # ---------------------------------------------------------------------
    # Public methods

    def set_mouse_mode(self, mode):
        if mode is MouseMode.Picker:
            vb_mode = ViewBox.RectMode
            cursor = Qt.PointingHandCursor
        else:
            super(KaraboImageViewBox, self).set_mouse_mode(mode)
            return

        self.setMouseMode(vb_mode)
        self.setCursor(cursor)
        self.mouse_mode = mode
