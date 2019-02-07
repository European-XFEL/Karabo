from collections import namedtuple

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QComboBox, QDialog, QHBoxLayout, QToolButton, QWidget)
from traits.api import Bool, Instance, Int, on_trait_change, String

from karabo.common.scenemodel.api import EditableOptionComboBoxModel
from karabogui import icons, messagebox
from karabogui.binding.api import (
    get_binding_value, get_editor_value, StringBinding)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.dialogs.optionsedit import OptionsEditDialog
from karabogui.util import MouseWheelEventBlocker, SignalBlocker


Messages = namedtuple("Messages", ["tooltip", "dialog"])

CLEARED_OPTIONS = Messages(
    tooltip=('It was detected that the options are removed. '
             'The value "{value}" is now added as an option.'),
    dialog=None)

INJECTED_OPTIONS = Messages(
    tooltip=('The property was injected with options. '
             'This will now be used instead.'),
    dialog=('<b>{proxy}</b> was injected with options. '
            'This will now be used instead.<br>'
            'Consider using <i>Selection Field</i> widget for this property.'))

VALUE_NOT_IN_OPTIONS = Messages(
    tooltip=('The value "{value}" is not in the options. '
             'It will now be added.'),
    dialog=('The value <b>"{value}"</b> is not in the options of '
            '<b>{proxy}</b>. It will now be added.'))


def _is_compatible(binding):
    return len(binding.options) == 0


@register_binding_controller(ui_name='Option Field', can_edit=True,
                             klassname='EditableOptionComboBox',
                             binding_type=StringBinding,
                             is_compatible=_is_compatible, priority=0)
class EditableOptionComboBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(EditableOptionComboBoxModel, args=())

    # Internal traits
    _internal_widget = Instance(QComboBox)
    _edit_button = Instance(QToolButton)
    _filter = Instance(MouseWheelEventBlocker)
    _current_index = Int(0)
    _current_binding_value = String
    _is_injected_with_options = Bool(False)
    _is_edit_value_changed_internally = Bool(False)

    def create_widget(self, parent):
        composite_widget = QWidget(parent)

        self._internal_widget = QComboBox()
        self._internal_widget.setFrame(False)

        options = self.model.options
        if options:
            self._internal_widget.addItems([str(o) for o in options])

        self._filter = MouseWheelEventBlocker(self._internal_widget)
        self._internal_widget.installEventFilter(self._filter)
        self._internal_widget.currentIndexChanged.connect(self._on_user_edit)

        self._edit_button = QToolButton()
        self._edit_button.setIcon(icons.edit)
        self._edit_button.setMaximumSize(25, 25)
        self._edit_button.setFocusPolicy(Qt.NoFocus)
        self._edit_button.clicked.connect(self._on_edit_clicked)

        layout = QHBoxLayout(composite_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._internal_widget)
        layout.addWidget(self._edit_button)
        composite_widget.setFocusProxy(self._internal_widget)

        return composite_widget

    def binding_update(self, proxy):
        if not len(proxy.binding.options):
            if not self.model.options:
                # No existing model, usually it is a widget initialization.
                # Use the binding value as the choice if not None.
                value = get_binding_value(proxy)
                if value is not None:
                    self.model.options = [value]
            elif self._is_injected_with_options:
                # If previously injected but the options seems to be cleared,
                # reset the widget.
                if self._is_current_value_not_in_options(self.model.options):
                    self.model.options.append(self._current_binding_value)
                    self._set_combobox_tooltip(CLEARED_OPTIONS.tooltip.format(
                        value=self._current_binding_value))
                self._is_injected_with_options = False
        else:
            # Options are injected in the property. Show a warning and use
            # as the combobox choices instead.
            new_options = proxy.binding.options
            same_choices = (set(self.model.options) == set(new_options))
            if self._is_current_value_not_in_options(new_options):
                new_options.append(self._current_binding_value)
            self.model.options = new_options
            self._is_injected_with_options = True

            if not same_choices:
                self._set_combobox_tooltip(INJECTED_OPTIONS.tooltip)
                messagebox.show_warning(
                    INJECTED_OPTIONS.dialog.format(proxy=self.proxy.key),
                    parent=self._internal_widget)

        self._change_edit_button_state()

    def value_update(self, proxy):
        value = self._get_current_edit_value(proxy)
        if value is None:
            return

        if value not in self.model.options:
            self.model.options.append(value)
            self._set_combobox_tooltip(VALUE_NOT_IN_OPTIONS.tooltip
                                       .format(value=value))
            if self._current_binding_value:
                # Show messagebox if not widget initialization
                messagebox.show_warning(
                    VALUE_NOT_IN_OPTIONS.dialog.format(value=value,
                                                       proxy=self.proxy.key),
                    parent=self._internal_widget)
        else:
            self._set_current_index(value)
            self._set_option_as_edit_value(value)

        binding_value = get_binding_value(proxy, None)
        if binding_value is not None:
            self._current_binding_value = binding_value

    @on_trait_change('model.options')
    def _options_changed(self):
        if self._internal_widget is None:
            return

        # If value is None, use tracked current value. This is triggered by
        # initialization and schema injection
        value = get_binding_value(self.proxy, self._current_binding_value)

        # If there is a value but no options, set value as option.
        if value and not self.model.options:
            self.model.options = [value]
            return

        # Reset edit value changes
        self.proxy.edit_value = None
        self._is_edit_value_changed_internally = False

        with SignalBlocker(self._internal_widget):
            self._internal_widget.clear()
            self._internal_widget.addItems([str(o) for o
                                            in self.model.options])
        self._set_current_index(value)
        self._set_option_as_edit_value(value)
        self._set_combobox_tooltip(self.proxy.key, is_status=False)

    def _change_edit_button_state(self):
        if self._is_injected_with_options:
            message = ("The property was injected with options. "
                       "Editing is disabled.")
        else:
            message = "Edit the choices for this string."

        self._edit_button.setStatusTip(message)
        self._edit_button.setToolTip(message)
        self._edit_button.setDisabled(self._is_injected_with_options)

    @pyqtSlot(int)
    def _on_user_edit(self, index):
        if self.proxy.binding is None:
            return
        self._is_edit_value_changed_internally = False
        self.proxy.edit_value = self.model.options[index]
        self._set_combobox_tooltip(self.proxy.key, is_status=False)

    @pyqtSlot()
    def _on_edit_clicked(self):
        if self.proxy.binding is None:
            return

        options_edit = OptionsEditDialog(self.model.options)
        if options_edit.exec_() == QDialog.Accepted:
            self.model.options = options_edit.values

        # Give back the focus!
        self._internal_widget.setFocus(Qt.PopupFocusReason)

    def _set_option_as_edit_value(self, value):
        current_option = (self.model.options[self._current_index]
                          if len(self.model.options) else None)

        # We only change edit value when there is a binding value.
        if value:
            if current_option != value:
                # Change edit value to the current option. This is usually
                # invoked with internal device changes. The force change is
                # now then tracked.
                self.proxy.edit_value = current_option
                self._is_edit_value_changed_internally = True
            elif get_binding_value(self.proxy) == value:
                # Reset edit value changes
                self.proxy.edit_value = None
                self._is_edit_value_changed_internally = False

        with SignalBlocker(self._internal_widget):
            self._internal_widget.setCurrentIndex(self._current_index)

    def _set_current_index(self, value):
        try:
            # Set the index of the combobox to the (edit) value
            self._current_index = self.model.options.index(value)
        except ValueError:
            # The value is not among the options. This is most likely due to
            # internal device change and options change from injection/dialog)
            num_options = len(self.model.options)
            if num_options == 0:
                # No options, default to zero index
                self._current_index = 0
            elif self._current_index >= num_options:
                # Default to the last item of combobox
                self._current_index = num_options - 1

    def _set_combobox_tooltip(self, message, is_status=True):
        status = message if is_status else ""
        self._internal_widget.setToolTip(message)
        self._internal_widget.setStatusTip(status)

    def _get_current_edit_value(self, proxy):
        binding_value = get_binding_value(proxy, None)
        if (self._current_binding_value != binding_value
                or self._is_edit_value_changed_internally):
            return binding_value
        else:
            return get_editor_value(proxy)

    def _is_current_value_not_in_options(self, options):
        return (self._current_binding_value and
                self._current_binding_value not in options)
