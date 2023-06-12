#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 10, 2020
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
        """Check and substitute the font with the application fonts"""
        self.model = model
        substitute_font(model)
        super().__init__(*args, **kwargs)

    def sizeHint(self):
        """On old GUI, the geometry of the widgets in a layout are saved
        with smaller dimensions and with (0, 0) position. We only use the
        model geometry as size hint if it is the corrected one."""
        size = QSize()
        if self._has_model_pos():
            size = QSize(self.model.width, self.model.height)
        if size.isEmpty():
            size = super().sizeHint()
        return size

    def minimumSizeHint(self):
        """On old GUI, the geometry of the widgets in a layout are saved
        with smaller dimensions and with (0, 0) position. We only use the
        model geometry as size hint if it is the corrected one."""
        size = QSize()
        if self._has_model_pos():
            size = QSize(self.model.width, self.model.height)
        if size.isEmpty():
            size = super().minimumSizeHint()
        return size

    def _has_model_pos(self):
        model = self.model
        return model is not None and (model.x, model.y) != (0, 0)


class Label(QLabel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignCenter)
        self.setWordWrap(True)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        width = fm.width(self.text()) + CONTENT_MARGIN

        return QSize(width, DEFAULT_SIZE_HINT)


class ElidingLabel(QLabel):
    """A label that elides if the text and margins are larger than the width
    """

    def __init__(self, parent=None):
        self._text = ""
        self._elided = False
        super().__init__(self._text, parent=parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignCenter)

    def isElided(self):
        """Return if the text is elided or not"""
        return self._elided

    def sizeHint(self):
        """Reimplemented function of `QLabel` for the size hint calculation"""
        fm = QFontMetrics(self.font())
        width = fm.width(self.text()) + CONTENT_MARGIN
        return QSize(width, WIDGET_MIN_HEIGHT)

    @property
    def text_width(self):
        return self.width() - CONTENT_MARGIN

    def setText(self, text):
        """Reimplemented function of `QLabel` to adjust the display text"""
        self._text = text
        text_width = self.text_width
        fm = QFontMetrics(self.font())
        elided = False
        if fm.width(text) > text_width:
            text = fm.elidedText(text, Qt.ElideRight, text_width)
            elided = True
        self._elided = elided
        if text != self.text():
            super().setText(text)

    def resizeEvent(self, event):
        """Reimplemented function of `QLabel` to adjust the display text"""
        super().resizeEvent(event)
        self.setText(self._text)


class LineEdit(QLineEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
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
        super().__init__(parent)
        self.setMaximumSize(SVG_MAX_SIZE, SVG_MAX_SIZE)

    def sizeHint(self):
        return QSize(DEFAULT_SIZE_HINT, DEFAULT_SIZE_HINT)


class FrameWidget(QLabel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAutoFillBackground(True)
        self.setAlignment(Qt.AlignCenter)
        self.setMinimumWidth(STATE_MIN_SIZE)
        self.setMinimumHeight(STATE_MIN_SIZE)
        self.setWordWrap(True)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)

    def sizeHint(self):
        return QSize(STATE_WIDTH_HINT, DEFAULT_SIZE_HINT)
