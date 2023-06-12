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
from qtpy.QtWidgets import QAction, QDialog
from traits.api import Instance, Type, on_trait_change

from karabo.common.scenemodel.api import (
    DigitIconsModel, SelectionIconsModel, TextIconsModel)
from karabogui.binding.api import (
    BaseBinding, FloatBinding, IntBinding, StringBinding, get_binding_value)
from karabogui.controllers.api import (
    BaseBindingController, has_options, register_binding_controller)
from karabogui.controllers.icons_dialogs import (
    DigitDialog, IconItem, IconLabel, SelectionDialog, TextDialog)

NUMERICAL_BINDINGS = (FloatBinding, IntBinding)
MINIMUM_SIZE = (24, 24)


class _BaseIcons(BaseBindingController):

    current_item = Instance(IconItem)

    def create_widget(self, parent):
        widget = IconLabel(parent)
        action = QAction('Change Icons...', widget)
        action.triggered.connect(self._on_change_icons)
        widget.addAction(action)

        return widget

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if value is None:
            self.current_item = None
            return

        current_item = None
        for item in self.model.values:
            if self.condition(value, item):
                current_item = item
                break

        # Update the current item and invoke the pixmap changes
        self.current_item = current_item

    def condition(self, value, item):
        """Specifies the condition in which the widget will change the icon
           into.

           :param value: proxy value. Can be int, float, or string
           :param item: IconItem value from the collection self.model.values"""
        raise NotImplementedError("The subclass must specify the condition "
                                  "for the icon changes.")

    @on_trait_change('model.values')
    def _force_update(self):
        """Handle updates to the `IconData` collection for all subclasses.
        We make sure to convert from plain IconData => IconItem here.
        """
        def _state(item):
            NAMES = ['equal', 'value', 'image', 'data']
            return {k: getattr(item, k) for k in NAMES}

        converted = [IconItem(**_state(it))
                     if not isinstance(it, IconItem) else it
                     for it in self.model.values]
        self.model.trait_setq(values=converted)

        # Maybe update the widget
        if (self.widget is not None and
                get_binding_value(self.proxy) is not None):
            # Store a reference for the current item
            current_item = self.current_item

            self.value_update(self.proxy)

            # Check if current item is still the same and only the image
            # has changed, we set the pixmap manually
            if current_item is not None and self.current_item is current_item:
                self.widget.setPixmap(current_item.pixmap)

    def _on_change_icons(self):
        binding = self.proxy.binding
        dialog = self.dialog_klass(self.model.values, binding,
                                   parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.values = dialog.items
            # The trait handler won't fire as are using a list, hence, we force
            # an update!
            self._force_update()

    def _current_item_changed(self, item):
        if item is None:
            self.widget.setDefaultPixmap()
        else:
            self.widget.setPixmap(item.pixmap)


@register_binding_controller(ui_name='Icons', binding_type=NUMERICAL_BINDINGS,
                             klassname='DigitIcons')
class DigitIcons(_BaseIcons):
    model = Instance(DigitIconsModel, args=())
    dialog_klass = Type(DigitDialog)

    def condition(self, value, item):
        """Check if the proxy value satisfy the comparison of one of
           the options."""
        return (value < float(item.value)
                or value == float(item.value)
                and item.equal)


@register_binding_controller(ui_name='Selection Icons',
                             klassname='SelectionIcons',
                             binding_type=BaseBinding,
                             is_compatible=has_options)
class SelectionIcons(_BaseIcons):
    model = Instance(SelectionIconsModel, args=())
    dialog_klass = Type(SelectionDialog)

    def binding_update(self, proxy):
        items = list(self.model.values)
        for opt in proxy.binding.options:
            if not any(opt == item.value for item in self.model.values):
                newItem = IconItem(value=opt)
                items.append(newItem)
        self.model.values = items

    def condition(self, value, item):
        """Check if the proxy value is the same as the string of one of the
           options"""
        return value == item.value


@register_binding_controller(ui_name='Icons', binding_type=StringBinding,
                             klassname='TextIcons')
class TextIcons(_BaseIcons):
    model = Instance(TextIconsModel, args=())
    dialog_klass = Type(TextDialog)

    def condition(self, value, item):
        """Check if the proxy value is the same as the string of one of the
           options"""
        return item.re.match(value)
