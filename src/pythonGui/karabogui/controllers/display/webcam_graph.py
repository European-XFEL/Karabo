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
from qtpy.QtWidgets import QAction, QMenu
from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import WebCamGraphModel
from karabogui import icons
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.graph.image.api import (
    KaraboImageNode, KaraboImagePlot, KaraboImageView, karabo_default_image)


@register_binding_controller(ui_name='WebCam Graph',
                             klassname='WebCamGraph',
                             binding_type=ImageBinding, priority=90,
                             can_show_nothing=False)
class DisplayWebCamGraph(BaseBindingController):
    model = Instance(WebCamGraphModel, args=())

    _plot = WeakRef(KaraboImagePlot)
    _image_node = Instance(KaraboImageNode, args=())

    def create_widget(self, parent):
        widget = KaraboImageView(parent=parent)
        widget.stateChanged.connect(self._change_model)

        self._plot = widget.plot()

        # Disable ViewBox Menu!
        self._plot.set_viewbox_menu(None)

        # Disable mouse and axis!
        self._plot.setMouseEnabled(False, False)
        self._plot.hide_all_axis()

        # Undock - Transient model data
        if not self.model.undock:
            menu = QMenu(widget)
            undock_action = QAction(icons.undock, "Undock", widget)
            undock_action.triggered.connect(self._undock_graph)
            menu.addAction(undock_action)
            self._plot.set_viewbox_menu(menu)

        # Colormap
        widget.add_colormap_action()
        widget.restore({"colormap": self.model.colormap})

        return widget

    def clear_widget(self):
        self._plot.setData(karabo_default_image())

    # -----------------------------------------------------------------------
    # Qt Slots

    def _undock_graph(self):
        """Undock the graph image but don't offer roi option"""
        model = self.model.clone_traits()
        model.undock = True
        broadcast_event(KaraboEvent.ShowUnattachedController,
                        {"model": model})

    def _change_model(self, content):
        self.model.trait_set(**content)

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        self._image_node.set_value(proxy.value)

        if not self._image_node.is_valid:
            return

        self._plot.setData(self._image_node.get_data())
