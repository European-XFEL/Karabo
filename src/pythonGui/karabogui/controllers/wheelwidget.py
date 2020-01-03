#############################################################################
##
# This file is part of Karabo (http://karabo.eu)
##
# Copyright European XFEL, Schenefeld, Germany
##
## This file was largely inspired by ``Taurus`` (http://taurus-scada.org)
#
#############################################################################

import math

import numpy as np
from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QRect, QSize
from PyQt5.QtWidgets import (
    QAbstractButton, QButtonGroup, QGridLayout, QLabel, QLineEdit, QPushButton,
    QSizePolicy)

from karabogui import icons
from karabogui.controllers.validators import NumberValidator


class Orientation(object):
    UP = 0
    DOWN = 1


BUTTON_SIZE = 20
ICON_SIZE = BUTTON_SIZE - 8
MINIMUM_HEIGHT = 60
FRAME_WIDTH = 20
TOP_ROW = 0
MIDDLE_ROW = 1
BOTTOM_ROW = 2
_DEFAULT_INTEGERS = 6
_DEFAULT_DECIMALS = 3


class WheelButton(QPushButton):
    def __init__(self, identifier, orientation=Orientation.UP, parent=None):
        super(WheelButton, self).__init__(parent)
        if orientation == Orientation.UP:
            self.increment = math.pow(10, identifier)
            self.setIcon(icons.arrowFancyUp)
        elif orientation == Orientation.DOWN:
            self.increment = -math.pow(10, identifier)
            self.setIcon(icons.arrowFancyDown)
        self.setIconSize(QSize(ICON_SIZE, ICON_SIZE))
        self.setFocusPolicy(Qt.ClickFocus)
        self.setStyleSheet(
            """QPushButton {
                        border: none;
                        outline: none;
                        }""")
        self.setFixedSize(BUTTON_SIZE, BUTTON_SIZE)


class DigitFrame(QLabel):
    """A private single digit label to be used by DoubleWheelEdit widget"""

    def __init__(self, text, parent=None):
        super(DigitFrame, self).__init__(parent)
        self.setAlignment(Qt.AlignCenter)
        self.setFocusPolicy(Qt.StrongFocus)
        self.setMinimumWidth(20)
        self.setStyleSheet(
            """QLabel:focus {background-color: rgba(40,90,160, 90);
             }""")
        self.setText(text)
        self.button_up = None
        self.button_down = None

    def setButtons(self, up, down):
        self.button_up = up
        self.button_down = down

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Up:
            self.button_up.click()
        elif event.key() == Qt.Key_Down:
            self.button_down.click()
        elif event.key() == Qt.Key_Right:
            self.focusNextChild()
        elif event.key() == Qt.Key_Left:
            self.focusPreviousChild()

        super(DigitFrame, self).keyPressEvent(event)


class EditWidget(QLineEdit):
    """The plain editor in the wheel edit widget

    Allows to edit and apply the parameter with a numpad editor
    """

    def __init__(self, parent=None):
        super(EditWidget, self).__init__(parent)
        self.setValidator(NumberValidator(parent=self))
        self.setFrame(False)


