#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on November 8, 2021
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

from enum import IntEnum

from qtpy.QtCore import QPoint, QRect, Qt, Signal
from qtpy.QtGui import QBrush, QPainter, QPalette, QPen
from qtpy.QtWidgets import QSlider, QStyle, QStyleOptionSlider

INT32_MAX = 2147483647
INT32_MIN = -2147483648


class SliderHandle(IntEnum):
    """Enum to indicate the active handle of a RangeSlider"""
    LOW = 0
    HIGH = 1
    SCROLL_BAR = -1


def integer(func):
    """Decorate a function to ensure integer input of the parameters"""

    def wrapper(self, *args):
        args = [int(v) for v in args]
        return func(self, *args)

    return wrapper


class RangeSlider(QSlider):
    """The RangeSlider that can have low and high handle

    <|--Low---High-------|>

    This is an adapted and simplified `RangeSlider` covering two handles
    and the enabled and disabled version.
    """
    sliderMoved = Signal(int, int)  # Reimplemented signal

    def __init__(self, orientation=Qt.Horizontal, parent=None):
        super().__init__(orientation, parent=parent)

        self.low_handle_pos = self.minimum()
        self.high_handle_pos = self.maximum()

        self.pressedControl = QStyle.SC_None
        self.clickOffset = 0
        self.pressedHandle = None
        self.setTickPosition(self.NoTicks)
        self.setFocusPolicy(Qt.ClickFocus)

    def initStyleOption(self, option):
        super().initStyleOption(option)
        option.subControls = QStyle.SC_None
        option.activeSubControls = QStyle.SC_None

    @integer
    def initialize(self, low, high):
        """Initialize the Slider with a low and high position and a new range
        """
        assert high >= low, "Maximum range must be larger than minimum range!"

        # Signed 32 bit integer protection as Qt cannot handle. Segfault ...
        if (high - low < 1
                or low <= INT32_MIN // 2
                or high >= INT32_MAX // 2):
            self.setEnabled(False)
            self.setVisible(False)
            return

        self.low_handle_pos = low
        self.high_handle_pos = high
        self.setRange(low, high)

    @integer
    def setMinimum(self, value):
        """Reimplemented method of QSlider"""
        super().setMinimum(value)
        if self.low_handle_pos < value:
            self.low_handle_pos = value
            self.update()

    @integer
    def setMaximum(self, value):
        """Reimplemented method of QSlider"""
        super().setMaximum(value)
        if self.high_handle_pos > value:
            self.high_handle_pos = value
            self.update()

    @integer
    def setValue(self, low, high):
        """Reimplemented method of QSlider

        This method safely accounts for the minimum and maximum.
        """
        # Basic level order validation
        low_position = min(low, high)
        high_position = max(low, high)
        # Protect against Qt Overflow ...
        if (low <= INT32_MIN // 2
                or high >= INT32_MAX // 2):
            return
        if low_position < self.minimum():
            low_position = self.minimum()
        if high_position > self.maximum():
            high_position = self.maximum()

        # Set values and update!
        self.low_handle_pos = low_position
        self.high_handle_pos = high_position
        self.update()

    def value(self):
        """Reimplemented method of QSlider"""
        return self.low_handle_pos, self.high_handle_pos

    setSliderPosition = setValue
    sliderPosition = value

    # -----------------------------------------------------------------------
    # Private Interface

    def pick(self, qpoint):
        """Reimplemented private function of QSlider"""
        if self.orientation() == Qt.Horizontal:
            return qpoint.x()
        else:
            return qpoint.y()

    def pixelPosToRangeValue(self, pos):
        """Reimplemented private function of QSlider"""
        style_option = QStyleOptionSlider()
        self.initStyleOption(style_option)
        style = self.style()
        gr = style.subControlRect(
            style.CC_Slider, style_option, style.SC_SliderGroove, self)
        sr = style.subControlRect(
            style.CC_Slider, style_option, style.SC_SliderHandle, self)

        if self.orientation() == Qt.Horizontal:
            sliderLength = sr.width()
            sliderMin = gr.x()
            sliderMax = gr.right() - sliderLength + 1
        else:
            sliderLength = sr.height()
            sliderMin = gr.y()
            sliderMax = gr.bottom() - sliderLength + 1

        slider_span = sliderMax - sliderMin

        return style.sliderValueFromPosition(
            self.minimum(), self.maximum(), pos - sliderMin, slider_span,
            style_option.upsideDown)

    # -----------------------------------------------------------------------
    # Paint Methods

    def paintEvent(self, event):
        """Reimplemented method of QSlider"""
        with QPainter(self) as painter:
            style = self.style()
            style_option = QStyleOptionSlider()
            self.initStyleOption(style_option)

            # Lower position
            style_option.rect = self.rect()

            # Draw the groove but no ticks
            style_option.subControls = QStyle.SC_SliderGroove
            style.drawComplexControl(QStyle.CC_Slider, style_option,
                                     painter, self)

            style_option.sliderPosition = self.low_handle_pos
            low_rect = style.subControlRect(QStyle.CC_Slider, style_option,
                                            QStyle.SC_SliderHandle)

            style_option.sliderPosition = self.high_handle_pos
            high_rect = style.subControlRect(QStyle.CC_Slider, style_option,
                                             QStyle.SC_SliderHandle)

            low_pos = self.pick(low_rect.center())
            high_pos = self.pick(high_rect.center())

            min_pos = min(low_pos, high_pos)
            max_pos = max(low_pos, high_pos)

            # Draw span rect between handles
            center = QRect(low_rect.center(), high_rect.center()).center()
            if style_option.orientation == Qt.Horizontal:
                span_rect = QRect(QPoint(min_pos, center.y() - 2),
                                  QPoint(max_pos, center.y() + 1))
            else:
                span_rect = QRect(QPoint(center.x() - 2, min_pos),
                                  QPoint(center.x() + 1, max_pos))

            groove = style.subControlRect(QStyle.CC_Slider, style_option,
                                          QStyle.SC_SliderGroove)
            if style_option.orientation == Qt.Horizontal:
                groove.adjust(0, 0, -1, 0)
            else:
                groove.adjust(0, 0, 0, -1)

            color = self.palette().color(QPalette.Highlight)
            if not self.isEnabled():
                color = color.darker(90)
            painter.setBrush(QBrush(color))
            painter.setPen(QPen(color, 0))
            painter.drawRect(span_rect.intersected(groove))

            for index, value in enumerate([self.low_handle_pos,
                                           self.high_handle_pos]):
                handle = SliderHandle(index)
                style_option.subControls = QStyle.SC_SliderHandle
                style_option.sliderPosition = value
                if handle is self.pressedHandle:
                    style_option.state |= QStyle.State_Sunken
                self.style().drawComplexControl(QStyle.CC_Slider, style_option,
                                                painter, self)

    # -----------------------------------------------------------------------
    # Mouse Events

    def mousePressEvent(self, event):
        """Reimplemented method of QSlider"""
        button = event.button()
        if button == Qt.LeftButton:
            event.accept()
            # We are pressing the slider, either handle or bar
            style_option = QStyleOptionSlider()
            self.initStyleOption(style_option)

            # Check if a dedicated handle has been pressed
            style = self.style()
            event_pos = event.pos()
            for index, value in enumerate(
                    [self.low_handle_pos, self.high_handle_pos]):
                style_option.sliderPosition = value
                hit = style.hitTestComplexControl(
                    style.CC_Slider, style_option, event_pos, self)
                if hit == style.SC_SliderHandle:
                    self.pressedControl = QStyle.SC_SliderHandle
                    self.pressedHandle = SliderHandle(index)
                    sr = style.subControlRect(
                        QStyle.CC_Slider, style_option, QStyle.SC_SliderHandle,
                        self)
                    new_pos = self.pick(event_pos - sr.topLeft())
                    self.clickOffset = self.pixelPosToRangeValue(new_pos)
                    break
            else:
                # SCROLL_BAR handles, we move the bar or a single handle!
                self.pressedHandle = SliderHandle.SCROLL_BAR
                self.pressedControl = QStyle.SC_SliderGroove
                self.clickOffset = self.pixelPosToRangeValue(
                    self.pick(event_pos))
        else:
            event.ignore()

    def mouseReleaseEvent(self, event):
        """Reimplemented method of QSlider"""
        if self.pressedControl == QStyle.SC_None:
            event.ignore()
            return

        self.pressedHandle = None
        self.pressedControl = QStyle.SC_None

    def mouseMoveEvent(self, event):
        """Reimplemented method of QSlider"""
        if self.pressedControl not in (QStyle.SC_SliderHandle,
                                       QStyle.SC_SliderGroove):
            event.ignore()
            return

        style_option = QStyleOptionSlider()
        self.initStyleOption(style_option)

        position = self.pixelPosToRangeValue(self.pick(event.pos()))
        # We are moving the full bar
        if self.pressedControl == QStyle.SC_SliderGroove:
            offset = position - self.clickOffset
            self.high_handle_pos += offset
            self.low_handle_pos += offset
            # Note: Check if we exceed a value of the range slider. This can
            # happen when the SCROLL_BAR is moved and the mouse cursor exceeds
            # the slider
            minimum = self.minimum()
            if self.low_handle_pos < minimum:
                difference = minimum - self.low_handle_pos
                self.low_handle_pos += difference
                self.high_handle_pos += difference

            maximum = self.maximum()
            if self.high_handle_pos > maximum:
                difference = maximum - self.high_handle_pos
                self.low_handle_pos += difference
                self.high_handle_pos += difference

        elif self.pressedHandle is SliderHandle.LOW:
            self.low_handle_pos = position
            # Low slider passed the high slider, align!
            if position > self.high_handle_pos:
                self.high_handle_pos = position
        else:
            self.high_handle_pos = position
            # High slider passed the low slider, align!
            if position < self.low_handle_pos:
                self.low_handle_pos = position

        event.accept()
        self.clickOffset = position
        self.update()
        self.sliderMoved.emit(self.low_handle_pos, self.high_handle_pos)
