from collections import namedtuple

from PyQt4.QtGui import QAction, QStackedLayout, QToolButton, QWidget
from traits.api import Instance, List

from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayCommandModel
from karabogui import globals as krb_globals
from karabogui.binding.api import SlotBinding, KARABO_SCHEMA_DISPLAYED_NAME
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
    model = Instance(DisplayCommandModel)
    # Internal traits
    _actions = List(Instance(Item))
    _button = Instance(QToolButton)

    def add_proxy(self, proxy):
        action = QAction('NO TEXT', self._button)
        action.setEnabled(False)
        self._button.addAction(action)

        item = Item(proxy=proxy, action=action)
        self._actions.append(item)

        if proxy.binding:
            # Only if the binding is already valid
            self._finalize_item_initialization(item)

        return True

    def create_widget(self, parent):
        widget = QWidget(parent)

        layout = QStackedLayout(widget)
        self._button = QToolButton(widget)
        layout.addWidget(self._button)

        self.add_proxy(self.proxy)
        return widget

    def binding_update(self, proxy):
        for item in self._actions:
            if item.proxy is proxy and item.action.text() == 'NO TEXT':
                self._finalize_item_initialization(item)
                break

    def state_update(self, proxy):
        state = State(proxy.root_proxy.state_binding.value)
        for item in self._actions:
            if item.proxy is proxy:
                is_allowed = proxy.binding.is_allowed(state)
                is_accessible = (krb_globals.GLOBAL_ACCESS_LEVEL >=
                                 proxy.binding.required_access_level)
                item.action.setEnabled(is_allowed and is_accessible)

        for a in self._button.actions():
            if a.isEnabled():
                self._button.setDefaultAction(a)
                break
        else:
            self._button.setDefaultAction(self._button.actions()[0])

    def _finalize_item_initialization(self, item):
        """When an item gets its binding, finish hooking things up."""
        proxy = item.proxy
        attributes = proxy.binding.attributes
        # if displayed name is not set, use path
        display_name = attributes.get(KARABO_SCHEMA_DISPLAYED_NAME, proxy.path)
        item.action.setText(display_name)
        item.action.triggered.connect(item.proxy.execute)
