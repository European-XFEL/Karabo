from unittest import main

from qtpy.QtCore import QEvent, QPoint, Qt, Slot
from qtpy.QtGui import QMouseEvent
from qtpy.QtWidgets import QStyle

from karabogui.testing import GuiTestCase
from karabogui.widgets.range_slider import RangeSlider, SliderHandle


class TestConst(GuiTestCase):

    def test_basic_widget(self):
        """Test the basic slider range widget"""
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

        self.assertEqual(widget.minimum(), 0)
        self.assertEqual(widget.maximum(), 100)
        self.assertEqual(widget.sliderPosition(), (0, 100))
        self.assertEqual(widget.value(), (0, 100))

        widget.setMaximum(120)
        self.assertEqual(widget.sliderPosition(), (0, 100))
        widget.setMinimum(-20)
        self.assertEqual(widget.sliderPosition(), (0, 100))

        # Set extremes wrong
        widget.setMaximum(90)
        self.assertEqual(widget.sliderPosition(), (0, 90))
        widget.setMinimum(20)
        self.assertEqual(widget.sliderPosition(), (20, 90))

        widget.setSliderPosition(30, 40)
        self.assertEqual(widget.sliderPosition(), (30, 40))

        widget.setValue(40, 50)
        self.assertEqual(widget.sliderPosition(), (40, 50))

        move_event = QMouseEvent(
            QEvent.MouseMove,
            QPoint(20, 0),
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)

        # We need to click before this is active
        widget.mouseMoveEvent(move_event)
        self.assertEqual(widget.sliderPosition(), (40, 50))

        self.assertEqual(widget.pressedHandle, None)
        self.assertEqual(widget.pressedControl, QStyle.SC_None)
        press_event = QMouseEvent(
            QEvent.MouseButtonPress,
            QPoint(80, 0),
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)
        widget.mousePressEvent(press_event)
        self.assertEqual(widget.pressedControl, QStyle.SC_SliderHandle)
        self.assertEqual(widget.pressedHandle, SliderHandle.HIGH)

        move_event = QMouseEvent(
            QEvent.MouseMove,
            QPoint(35, 45),
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)

        # Move slightly towards the click with padding
        widget.mouseMoveEvent(move_event)
        self.process_qt_events()
        self.assertEqual(widget.sliderPosition(), (40, 33))
        self.assertEqual(widget.pressedHandle, SliderHandle.HIGH)
        self.assertEqual(low, 40)
        self.assertEqual(high, 33)

        press_event = QMouseEvent(
            QEvent.MouseButtonPress,
            QPoint(60, 0),  # In pixel coordinates
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)
        widget.mousePressEvent(press_event)
        self.process_qt_events()
        self.assertEqual(widget.pressedHandle, SliderHandle.LOW)

        # Move lower bar
        move_event = QMouseEvent(
            QEvent.MouseMove,
            QPoint(35, 35),
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)
        widget.mouseMoveEvent(move_event)

        self.process_qt_events()
        self.assertEqual(widget.sliderPosition(), (33, 33))
        self.assertEqual(low, 33)
        self.assertEqual(high, 33)

        # Deactivate with RightButton main
        press_event = QMouseEvent(
            QEvent.MouseButtonPress,
            QPoint(80, 0),
            Qt.RightButton,
            Qt.LeftButton,
            Qt.NoModifier)
        widget.mousePressEvent(press_event)
        self.assertEqual(widget.pressedControl, QStyle.SC_SliderHandle)

        # Activate again on press, click bar
        press_event = QMouseEvent(
            QEvent.MouseButtonPress,
            QPoint(80, 0),
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)
        widget.mousePressEvent(press_event)
        self.assertEqual(widget.pressedControl, QStyle.SC_SliderGroove)

        # Release event triggers unclicked
        release_event = QMouseEvent(
            QEvent.MouseButtonRelease,
            QPoint(80, 0),
            Qt.LeftButton,
            Qt.LeftButton,
            Qt.NoModifier)
        widget.mouseReleaseEvent(release_event)
        self.assertEqual(widget.pressedControl, QStyle.SC_None)
        self.assertEqual(widget.pressedHandle, None)

        widget.setValue(0, 100)
        # Limits
        self.assertEqual(widget.value(), (20, 90))

        widget.close()
        widget.destroy()


if __name__ == "__main__":
    main()