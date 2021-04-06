#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 11, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from datetime import datetime
import traceback

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QFrame, QLabel, QInputDialog
from traits.api import Instance, Undefined

from karabo.common.scenemodel.api import DisplayTimeModel
from karabogui import messagebox
from karabogui.binding.api import (
    BaseBinding, ChoiceOfNodesBinding, get_binding_value, NodeBinding)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName

FORBIDDEN_BINDING = (ChoiceOfNodesBinding, NodeBinding)


def is_compatible(binding):
    return not isinstance(binding, FORBIDDEN_BINDING)


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
        style_sheet = ("QWidget#{}".format(objectName) +
                       " {{ background-color : rgba{}; }}")
        sheet = style_sheet.format(ALL_OK_COLOR)
        widget.setObjectName(objectName)
        widget.setStyleSheet(sheet)

        action = QAction('Change datetime format...', widget)
        widget.addAction(action)
        action.triggered.connect(self._change_time_format)

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
