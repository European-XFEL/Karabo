#############################################################################
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

from karabo.common.api import KARABO_SCHEMA_ROW_SCHEMA
from karabo.common.scenemodel.api import (
    FilterTableElementModel, TableElementModel)
from karabogui.binding.api import VectorHashBinding
from karabogui.controllers.api import register_binding_controller
from karabogui.controllers.table.api import (
    BaseFilterTableController, BaseTableController)


def _is_compatible(binding):
    return KARABO_SCHEMA_ROW_SCHEMA in binding.attributes


@register_binding_controller(ui_name="Table Element",
                             klassname="EditableTableElement", can_edit=True,
                             binding_type=VectorHashBinding, priority=100,
                             is_compatible=_is_compatible)
class EditableTableElement(BaseTableController):
    """The editable version of the table element"""
    model = Instance(TableElementModel, args=())


@register_binding_controller(ui_name="Display Table Element",
                             klassname="DisplayTableElement", priority=90,
                             binding_type=VectorHashBinding,
                             is_compatible=_is_compatible)
class DisplayTableElement(BaseTableController):
    """The display version of the table element"""
    model = Instance(TableElementModel, args=())


@register_binding_controller(ui_name="Editable Filter Table Element",
                             klassname="EditableFilterTableElement",
                             can_edit=True,
                             binding_type=VectorHashBinding, priority=80,
                             is_compatible=_is_compatible)
class EditableFilterTableElement(BaseFilterTableController):
    """The editable version of the filter table element"""
    model = Instance(FilterTableElementModel, args=())


@register_binding_controller(ui_name="Display Filter Table Element",
                             klassname="DisplayFilterTableElement",
                             priority=80,
                             binding_type=VectorHashBinding,
                             is_compatible=_is_compatible)
class DisplayFilterTableElement(BaseFilterTableController):
    """The display version of the filter table element"""
    model = Instance(FilterTableElementModel, args=())
