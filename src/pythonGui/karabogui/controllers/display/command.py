from collections import namedtuple

from qtpy.QtWidgets import (
    QAction, QMessageBox, QStackedLayout, QToolButton, QWidget
)
from traits.api import Instance, List

from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayCommandModel
from karabogui import globals as krb_globals
from karabogui.binding.api import get_binding_value, SlotBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.indicators import LOCKED_COLOR

# An item contains the slotbinding proxy and its connected qt action
Item = namedtuple('Item', ['proxy', 'action'])
DEFAULT_TEXT = 'NO TEXT'


# XXX: Reactivate the ability to assign icon/image to the button and save it
# to the widget's data model
@register_binding_controller(ui_name='Command', klassname='DisplayCommand',
                             binding_type=SlotBinding)
class DisplayCommand(BaseBindingController):
    # The scene model class for this controller
    model = Instance(DisplayCommandModel, args=())
    # Internal traits
    _actions = List(Instance(Item))
    _button = Instance(QToolButton)

    def add_proxy(self, proxy):
        action = QAction(DEFAULT_TEXT, self._button)
        action.setEnabled(False)
        self._button.addAction(action)

        item = Item(proxy=proxy, action=action)
        self._actions.append(item)

        if proxy.binding is not None:
            # Only if the binding is already valid
            self._item_creation(item)
        return True

    def create_widget(self, parent):
        widget = QWidget(parent)

        layout = QStackedLayout(widget)
        self._button = QToolButton(widget)
        layout.addWidget(self._button)

        self.add_proxy(self.proxy)

        confirmation = QAction('Requires Confirmation', widget)
        confirmation.setCheckable(True)
        confirmation.setChecked(self.model.requires_confirmation)
        confirmation.toggled.connect(self._requires_confirmation_slot)

        self._change_button_style(widget)

        widget.addAction(confirmation)

        return widget

    def destroy_widget(self):
        """Clear the action on destruction of the command widget

        NOTE: This is required since Qt events are handled differently than
        trait events and the actions might have been deleted already when for
        example the binding update is called.
        """
        self._actions.clear()

    def _requires_confirmation_slot(self):
        self.model.requires_confirmation = not self.model.requires_confirmation
        self._change_button_style(self.widget)

    def _change_button_style(self, widget):
        """Receives the widget button which will be modified
        :param widget: QWidget
        """
        if self.model.requires_confirmation:
            rgb = ("rgb({0}, {1}, {2})").format(*LOCKED_COLOR)
            widget.setStyleSheet(
                'QToolButton {{ font: bold; color: {} }}'.format(rgb))
        else:
            widget.setStyleSheet("")

    def binding_update(self, proxy):
        for item in self._actions:
            if item.proxy is proxy:
                initial = item.action.text() == DEFAULT_TEXT
                self._item_creation(item, initial)
                break

    def state_update(self, proxy):
        updated_dev = proxy.root_proxy
        value = get_binding_value(updated_dev.state_binding, '')
        if value == '':
            return
        state = State(value)
        for item in self._actions:
            item_dev = item.proxy.root_proxy
            item_binding = item.proxy.binding
            if item_dev is updated_dev:
                is_allowed = item_binding.is_allowed(state)
                is_accessible = (krb_globals.GLOBAL_ACCESS_LEVEL >=
                                 item_binding.required_access_level)
                item.action.setEnabled(is_allowed and is_accessible)

        self._set_default_action()

    def _item_creation(self, item, initial=True):
        """When an item gets its binding or update, finish hooking things up.

        :param initial: true or false for signal connection
        :type initial: bool
        """
        proxy = item.proxy
        display_name = proxy.binding.displayed_name or proxy.path
        # if displayed name is not set, use path
        item.action.setText(display_name)
        # only initially we connect signals and slots
        if initial:
            item.action.triggered.connect(
                lambda _: self._dialog_confirmation(item))
            if proxy.root_proxy.state_binding is not None:
                self.state_update(proxy)

    def _dialog_confirmation(self, item_proxy):
        """This slot is necessary when the operator wants a dialog
        confirmation for the user when execute the button action
        :param item_proxy: It's an Item object with proxy and action
        information
        """
        if self.model.requires_confirmation:
            message_box = QMessageBox()
            message_box.setModal(False)
            confirmation = message_box.question(
                                    self.widget, 'Confirmation',
                                    'Continue with this operation?',
                                    QMessageBox.No,
                                    QMessageBox.Yes)

            if confirmation == QMessageBox.No:
                return
        item_proxy.proxy.execute()

    def _set_default_action(self):
        for a in self._button.actions():
            if a.isEnabled():
                self._button.setDefaultAction(a)
                break
        else:
            self._button.setDefaultAction(self._button.actions()[0])
