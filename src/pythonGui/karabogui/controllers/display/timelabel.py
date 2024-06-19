#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 11, 2019
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
#############################################################################
import traceback
from datetime import datetime

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QDialog, QFrame, QInputDialog, QLabel
from traits.api import Instance, Undefined

from karabo.common.scenemodel.api import DisplayTimeModel
from karabogui import messagebox
from karabogui.binding.api import BaseBinding, NodeBinding, get_binding_value
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.dialogs.format_label import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName


def is_compatible(binding):
    return not isinstance(binding, NodeBinding)


@register_binding_controller(ui_name='Time Field',
                             klassname='TimeLabel',
                             binding_type=BaseBinding, priority=-10,
                             is_compatible=is_compatible,
                             can_show_nothing=False)
class DisplayTimeLabel(BaseBindingController):
    model = Instance(DisplayTimeModel, args=())

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setAlignment(Qt.AlignCenter)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setWordWrap(True)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)
        objectName = generateObjectName(self)
        widget.setObjectName(objectName)
        style_sheet = (f"QWidget#{objectName}" +
                       f" {{ background-color : rgba{ALL_OK_COLOR}; }}")
        widget.setStyleSheet(style_sheet)

        action = QAction('Change datetime format...', widget)
        widget.addAction(action)
        action.triggered.connect(self._change_time_format)

        action = QAction("Format field...", widget)
        action.triggered.connect(self._format_field)
        widget.addAction(action)

        self._apply_format(widget)
        return widget

    def value_update(self, proxy):
        if proxy.value is Undefined:
            return
        timestamp = proxy.binding.timestamp
        # Be backward compatible, some elements might not have a timestamp
        # in the past
        if timestamp is not None:
            dt = datetime.fromtimestamp(timestamp.toTimestamp())
            stamp = dt.strftime(self.model.time_format)
        else:
            stamp = "NaN"
        self.widget.setText(stamp)

    def _change_time_format(self):
        # NOTE: No extra protection required, as we do not allow altering
        # models for offline devices without binding
        text, ok = QInputDialog.getText(
            self.widget, 'Enter datetime format', 'datetime format = ',
            text=self.model.time_format)

        if not ok:
            return

        try:
            # Plainly try to apply the time_format on the value
            timestamp = self.proxy.binding.timestamp
            dt = datetime.fromtimestamp(timestamp.toTimestamp())
            dt.strftime(text)
        except Exception as e:
            err = traceback.format_exception_only(type(e), e)
            msg = '<pre>{1}{2}</pre>{3}'.format(*err)
            messagebox.show_error(msg, title='Error in datetime format',
                                  parent=self.widget)
            return
        self.model.time_format = text
        if get_binding_value(self.proxy) is not None:
            self.value_update(self.proxy)

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
        if widget is None:
            widget = self.widget

        # Apply font formatting
        font = widget.font()
        font.setPointSize(get_font_size_from_dpi(self.model.font_size))
        font.setBold(self.model.font_weight == "bold")
        widget.setFont(font)
