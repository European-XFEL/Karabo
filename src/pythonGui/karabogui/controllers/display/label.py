#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from numbers import Number

from numpy import log10, number
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QFrame, QLabel
from traits.api import Instance, Str, Tuple

from karabo.common.api import (
    KARABO_SCHEMA_ABSOLUTE_ERROR, KARABO_SCHEMA_DISPLAY_TYPE,
    KARABO_SCHEMA_RELATIVE_ERROR
)
from karabo.common.scenemodel.api import DisplayLabelModel
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, get_binding_value, IntBinding,
    StringBinding
)
from karabogui.const import (
    FINE_COLOR, PROPERTY_ALARM_COLOR_MAP, WIDGET_MIN_HEIGHT)
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, register_binding_controller)
from karabogui.util import generateObjectName

BINDING_TYPES = (CharBinding, ComplexBinding, FloatBinding, StringBinding,
                 IntBinding)


@register_binding_controller(ui_name='Value Field',
                             klassname='DisplayLabel',
                             binding_type=BINDING_TYPES, priority=20)
class DisplayLabel(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(DisplayLabelModel, args=())
    # Internal traits
    _bg_color = Tuple(FINE_COLOR)
    _internal_widget = Instance(QLabel, allow_none=True)
    _style_sheet = Str

    def create_widget(self, parent):
        self._internal_widget = QLabel(parent)
        self._internal_widget.setAlignment(Qt.AlignCenter)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.setWordWrap(True)
        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)

        objectName = generateObjectName(self)
        self._style_sheet = ("QWidget#{}".format(objectName) +
                             " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)
        return widget

    def value_update(self, proxy):
        self.widget.update_label(proxy)

        binding = proxy.binding
        value = get_binding_value(proxy, '')
        if value == '' or isinstance(binding, StringBinding):
            # Early bail out for Long binary data (e.g. image) or
            # if the property is not set (Undefined)
            self._internal_widget.setText(value[:255])
            return

        disp_type = binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE)
        try:
            fmt = {
                'bin': 'b{:b}',
                'oct': 'o{:o}',
                'hex': '0x{:X}'
            }[disp_type[:3]]
        except (TypeError, KeyError):
            abserr = binding.attributes.get(KARABO_SCHEMA_ABSOLUTE_ERROR)
            relerr = binding.attributes.get(KARABO_SCHEMA_RELATIVE_ERROR)
            if (relerr is not None and
                    (abserr is None or
                     not isinstance(value, (Number, number)) or
                     relerr * value > abserr)):
                fmt = "{{:.{}g}}".format(-int(log10(relerr)))
            elif abserr is not None:
                if abserr < 1:
                    fmt = "{{:.{}f}}".format(-int(log10(abserr)))
                elif (isinstance(value, (Number, number)) and
                        abs(value) > abserr):
                    fmt = "{{:.{}e}}".format(int(log10(abs(value))) -
                                             int(log10(abserr)))
                else:
                    fmt = "{:.0f}"
            elif isinstance(binding, FloatBinding):
                fmt = "{:.8g}"
            else:
                fmt = "{}"

        ret = fmt.format(value)
        self._internal_widget.setText(ret)

    def update_alarms(self, alarm_type):
        self._bg_color = PROPERTY_ALARM_COLOR_MAP[alarm_type]
        sheet = self._style_sheet.format(self._bg_color)
        self.widget.setStyleSheet(sheet)
