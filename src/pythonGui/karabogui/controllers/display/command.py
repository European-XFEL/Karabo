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
from collections import namedtuple

from qtpy.QtWidgets import (
    QAction, QDialog, QMessageBox, QStackedLayout, QToolButton, QWidget)
from traits.api import Instance, List

import karabogui.access as krb_access
from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayCommandModel
from karabogui.binding.api import SlotBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.dialogs.api import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.indicators import LOCKED_COLOR

# An item contains the slotbinding proxy and its connected qt action
Item = namedtuple('Item', ['proxy', 'action'])
DEFAULT_TEXT = 'NO TEXT'

CONFIRM_STYLE = ('QToolButton{{font-size: {}pt; font: bold;}}'
                 'QToolButton:enabled{{color: {};}}')
TOOL_STYLE = 'QToolButton {{ font-size: {}pt; font: {} }}'


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

    def remove_proxy(self, proxy):
        """Remove an additional property proxy `proxy` from the controller"""
        item = [item for item in self._actions
                if item.proxy is proxy][0]
        self._button.removeAction(item.action)
        self._actions.remove(item)
        self._set_default_action()
        return True

    def create_widget(self, parent):
        widget = QWidget(parent)

        layout = QStackedLayout(widget)
        self._button = QToolButton(widget)
        layout.addWidget(self._button)

        self.add_proxy(self.proxy)

        confirmation = QAction("Requires Confirmation", widget)
        confirmation.setCheckable(True)
        confirmation.setChecked(self.model.requires_confirmation)
        confirmation.toggled.connect(self._requires_confirmation_slot)

        # Format
        format_action = QAction("Format field...", widget)
        format_action.triggered.connect(self._format_field)

        self._change_button_style(widget)

        widget.addAction(confirmation)
        widget.addAction(format_action)

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
        font_size = get_font_size_from_dpi(self.model.font_size)
        if self.model.requires_confirmation:
            color = ("rgb({}, {}, {})").format(*LOCKED_COLOR)
            widget.setStyleSheet(CONFIRM_STYLE.format(font_size, color))
        else:
            fw = self.model.font_weight
            widget.setStyleSheet(TOOL_STYLE.format(font_size, fw))

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
            if item_binding is None:
                # XXX: Deprecated slot on device side, but still included...
                item.action.setEnabled(False)
            elif item_dev is updated_dev:
                is_allowed = item_binding.is_allowed(state)
                is_accessible = (krb_access.GLOBAL_ACCESS_LEVEL >=
                                 item_binding.requiredAccessLevel)
                item.action.setEnabled(is_allowed and is_accessible)

        self._set_default_action()

    def setEnabled(self, enable):
        """Reimplemented to account for access level changes
        """
        for item in self._actions:
            root_proxy = item.proxy.root_proxy
            state_binding = root_proxy.state_binding
            if state_binding is None:
                item.action.setEnabled(False)
                continue
            state = get_binding_value(state_binding, "")
            if state == "":
                continue
            binding = item.proxy.binding
            if binding is None:
                # XXX: Deprecated slot on device side, but still included...
                item.action.setEnabled(False)
                continue
            is_allowed = binding.is_allowed(state)
            is_accessible = (krb_access.GLOBAL_ACCESS_LEVEL >=
                             binding.requiredAccessLevel)
            item.action.setEnabled(is_allowed and is_accessible)

        self._set_default_action()

    def _item_creation(self, item, initial=True):
        """When an item gets its binding or update, finish hooking things up.

        :param initial: true or false for signal connection
        :type initial: bool
        """
        proxy = item.proxy
        display_name = proxy.binding.displayedName or proxy.path
        # if displayed name is not set, use path
        item.action.setText(display_name)
        item.action.setToolTip(proxy.key)
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

    # -----------------------------------------------------------------------
    # Formatting methods

    def _format_field(self):
        dialog = FormatLabelDialog(font_size=self.model.font_size,
                                   font_weight=self.model.font_weight,
                                   parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(font_size=dialog.font_size,
                                 font_weight=dialog.font_weight)
            self._change_button_style(self.widget)
