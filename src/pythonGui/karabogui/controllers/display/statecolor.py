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
from traits.api import Instance, Str, on_trait_change

from karabo.common.scenemodel.api import DisplayStateColorModel
from karabogui.binding.api import StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.dialogs.format_label import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.indicators import get_state_color
from karabogui.util import generateObjectName
from karabogui.widgets.hints import FrameWidget


@register_binding_controller(ui_name='State Color Field',
                             klassname='DisplayStateColor',
                             binding_type=StringBinding,
                             priority=90,
                             is_compatible=with_display_type('State'),
                             can_show_nothing=False)
class DisplayStateColor(BaseBindingController):
    # The specific scene model class used by this widget
    model = Instance(DisplayStateColorModel, args=())
    # CSS template for the widget BG color
    _style_sheet = Str

    def create_widget(self, parent):
        widget = FrameWidget(parent)
        objectName = generateObjectName(self)
        self._style_sheet = (f"QLabel#{objectName}" +
                             " {{ background-color : rgb{}; }}")
        widget.setObjectName(objectName)

        textAction = QAction("Show State String", widget)
        textAction.triggered.connect(self._show_state_string)
        # update the context menu and keep track
        textAction.setCheckable(True)
        textAction.setChecked(self.model.show_string)
        widget.addAction(textAction)

        # Add an action for formatting options
        format_action = QAction("Format field..", widget)
        format_action.triggered.connect(self._format_field)
        widget.addAction(format_action)
        self._apply_format(widget)

        return widget

    def value_update(self, proxy):
        value = proxy.value
        color = get_state_color(value)
        sheet = self._style_sheet.format(color)
        self.widget.setStyleSheet(sheet)

        if self.model.show_string:
            self.widget.setText(proxy.value)

    @on_trait_change('model.show_string', post_init=True)
    def _update_text(self):
        if self.proxy is not None:
            self.value_update(self.proxy)
            if not self.model.show_string:
                # Only clear the widget once if no action is set!
                self.widget.clear()

    def _show_state_string(self):
        self.model.show_string = not self.model.show_string

    # -----------------------------------------------------------------------
    # Formatting methods

    def _format_field(self):
        dialog = FormatLabelDialog(font_size=self.model.font_size,
                                   font_weight=self.model.font_weight,
                                   parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(font_size=dialog.font_size,
                                 font_weight=dialog.font_weight)
            self._apply_format()

    def _apply_format(self, widget=None):
        """The widget is passed as an argument in create_widget as it is not
           yet bound to self.widget then"""
        if widget is None:
            widget = self.widget

        # Apply font formatting
        font = widget.font()
        font.setPointSize(get_font_size_from_dpi(self.model.font_size))
        font.setBold(self.model.font_weight == "bold")
        widget.setFont(font)
