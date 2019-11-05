#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtWidgets import QAction, QInputDialog

from karabo.common.scenemodel.api import (
    restore_graph_config, build_graph_config, VectorRollGraphModel)
from karabo.native import Timestamp
from karabogui.graph.common.api import AuxPlots
from karabogui.graph.image.api import (
    KaraboImagePlot, KaraboImageView, RollImage)
from karabogui.binding.api import get_binding_value, VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from traits.api import Instance


MAX_NUM_VECTORS = 500


@register_binding_controller(ui_name='VectorRoll Graph',
                             klassname='VectorRollGraph',
                             binding_type=VectorNumberBinding,
                             priority=0,
                             can_show_nothing=False)
class DisplayVectorRollGraph(BaseBindingController):
    # Our VectorRollGraph Model
    model = Instance(VectorRollGraphModel, args=())

    _plot = Instance(KaraboImagePlot)
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
        widget.add_toolbar()
        widget.plotItem.set_aspect_ratio(0)
        widget.plotItem.vb.disableAutoRange()

        # Get a reference for our plotting
        self._plot = widget.plot()
        self._plot.enable_downsampling(False)

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

    # @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    # @pyqtSlot()
    def _configure_maxlen(self, checked):
        maxlen, ok = QInputDialog.getInt(self.widget, 'Vector Stack',
                                         'Maxlen:', self.model.maxlen, 5,
                                         MAX_NUM_VECTORS)
        if ok:
            self._image.stack = maxlen
            self.model.maxlen = maxlen

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        value = get_binding_value(proxy.binding)
        if value is None:
            return

        timestamp = proxy.binding.timestamp
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
