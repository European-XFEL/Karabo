#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
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

from traits.api import Instance

from karabo.common.scenemodel.api import (
    DisplayAlarmFloatModel, DisplayAlarmIntegerModel, DisplayFloatModel,
    DisplayLabelModel)
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, IntBinding, StringBinding,
    get_dtype_format)
from karabogui.controllers.api import (
    AlarmMixin, BaseLabelController, FormatMixin, register_binding_controller)

BINDING_TYPES = (StringBinding, CharBinding, ComplexBinding, IntBinding,
                 FloatBinding)


@register_binding_controller(ui_name="Value Field",
                             klassname="DisplayLabel",
                             binding_type=BINDING_TYPES, priority=20)
class DisplayLabel(BaseLabelController):
    model = Instance(DisplayLabelModel, args=())

    def binding_update_proxy(self, proxy):
        self.fmt = get_dtype_format(proxy.binding)


@register_binding_controller(ui_name="Float Field",
                             klassname="DisplayFloat",
                             binding_type=(FloatBinding, ComplexBinding),
                             priority=10)
class DisplayFloat(FormatMixin, BaseLabelController):
    model = Instance(DisplayFloatModel, args=())


@register_binding_controller(ui_name="Alarm Float Field",
                             klassname="DisplayAlarmFloat",
                             binding_type=(FloatBinding, ComplexBinding),
                             priority=0)
class DisplayAlarmFloat(AlarmMixin, FormatMixin, BaseLabelController):
    model = Instance(DisplayAlarmFloatModel, args=())


@register_binding_controller(ui_name="Alarm Integer Field",
                             klassname="DisplayAlarmInteger",
                             binding_type=IntBinding,
                             priority=0)
class DisplayAlarmInteger(AlarmMixin, BaseLabelController):
    model = Instance(DisplayAlarmIntegerModel, args=())
