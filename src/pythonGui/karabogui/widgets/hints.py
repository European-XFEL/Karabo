#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 10, 2020
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import QSize, Qt
from PyQt5.QtGui import QFontMetrics
from PyQt5.QtSvg import QSvgWidget
from PyQt5.QtWidgets import QFrame, QLabel, QLineEdit

from karabogui.const import WIDGET_MIN_HEIGHT, WIDGET_MIN_WIDTH
from karabogui.fonts import substitute_font


CONTENT_MARGIN = 10
MAX_SVG_SIZE = 24
SIZE_HINT_SVG = 20

MIN_STATE_SIZE = 10
SIZE_HINT_STATE_WIDTH = 30
SIZE_HINT_STATE_HEIGHT = 20


class KaraboSceneWidget:

    def __init__(self, *args, model=None, **kwargs):
        super(KaraboSceneWidget, self).__init__(*args, **kwargs)
        self.model = model
        # Check and substitute the font with the application fonts
        substitute_font(model)

    def sizeHint(self):
        """On old GUI, the geometry of the widgets in a layout are saved
        with smaller dimensions and with (0, 0) position. We only use the
        model geometry as size hint if it is the corrected one."""
        if self._has_model_pos():
            return QSize(self.model.width, self.model.height)

        return super(KaraboSceneWidget, self).sizeHint()

    def minimumSizeHint(self):
        """On old GUI, the geometry of the widgets in a layout are saved
        with smaller dimensions and with (0, 0) position. We only use the
        model geometry as size hint if it is the corrected one."""
        if self._has_model_pos():
            return QSize(self.model.width, self.model.height)

        return super(KaraboSceneWidget, self).minimumSizeHint()

    def _has_model_pos(self):
        model = self.model
        return model is not None and (model.x, model.y) != (0, 0)


class Label(QLabel):
    def __init__(self, parent=None):
        super(Label, self).__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignCenter)
        self.setWordWrap(True)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        width = fm.width(self.text()) + CONTENT_MARGIN

        return QSize(width, 20)


class LineEdit(QLineEdit):
    def __init__(self, parent=None):
        super(LineEdit, self).__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignLeft | Qt.AlignAbsolute)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        CONTENT_MARGIN = 10
        width = fm.width(self.text()) + CONTENT_MARGIN

        return QSize(width, 20)


class SvgWidget(QSvgWidget):
    def __init__(self, parent=None):
        super(SvgWidget, self).__init__(parent)
        self.setMaximumSize(MAX_SVG_SIZE, MAX_SVG_SIZE)

    def sizeHint(self):
        return QSize(SIZE_HINT_SVG, SIZE_HINT_SVG)


class FrameWidget(QLabel):
    def __init__(self, parent=None):
        super(FrameWidget, self).__init__(parent)
        self.setAutoFillBackground(True)
        self.setAlignment(Qt.AlignCenter)
        self.setMinimumWidth(MIN_STATE_SIZE)
        self.setMinimumHeight(MIN_STATE_SIZE)
        self.setWordWrap(True)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)

    def sizeHint(self):
        return QSize(SIZE_HINT_STATE_WIDTH, SIZE_HINT_STATE_HEIGHT)
