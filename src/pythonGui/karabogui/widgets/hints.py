#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 10, 2020
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QLocale, QSize, Qt
from qtpy.QtGui import QFontMetrics
from qtpy.QtSvg import QSvgWidget
from qtpy.QtWidgets import QDoubleSpinBox, QFrame, QLabel, QLineEdit, QSpinBox

from karabogui.const import WIDGET_MIN_HEIGHT, WIDGET_MIN_WIDTH
from karabogui.fonts import substitute_font

DEFAULT_SIZE_HINT = 20
CONTENT_MARGIN = 10

SVG_MAX_SIZE = 24

STATE_MIN_SIZE = 10
STATE_WIDTH_HINT = 30

LOCALE = QLocale("en_US")


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
        size = QSize()
        if self._has_model_pos():
            size = QSize(self.model.width, self.model.height)
        if size.isEmpty():
            size = super(KaraboSceneWidget, self).sizeHint()
        return size

    def minimumSizeHint(self):
        """On old GUI, the geometry of the widgets in a layout are saved
        with smaller dimensions and with (0, 0) position. We only use the
        model geometry as size hint if it is the corrected one."""
        size = QSize()
        if self._has_model_pos():
            size = QSize(self.model.width, self.model.height)
        if size.isEmpty():
            size = super(KaraboSceneWidget, self).minimumSizeHint()
        return size

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

        return QSize(width, DEFAULT_SIZE_HINT)


class LineEdit(QLineEdit):
    def __init__(self, parent=None):
        super(LineEdit, self).__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignLeft | Qt.AlignAbsolute)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        width = fm.width(self.text()) + CONTENT_MARGIN

        return QSize(width, DEFAULT_SIZE_HINT)


class SpinBox(QSpinBox):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignLeft | Qt.AlignAbsolute)
        self.setLocale(LOCALE)
        self.setFocusPolicy(Qt.StrongFocus)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        width = fm.width(self.cleanText()) + CONTENT_MARGIN
        return QSize(width, DEFAULT_SIZE_HINT)


class DoubleSpinBox(QDoubleSpinBox):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignLeft | Qt.AlignAbsolute)
        self.setLocale(LOCALE)
        self.setFocusPolicy(Qt.StrongFocus)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        width = fm.width(self.cleanText()) + CONTENT_MARGIN
        return QSize(width, DEFAULT_SIZE_HINT)


class SvgWidget(QSvgWidget):
    def __init__(self, parent=None):
        super(SvgWidget, self).__init__(parent)
        self.setMaximumSize(SVG_MAX_SIZE, SVG_MAX_SIZE)

    def sizeHint(self):
        return QSize(DEFAULT_SIZE_HINT, DEFAULT_SIZE_HINT)


class FrameWidget(QLabel):
    def __init__(self, parent=None):
        super(FrameWidget, self).__init__(parent)
        self.setAutoFillBackground(True)
        self.setAlignment(Qt.AlignCenter)
        self.setMinimumWidth(STATE_MIN_SIZE)
        self.setMinimumHeight(STATE_MIN_SIZE)
        self.setWordWrap(True)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)

    def sizeHint(self):
        return QSize(STATE_WIDTH_HINT, DEFAULT_SIZE_HINT)
