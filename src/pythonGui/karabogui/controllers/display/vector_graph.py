#############################################################################
# Author: <dennis.goeries@xfel.eu>
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

from karabo.common.scenemodel.api import VectorGraphModel
from karabo.common.scenemodel.widgets.graph_plots import NDArrayGraphModel
from karabogui.binding.api import VectorNumberBinding
from karabogui.binding.binding_types import NDArrayBinding
from karabogui.controllers.api import register_binding_controller
from karabogui.controllers.basearray import BaseArrayGraph


@register_binding_controller(ui_name='Vector Graph', klassname='VectorGraph',
                             binding_type=VectorNumberBinding,
                             priority=0,
                             can_show_nothing=False)
class DisplayVectorGraph(BaseArrayGraph):
    """The VectorGraph controller for display basic vector data
    """
    model = Instance(VectorGraphModel, args=())


@register_binding_controller(ui_name='NDArray Graph',
                             klassname='NDArrayGraph',
                             binding_type=NDArrayBinding,
                             priority=90,
                             can_show_nothing=False)
class DisplayNDArrayGraph(BaseArrayGraph):
    """The NDArray controller for display of pulse data"""
    model = Instance(NDArrayGraphModel, args=())
