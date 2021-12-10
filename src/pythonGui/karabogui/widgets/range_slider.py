#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on November 8, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from enum import IntEnum

from qtpy.QtCore import QPoint, QRect, Qt, Signal
from qtpy.QtGui import QBrush, QPainter, QPalette, QPen
from qtpy.QtWidgets import QAbstractSlider, QSlider, QStyle, QStyleOptionSlider

CLICK_TOLERANCE = 0.10  # In percent when moving bars by click


class SliderHandle(IntEnum):
    """Enum to indicate the active handle of a RangeSlider"""
    LOW = 0
    HIGH = 1
    SCROLL_BAR = -1


class RangeSlider(QSlider):
    """The RangeSlider that can have low and high handle

    <|--Low---High-------|>

    This is an adapted and simplified `RangeSlider` covering two handles
    and the enabled and disabled version.
    """
    sliderMoved = Signal(int, int)  # Reimplemented signal

    def __init__(self, orientation=Qt.Horizontal, parent=None):
        super().__init__(orientation, parent=parent)

        self._low_position = self.minimum()
        self._high_position = self.maximum()

        self.pressedControl = QStyle.SC_None
        self.clickOffset = 0
        self.pressedHandle = None
        self.setTickPosition(self.NoTicks)

    def initStyleOption(self, option):
        super().initStyleOption(option)
        option.subControls = QStyle.SC_None
        option.activeSubControls = QStyle.SC_None

    def initialize(self, low, high):
        """Initialize the Slider with a low and high position and a new range
        """
        assert high > low, "Maximum range must be larger than minimum range!"
        self._low_position = low
        self._high_position = high
        self.setRange(low, high)

    def setMinimum(self, value):
        """Reimplemented method of QSlider"""
        super().setMinimum(value)
        if self._low_position < value:
            self._low_position = value
            self.update()

    def setMaximum(self, value):
        """Reimplemented method of QSlider"""
        super().setMaximum(value)
        if self._high_position > value:
            self._high_position = value
            self.update()

    def setValue(self, *value):
        """Reimplemented method of QSlider

        This method safely accounts for the minimum and maximum.
        """
        # Check for tuple or value input
        low, high = value if len(value) > 1 else value[0]

        # Basic level order validation
        low_position = min(low, high)
        high_position = max(low, high)
        if low_position < self.minimum():
            low_position = self.minimum()
        if high_position > self.maximum():
            high_position = self.maximum()

        # Set values and update!
        self._low_position = low_position
        self._high_position = high_position
        self.update()

    def value(self):
        """Reimplemented method of QSlider"""
        return self._low_position, self._high_position

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

    @property
    def offset_position(self):
        return self.minimum() - 1e6

    def paintEvent(self, event):
        """Reimplemented method of QSlider"""
        with QPainter(self) as painter:
            style = self.style()
            style_option = QStyleOptionSlider()
            self.initStyleOption(style_option)

            # Lower position
            style_option.rect = self.rect()

            # Note: This is a Mac rendering problem, it would paint an
            # additional slider. This is fixed by Qt. 5.15 !
            style_option.sliderPosition = self.offset_position
            style_option.sliderValue = self.offset_position

            # Draw the groove but no ticks
            style_option.subControls = QStyle.SC_SliderGroove
            style.drawComplexControl(QStyle.CC_Slider, style_option,
                                     painter, self)

            style_option.sliderPosition = self._low_position
            low_rect = style.subControlRect(QStyle.CC_Slider, style_option,
                                            QStyle.SC_SliderHandle)

            style_option.sliderPosition = self._high_position
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

            for index, value in enumerate([self._low_position,
                                           self._high_position]):
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
                    [self._low_position, self._high_position]):
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
                    self.setSliderDown(True)
                    self.triggerAction(self.SliderNoAction)
                    self.setRepeatAction(self.SliderNoAction)
                    break
            else:
                # SCROLL_BAR handles, we move the bar or a single handle!
                self.pressedHandle = SliderHandle.SCROLL_BAR
                self.pressedControl = QStyle.SC_SliderGroove
                self.clickOffset = self.pixelPosToRangeValue(
                    self.pick(event_pos))
                self.triggerAction(self.SliderMove)
                self.setRepeatAction(self.SliderNoAction)
        else:
            event.ignore()

    def mouseReleaseEvent(self, event):
        """Reimplemented method of QSlider"""
        if self.pressedControl == QStyle.SC_None:
            event.ignore()
            return

        if self.pressedHandle in (SliderHandle.LOW, SliderHandle.HIGH):
            self.setSliderDown(False)

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
            self._high_position += offset
            self._low_position += offset
            # Note: Check if we exceed a value of the range slider. This can
            # happen when the SCROLL_BAR is moved and the mouse cursor exceeds
            # the slider
            minimum = self.minimum()
            if self._low_position < minimum:
                difference = minimum - self._low_position
                self._low_position += difference
                self._high_position += difference

            maximum = self.maximum()
            if self._high_position > maximum:
                difference = maximum - self._high_position
                self._low_position += difference
                self._high_position += difference

        elif self.pressedHandle is SliderHandle.LOW:
            self._low_position = position
        else:
            self._high_position = position

        event.accept()
        self.clickOffset = position
        self.update()
        self.sliderMoved.emit(self._low_position, self._high_position)

    # -----------------------------------------------------------------------
    # Actions

    def triggerAction(self, action):
        """Reimplemented method of QSlider"""
        if action is QAbstractSlider.SliderMove:
            slider_range = self.maximum() - self.minimum()
            tolerance = slider_range * CLICK_TOLERANCE
            if abs(self.clickOffset - self._high_position) <= tolerance:
                value = min(self.clickOffset, self.maximum() - 1)
                self._high_position = value
                self.update()
            elif abs(self.clickOffset - self._low_position) <= tolerance:
                value = max(self.clickOffset, self.minimum() + 1)
                self._low_position = value
                self.update()
        else:
            super().triggerAction(action)