class DoubleWheelEdit(QLabel):
    """A widget designed to handle double values.

    It allows interaction based on single digit as well as normal
    value edition.
    """

    returnPressed = pyqtSignal()
    valueChanged = pyqtSignal(float)
    configurationChanged = pyqtSignal(int)

    def __init__(self, integers=_DEFAULT_INTEGERS, decimals=_DEFAULT_DECIMALS,
                 parent=None):
        super(DoubleWheelEdit, self).__init__(parent)
        self.layout = QGridLayout(self)
        self.layout.setSpacing(0)
        self.layout.setMargin(0)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.setSizePolicy(QSizePolicy.MinimumExpanding,
                           QSizePolicy.MinimumExpanding)

        # Initialize default settings, we show all zeros when we get a None
        self.value = 0
        self.string_value = '0'

        # The limits that are derived from the wheel settings (integers ...)
        self.total_maximum = None
        self.total_minimum = None

        # From value point of view
        self._value_maximum = None
        self._value_minimum = None
        # From the widget constellation
        self._frames_minimum = None
        self._frames_maximum = None

        self.editor_widget = None
        self.editing = False

        self.integers = integers
        self.decimals = decimals
        self._set_digit_format(integers, decimals)

        # Build our button groups
        self.button_group_up = QButtonGroup(parent=self)
        self.button_group_down = QButtonGroup(parent=self)
        self.button_group_up.buttonClicked.connect(self.buttonPressed)
        self.button_group_down.buttonClicked.connect(self.buttonPressed)
        self._build_button_group()
        self._create_editor()

    def _create_editor(self):
        """Build the edit widget overlaying the wheelbox"""
        if self.editor_widget is not None:
            self.editor_widget.setParent(None)
            self.editor_widget.destroy()
            self.editor_widget = None

        self.editor_widget = EditWidget(parent=self)
        self.editor_widget.setVisible(False)
        self.editor_widget.returnPressed.connect(self.editingFinished)
        self.editor_widget.editingFinished.connect(self.editor_widget.hide)
        rect = QRect(
            self.layout.cellRect(1, 0).topLeft(),
            self.layout.cellRect(
                1, self.layout.columnCount() - 1).bottomRight())
        self.editor_widget.setGeometry(rect)
        self.editor_widget.setAlignment(Qt.AlignRight)
        validator = self.editor_widget.validator()
        validator.decimals = self.decimals
        validator.setBottom(self.total_minimum)
        validator.setTop(self.total_maximum)

    def _build_button_group(self):
        """Builds this widget sub-items"""
        self.digit_frames = []

        sign_label = DigitFrame('+', parent=self)
        sign_label.setFocusPolicy(Qt.NoFocus)
        sign_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.digit_frames.append(sign_label)
        self.layout.addWidget(sign_label, 1, 0)

        # NOTE: This should not be required!
        self.layout.setRowMinimumHeight(
            1, sign_label.minimumSizeHint().height())
        self.layout.setColumnMinimumWidth(0, BUTTON_SIZE)

        # Required otherwise the widget is split!
        self.layout.setColumnStretch(0, 1)

        for index in range(self.integers):
            column = index + 1
            digit_widget = DigitFrame('0', parent=self)
            button_up = WheelButton(self.integers - index - 1,
                                    orientation=Orientation.UP)
            button_down = WheelButton(self.integers - index - 1,
                                      orientation=Orientation.DOWN)
            digit_widget.setButtons(button_up, button_down)
            button_up.setFocusProxy(digit_widget)
            button_down.setFocusProxy(digit_widget)
            self.button_group_up.addButton(button_up, column)
            self.button_group_down.addButton(button_down, column)
            self.digit_frames.append(digit_widget)

            # show arrow buttons
            self.layout.addWidget(button_up, TOP_ROW, column)
            self.layout.addWidget(button_down, BOTTOM_ROW, column)
            self.layout.addWidget(digit_widget, MIDDLE_ROW, column)

        if self.decimals > 0:
            dot_widget = DigitFrame('.', parent=self)
            dot_widget.setFocusPolicy(Qt.NoFocus)
            dot_widget.setAlignment(Qt.AlignCenter)
            self.digit_frames.append(dot_widget)
            self.layout.addWidget(dot_widget, MIDDLE_ROW, self.integers + 1)

        for index in range(self.integers, self.number_count):
            column = index + 1
            if self.decimals > 0:
                column += 1
            widget = DigitFrame('0', parent=self)
            button_up = WheelButton(self.integers - index - 1,
                                    orientation=Orientation.UP)
            button_down = WheelButton(self.integers - index - 1,
                                      orientation=Orientation.DOWN)
            widget.setButtons(button_up, button_down)
            button_up.setFocusProxy(widget)
            button_down.setFocusProxy(widget)
            self.button_group_up.addButton(button_up, column)
            self.button_group_down.addButton(button_down, column)
            self.digit_frames.append(widget)

            # show arrow buttons
            self.layout.addWidget(button_up, TOP_ROW, column)
            self.layout.addWidget(button_down, BOTTOM_ROW, column)
            self.layout.addWidget(widget, MIDDLE_ROW, column)

    def _clear_button_group(self):
        """This function clears the button group buttons"""
        for button in self.button_group_up.buttons():
            self.button_group_up.removeButton(button)
            button.setParent(None)
            button.destroy()

        for button in self.button_group_down.buttons():
            self.button_group_down.removeButton(button)
            button.setParent(None)
            button.destroy()

        for widget in self.digit_frames:
            widget.setParent(None)
            widget.destroy()

        self.digit_frames = []

    def _rebuild_wheelwidget(self):
        """Updates widget contents (sub-widgets) to current configuration"""
        self._clear_button_group()
        self._build_button_group()
        self._create_editor()

    # Value related methods
    # ----------------------------------------------------------------------

    def _set_digit_format(self, integers=None, decimals=None):
        """Set the digits format to show in the widget

        Generates the eventual limits for this widget and builds a new
        string value with the format!
        """
        self.integers = integers if integers is not None else self.integers
        self.decimals = decimals if decimals is not None else self.decimals

        # make sure that the current value can be displayed
        self.integers = max(self.integers, len('%d' % self.value))

        self.number_count = self.integers + self.decimals

        # NOTE: Account for the sign and the dot!
        total_chars = self.number_count + 1
        if self.decimals > 0:
            total_chars += 1

        self.value_format = '%%+0%d.%df' % (total_chars, self.decimals)
        self._generate_limits()

        # NOTE: We must update the string representation!
        self._create_string_from_format(self.value)

        # Don't forget to account for minimum size with sign and dot
        width = total_chars * FRAME_WIDTH
        self.setMinimumSize(width, MINIMUM_HEIGHT)

    def _generate_limits(self):
        """Generate the possible value limits for this widget"""
        decimal_max = 0
        for i in range(self.decimals):
            decimal_max += 9 * math.pow(10, -(i + 1))
        self._frames_minimum = -math.pow(10.0, self.integers) + 1 - decimal_max
        self._frames_maximum = math.pow(10.0, self.integers) - 1 + decimal_max

        # Reset the total maximum and minimum for the validators!
        if self._value_minimum is not None:
            self.total_minimum = max(self._frames_minimum, self._value_minimum)
        else:
            self.total_minimum = self._frames_minimum
        if self._value_maximum is not None:
            self.total_maximum = min(self._frames_maximum, self._value_maximum)
        else:
            self.total_maximum = self._frames_maximum

    def _create_string_from_format(self, value):
        """Create the string representation of the value to the format"""
        ret = self.value_format % value
        if ret.endswith('nan'):
            ret = ret.replace('0', ' ')
        self.string_value = ret

    def _set_frame_value(self):
        """Update internally the displayed value of the widget

        This method will set the string value in the digit frames and can
        evaluate if the value can be represented. If this is not the case,
        the widget gets adjusted.
        """
        value, string_value = self.value, self.string_value

        # NOTE: We evaluate if we can represent the integers
        # If we cannot, we have to update the widget!
        if len(string_value) > len(self.digit_frames):
            integers = len(string_value.split('.')[0])
            self._set_digit_format(integers=integers)
            self._create_string_from_format(value)
            self.configurationChanged.emit(integers)
            self._rebuild_wheelwidget()

        for index, text in enumerate(string_value):
            self.digit_frames[index].setText(text)

    # --------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot(int)
    def set_number_integers(self, value):
        self._set_digit_format(integers=value)
        self._rebuild_wheelwidget()
        self._set_frame_value()

    @pyqtSlot(int)
    def set_number_decimals(self, value):
        self._set_digit_format(decimals=value)
        self._rebuild_wheelwidget()
        self._set_frame_value()

    @pyqtSlot()
    def editingFinished(self):
        value = float(self.editor_widget.text())
        self.valueChanged.emit(value)

    @pyqtSlot(QAbstractButton)
    def buttonPressed(self, button):
        """Executed when an arrow button is pressed from the button group"""
        num_value = np.nan_to_num(self.value)
        # XXX: Make sure we remove NaN value here
        value = num_value + button.increment
        # Recast to the format we have and validate against the minimum and
        # maximum of the widget before sending!
        value = float(self.value_format % value)
        if self.total_minimum > value or self.total_maximum < value:
            return

        self.valueChanged.emit(value)

    # Public interface
    # ---------------------------------------------------------------

    def set_value_limits(self, low, high):
        """This function"""
        self._value_minimum = low
        self._value_maximum = high
        self._generate_limits()
        if self.editor_widget is not None:
            validator = self.editor_widget.validator()
            validator.setBottom(self.total_minimum)
            validator.setTop(self.total_maximum)

    def set_integer_decimal_configuration(self, integers, decimals):
        self._set_digit_format(integers, decimals)
        self._rebuild_wheelwidget()
        self._set_frame_value()

    def set_value(self, value, external=False):
        """Sets value of this widget and calculate the string value!

        If the value exceeds the value limit, the value is NOT set.
        """
        if value is None:
            return

        # Depending on where we receive the value, we have to validate
        # differently. An external applied value is validated against our
        # total boundaries, as the widget can adjust. An internal value
        # is only validated against the current value extrema, as it should
        # not exceed the limits.
        value_min = self.total_minimum if external else self._value_minimum
        value_max = self.total_maximum if external else self._value_maximum

        # NOTE: We again check here the value maximum and minimum
        if value_min is not None and value_min > value:
            return
        elif value_max is not None and value_max < value:
            return

        self.value = value
        self._create_string_from_format(value)
        self._set_frame_value()

    # Editor
    # ---------------------------------------------------------------

    def showEditWidget(self):
        """Show the number editor"""
        widget = self.editor_widget
        layout = self.layout
        rect = QRect(
            layout.cellRect(1, 0).topLeft(),
            layout.cellRect(1, layout.columnCount() - 1).bottomRight())
        widget.setGeometry(rect)
        widget.setAlignment(Qt.AlignRight)
        widget.setMaxLength(self.number_count + 2)
        widget.setText(self.string_value)
        widget.selectAll()
        widget.setFocus()
        widget.setVisible(True)
        widget.activateWindow()

    def hide_edit_widget(self):
        """Hide the number editor"""
        self.editor_widget.setVisible(False)
        self.setFocus()

    # Qt Events
    # ---------------------------------------------------------------

    def mouseDoubleClickEvent(self, event):
        self.showEditWidget()
        self.editing = True
        super(DoubleWheelEdit, self).mouseDoubleClickEvent(event)

    def keyPressEvent(self, event):
        key = event.key()
        if key == Qt.Key_Escape:
            if self.editing:
                self.hide_edit_widget()
                self.editing = False
                event.accept()

        elif key in (Qt.Key_Return, Qt.Key_Enter):
            if self.editing:
                self.hide_edit_widget()
                self.editing = False
            else:
                self.returnPressed.emit()
                event.accept()

        super(DoubleWheelEdit, self).keyPressEvent(event)
