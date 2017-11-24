from collections import namedtuple

from PyQt4.QtGui import QAction, QStackedLayout, QToolButton, QWidget
from traits.api import Instance, List, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayCommandModel
from karabogui import globals as krb_globals
from karabogui.binding.api import SlotBinding, KARABO_SCHEMA_DISPLAYED_NAME
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller


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
        self._button.addAction(action)

        item = Item(proxy=proxy, action=action)
        self._actions.append(item)

        if proxy.binding:
            # Only if the binding is already valid
            self._finalize_item_initialization(item)

        state_binding = proxy.root_proxy.state_binding
        if state_binding:
            state_binding.on_trait_change(self._state_update, 'value')
            self._state_update(state_binding, 'value', state_binding.value)

        return True

    def create_widget(self, parent):
        widget = QWidget(parent)

        layout = QStackedLayout(widget)
        self._button = QToolButton(widget)
        layout.addWidget(self._button)

        self.add_proxy(self.proxy)
        return widget

    def destroy_widget(self):
        for proxy in self.proxies:
            state_binding = proxy.root_proxy.state_binding
            if state_binding:
                state_binding.on_trait_change(self._state_update, 'value',
                                              remove=True)

    @on_trait_change('proxies:binding', post_init=True)
    def _binding_update(self, obj, name, new):
        """Handle the arrival of new proxy bindings.

        `obj` is a PropertyProxy instance, `name` is 'binding', and `new` is
        a BaseBindingType instance.
        """
        for item in self._actions:
            if item.proxy is obj and item.action.text() == 'NO TEXT':
                self._finalize_item_initialization(item)
                break

    def _finalize_item_initialization(self, item):
        """When an item gets its binding, finish hooking things up."""
        proxy = item.proxy
        if proxy.binding is None:
            return

        attributes = proxy.binding.attributes
        # if displayed name is not set, use path
        display_name = attributes.get(KARABO_SCHEMA_DISPLAYED_NAME, proxy.path)
        item.action.setText(display_name)
        item.action.triggered.connect(item.proxy.execute)

    def _state_update(self, obj, name, new):
        """This is a trait change handler for a `state.value` trait, so `obj`
        is the parent object of the trait which changed (`state`), `name` is
        the name of the trait ('value') and `new` is the new value of the
        trait (a state string).
        """
        state = State(new)
        for item in self._actions:
            proxy = item.proxy
            if proxy.root_proxy.state_binding is obj:
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
