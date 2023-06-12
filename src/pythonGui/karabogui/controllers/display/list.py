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
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QDialog, QFrame
from traits.api import Instance

from karabo.common.scenemodel.api import DisplayListModel
from karabogui.binding.api import (
    VectorBinding, VectorCharBinding, VectorHashBinding, get_binding_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.dialogs.format_label import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName
from karabogui.widgets.hints import ElidingLabel


def _is_compatible(binding):
    return not isinstance(binding, (VectorCharBinding, VectorHashBinding))


@register_binding_controller(ui_name="List", is_compatible=_is_compatible,
                             klassname="DisplayList", priority=10,
                             binding_type=VectorBinding)
class DisplayList(BaseBindingController):
    model = Instance(DisplayListModel, args=())

    def create_widget(self, parent):
        widget = ElidingLabel(parent)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)
        widget.setFocusPolicy(Qt.NoFocus)
        objectName = generateObjectName(self)
        widget.setObjectName(objectName)
        style_sheet = (f"QWidget#{objectName}" +
                       f" {{ background-color : rgba{ALL_OK_COLOR}; }}")
        widget.setStyleSheet(style_sheet)
        action = QAction("Format field...", widget)
        action.triggered.connect(self._format_field)
        widget.addAction(action)

        return widget

    def value_update(self, proxy):
        value = get_binding_value(proxy, [])
        self.widget.setText(",".join(str(v) for v in value))

    def _format_field(self):
        dialog = FormatLabelDialog(font_size=self.model.font_size,
                                   font_weight=self.model.font_weight,
                                   parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(
                font_size=dialog.font_size, font_weight=dialog.font_weight)
            self._apply_format()

    def _apply_format(self, widget=None):
        if widget is None:
            widget = self.widget

        # Apply font formatting
        font = widget.font()
        font.setPointSize(get_font_size_from_dpi(self.model.font_size))
        font.setBold(self.model.font_weight == "bold")
        widget.setFont(font)
