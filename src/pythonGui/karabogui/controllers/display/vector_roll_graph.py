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
from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import (
    VectorRollGraphModel, build_graph_config, restore_graph_config)
from karabo.native import Timestamp
from karabogui import icons
from karabogui.binding.api import NDArrayBinding, VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, get_array_data, register_binding_controller)
from karabogui.graph.common.api import AuxPlots, create_button
from karabogui.graph.image.api import (
    KaraboImagePlot, KaraboImageView, RollImage)

MAX_NUM_VECTORS = 500


@register_binding_controller(
    ui_name='VectorRoll Graph',
    klassname='VectorRollGraph',
    binding_type=(NDArrayBinding, VectorNumberBinding),
    priority=0,
    can_show_nothing=False)
class ArrayRollGraph(BaseBindingController):
    # Our VectorRollGraph Model
    model = Instance(VectorRollGraphModel, args=())

    _plot = WeakRef(KaraboImagePlot)
    _image = Instance(RollImage)
    _timestamp = Instance(Timestamp, args=())

    def create_widget(self, parent):
        widget = KaraboImageView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_colorbar()
        widget.add_picker()
        widget.add_roi()
        widget.add_aux(AuxPlots.ProfilePlot)

        # Finalize
        toolbar = widget.add_toolbar()

        _btn_reset = create_button(
            checkable=False,
            icon=icons.reset,
            tooltip="Reset the image",
            on_clicked=self._reset_image)
        toolbar.add_button(_btn_reset)

        widget.plotItem.set_aspect_ratio(0)
        widget.plotItem.vb.disableAutoRange()

        # Get a reference for our plotting
        self._plot = widget.plot()

        # Set the rolling image with stack!
        self._image = RollImage()
        self._image.stack = self.model.maxlen

        # QActions
        widget.add_axes_labels_dialog()

        size_action = QAction('Image Size', widget)
        size_action.triggered.connect(self._configure_maxlen)
        widget.addAction(size_action)

        # Restore the model information
        widget.restore(build_graph_config(self.model))

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    def _reset_image(self):
        self._image.reset()
        self._plot.vb.setRange(yRange=(0, self._image.stack))

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def _configure_maxlen(self):
        maxlen, ok = QInputDialog.getInt(self.widget, 'Vector Stack',
                                         'Maxlen:', self.model.maxlen, 5,
                                         MAX_NUM_VECTORS)
        if ok:
            self._image.stack = maxlen
            self.model.maxlen = maxlen
            self._plot.vb.setRange(yRange=(0, maxlen))

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        value, timestamp = get_array_data(proxy)
        if value is None:
            return

        if timestamp != self._timestamp:
            # Get new timestamp reference!
            self._timestamp = timestamp

            self._image.add(value)
            if self._image.data is None:
                return

            if not self._plot.image_set:
                self._plot.vb.setRange(xRange=(0, len(value)),
                                       yRange=(0, self._image.stack))

            self._plot.setData(self._image.data, update=False)
