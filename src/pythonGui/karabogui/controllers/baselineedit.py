from qtpy.QtCore import Qt
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QLineEdit
from traits.api import Instance, Int, String, Undefined

from karabogui.binding.api import get_min_max
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.controllers.util import is_proxy_allowed
from karabogui.util import generateObjectName
from karabogui.widgets.hints import LineEdit

FINE_COLOR = "black"
ERROR_COLOR = "red"


class BaseLineEditController(BaseBindingController):
    """The `BaseLineEditController` class provides a visual indication
    if a set text provides acceptable input validated by a subclassed
     `validator` instance.
    """
    model = Undefined
    # The line edit widget instance
    internal_widget = Instance(QLineEdit)
    # The validator instance
    validator = Instance(QValidator)
    # The last updated value from the device
    internal_value = String("")
    # The displayed value
    display_value = String("")
    # The last cursor position
    last_cursor_pos = Int(0)

    # Private properties
    _style_sheet = String("")

    def create_widget(self, parent):
        self.internal_widget = LineEdit(parent)
        self.internal_widget.setValidator(self.validator)
        objectName = generateObjectName(self)
        self._style_sheet = ("QWidget#{}".format(objectName) +
                             " {{ color: {}; }}")
        self.internal_widget.setObjectName(objectName)
        sheet = self._style_sheet.format(FINE_COLOR)
        self.internal_widget.setStyleSheet(sheet)
        widget = add_unit_label(self.proxy, self.internal_widget,
                                parent=parent)
        widget.setFocusProxy(self.internal_widget)
        return widget

    def set_read_only(self, ro):
        self.internal_widget.setReadOnly(ro)
        if not ro:
            self.internal_widget.textChanged.connect(self._on_text_changed)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self.internal_widget.setFocusPolicy(focus_policy)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.internal_widget.setEnabled(enable)

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        self.validator.setBottom(low)
        self.validator.setTop(high)
        self.widget.update_unit_label(proxy)

    def on_decline(self):
        """When the input was declined, this method is executed"""
        sheet = self._style_sheet.format(FINE_COLOR)
        self.internal_widget.setStyleSheet(sheet)

    # Private interface
    # ---------------------------------------------------------------------

    def _on_text_changed(self, text):
        acceptable_input = self.internal_widget.hasAcceptableInput()
        if self.proxy.binding is None:
            sheet = self._style_sheet.format(FINE_COLOR)
            self.internal_widget.setStyleSheet(sheet)
            return
        if acceptable_input:
            self.internal_value = text
            self.last_cursor_pos = self.internal_widget.cursorPosition()
            # proxy.edit_value is set to None if the user input is not valid
            self.proxy.edit_value = self.validate_value()
        else:
            # erase the edit value
            self.proxy.edit_value = None
        # update color after text change!
        color = FINE_COLOR if acceptable_input else ERROR_COLOR
        sheet = self._style_sheet.format(color)
        self.internal_widget.setStyleSheet(sheet)

    # Public interface
    # ---------------------------------------------------------------------

    def validate_text_color(self):
        """Public method to validate the text color"""
        acceptable_input = self.internal_widget.hasAcceptableInput()
        if self.proxy.binding is None:
            sheet = self._style_sheet.format(FINE_COLOR)
            self.internal_widget.setStyleSheet(sheet)
            return
        color = FINE_COLOR if acceptable_input else ERROR_COLOR
        sheet = self._style_sheet.format(color)
        self.internal_widget.setStyleSheet(sheet)

    def validate_value(self):
        """Subclass method for controller to validate the internal value

        Note: This method validates the internal value of the widget
        and returns the value (success) or `None` (failure).
        """
        if not self.internal_value:
            return None

        value = self.internal_value
        state, _, _ = self.validator.validate(value, 0)
        if state == QValidator.Invalid:
            value = None
        return value
