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
from qtpy.QtCore import QEvent, QPoint, Qt, Slot
from qtpy.QtGui import QMouseEvent
from qtpy.QtWidgets import QStyle

from karabogui.widgets.range_slider import RangeSlider, SliderHandle


def test_range_slider_widget(gui_app):
    """Test the basic slider range widget"""

    # Test slider goofy protection
    widget = RangeSlider()
    widget.initialize(0.2, 99.8)
    widget.show()

    assert widget.minimum() == 0
    assert widget.maximum() == 99

    widget.setValue(0.12, 47.5)
    assert widget.minimum() == 0
    assert widget.maximum() == 99
    assert widget.value() == (0, 47)

    widget.setMaximum(67.123)
    assert widget.maximum() == 67

    widget.setMinimum(1.56)
    assert widget.minimum() == 1
    assert widget.value() == (1, 47)
    widget.destroy()

    # Round trip test
    low = None
    high = None

    @Slot(int, int)
    def sliderMoved(low_value, high_value):
        nonlocal low, high
        low = low_value
        high = high_value

    widget = RangeSlider()
    widget.sliderMoved.connect(sliderMoved)
    widget.initialize(0, 100)
    widget.show()

    assert widget.minimum() == 0
    assert widget.maximum() == 100
    assert widget.sliderPosition() == (0, 100)
    assert widget.value() == (0, 100)

    widget.setMaximum(120)
    assert widget.sliderPosition() == (0, 100)
    widget.setMinimum(-20)
    assert widget.sliderPosition() == (0, 100)

    # Set extremes wrong
    widget.setMaximum(90)
    assert widget.sliderPosition() == (0, 90)
    widget.setMinimum(20)
    assert widget.sliderPosition() == (20, 90)

    widget.setSliderPosition(30, 40)
    assert widget.sliderPosition() == (30, 40)

    widget.setValue(40, 50)
    assert widget.sliderPosition() == (40, 50)

    move_event = QMouseEvent(
        QEvent.MouseMove,
        QPoint(20, 0),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)

    # We need to click before this is active
    widget.mouseMoveEvent(move_event)
    assert widget.sliderPosition() == (40, 50)

    assert widget.pressedHandle is None
    assert widget.pressedControl == QStyle.SC_None
    press_event = QMouseEvent(
        QEvent.MouseButtonPress,
        QPoint(80, 0),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)

    widget.mousePressEvent(press_event)
    assert widget.pressedControl == QStyle.SC_SliderHandle
    assert widget.pressedHandle == SliderHandle.HIGH

    move_event = QMouseEvent(
        QEvent.MouseMove,
        QPoint(35, 45),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)

    # Protection, moving from high to low
    widget.mouseMoveEvent(move_event)

    assert widget.sliderPosition() == (33, 33)
    assert widget.pressedHandle == SliderHandle.HIGH
    assert low == 33
    assert high == 33

    # Spawn a groove
    widget.setValue(30, 50)
    widget.update()

    # Release event triggers unclicked
    release_event = QMouseEvent(
        QEvent.MouseButtonRelease,
        QPoint(80, 0),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)
    widget.mouseReleaseEvent(release_event)
    assert widget.pressedControl == QStyle.SC_None
    assert widget.pressedHandle is None

    press_event = QMouseEvent(
        QEvent.MouseButtonPress,
        QPoint(30, 0),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)
    widget.mousePressEvent(press_event)
    assert widget.pressedControl == QStyle.SC_SliderHandle
    assert widget.pressedHandle == SliderHandle.LOW

    move_event = QMouseEvent(
        QEvent.MouseMove,
        QPoint(35, 45),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)

    # Protection, moving from low to high
    widget.mouseMoveEvent(move_event)
    assert widget.sliderPosition() == (33, 50)
    assert widget.pressedHandle == SliderHandle.LOW
    assert low == 33
    assert high == 50

    # Release event triggers unclicked
    release_event = QMouseEvent(
        QEvent.MouseButtonRelease,
        QPoint(80, 0),
        Qt.LeftButton,
        Qt.LeftButton,
        Qt.NoModifier)
    widget.mouseReleaseEvent(release_event)
    assert widget.pressedControl == QStyle.SC_None
    assert widget.pressedHandle is None

    widget.setValue(0, 100)
    # Limits
    assert widget.value() == (20, 90)

    # Test goofy values
    widget.initialize(-999999999999999999999999999999999, 0)
    assert not widget.isEnabled()
    assert not widget.isVisible()

    widget.setEnabled(True)
    widget.setVisible(True)
    widget.initialize(0, 9999999999999999999999999999999999999999)
    assert not widget.isEnabled()
    assert not widget.isVisible()

    widget.close()
    widget.destroy()
