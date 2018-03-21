from collections import namedtuple

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (
    QAction, QMessageBox, QStackedLayout, QToolButton, QWidget
)
from traits.api import Instance, List

from karabo.common.api import State, KARABO_SCHEMA_DISPLAYED_NAME
from karabo.common.scenemodel.api import DisplayCommandModel
from karabogui.const import CMD_LATCH
from karabogui import globals as krb_globals
from karabogui.binding.api import get_binding_value, SlotBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)

# An item contains the slotbinding proxy and its connected qt action
Item = namedtuple('Item', ['proxy', 'action'])


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
        action = QAction('NO TEXT', self._button)
        action.setEnabled(False)
        self._button.addAction(action)

        item = Item(proxy=proxy, action=action)
        self._actions.append(item)

        if proxy.binding is not None:
            # Only if the binding is already valid
            self._finalize_item_initialization(item)
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

    @pyqtSlot()
    def _requires_confirmation_slot(self):
        self.model.requires_confirmation = not self.model.requires_confirmation
        self._change_button_style(self.widget)

    def _change_button_style(self, widget):
        """Receives the widget button which will be modified
        :param widget: QWidget
        """
        if self.model.requires_confirmation:
            rgb = ("rgb({0}, {1}, {2})").format(*CMD_LATCH)
            widget.setStyleSheet(
                'QToolButton {{ font: bold; color: {} }}'.format(rgb))
        else:
            widget.setStyleSheet('QToolButton { color: rgb(76, 76, 76) }')

    def binding_update(self, proxy):
        for item in self._actions:
            if item.proxy is proxy and item.action.text() == 'NO TEXT':
                self._finalize_item_initialization(item)
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

    def _finalize_item_initialization(self, item):
        """When an item gets its binding, finish hooking things up."""
        proxy = item.proxy
        attributes = proxy.binding.attributes
        # if displayed name is not set, use path
        display_name = attributes.get(KARABO_SCHEMA_DISPLAYED_NAME, proxy.path)
        item.action.setText(display_name)
        item.action.triggered.connect(lambda: self._dialog_confirmation(item))
        if proxy.root_proxy.state_binding is not None:
            self.state_update(proxy)

    @pyqtSlot()
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
                                    QMessageBox.Yes
                                )

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
