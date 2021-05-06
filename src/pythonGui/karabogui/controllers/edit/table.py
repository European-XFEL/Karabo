#############################################################################
# Author: <steffen.hauf@xfel.eu> & <dennis.goeries@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance

from karabo.common.api import KARABO_SCHEMA_ROW_SCHEMA
from karabo.common.scenemodel.api import TableElementModel
from karabogui.binding.api import VectorHashBinding
from karabogui.controllers.api import register_binding_controller
from karabogui.controllers.table.api import BaseTableController


def _is_compatible(binding):
    return KARABO_SCHEMA_ROW_SCHEMA in binding.attributes


@register_binding_controller(ui_name='Table Element',
                             klassname='EditableTableElement', can_edit=True,
                             binding_type=VectorHashBinding, priority=100,
                             is_compatible=_is_compatible)
class EditableTableElement(BaseTableController):
    """The editable version of the table element"""
    model = Instance(TableElementModel, args=())


@register_binding_controller(ui_name='Display Table Element',
                             klassname='DisplayTableElement', priority=90,
                             binding_type=VectorHashBinding,
                             is_compatible=_is_compatible)
class DisplayTableElement(BaseTableController):
    """The display version of the table element"""
    model = Instance(TableElementModel, args=())
