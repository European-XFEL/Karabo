from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction
from traits.api import Instance, Int

from karabo.common.scenemodel.api import (
    build_graph_config, restore_graph_config, ImageGraphModel)
from karabo.native import Timestamp
from karabogui.graph.common.api import AuxPlots
from karabogui.graph.image.api import (
    KaraboImagePlot, KaraboImageNode, KaraboImageView)
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, DIMENSIONS, register_binding_controller)


@register_binding_controller(ui_name='Image Graph',
                             klassname='ImageGraph',
                             binding_type=ImageBinding,
                             priority=10,
                             can_show_nothing=False)
class DisplayImageGraph(BaseBindingController):
    # Our Image Graph Model
    model = Instance(ImageGraphModel, args=())

    _plot = Instance(KaraboImagePlot)
    _image_node = Instance(KaraboImageNode)

    _axis = Int(2)
    _colormap_action = Instance(QAction)

    def create_widget(self, parent):
        widget = KaraboImageView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.toolTipChanged.connect(self.show_timestamp_tooltip)
        widget.add_colorbar()
        widget.add_picker()
        widget.add_roi()
        widget.add_aux(AuxPlots.ProfilePlot, config={'smooth': True})

        # Finalize
        widget.add_toolbar()

        # Get a reference for our plotting
        self._plot = widget.plot()
        self._plot.enable_downsampling(True)

        # QActions
        widget.add_axes_labels_dialog()
        widget.add_transforms_dialog()

        # Restore the model information
        widget.restore(build_graph_config(self.model))

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot()
    def show_timestamp_tooltip(self):
        image_node = self.proxy.value
        if image_node is None:
            return
        timestamp = image_node.pixels.value.data.timestamp
        diff = Timestamp().toTimestamp() - timestamp.toTimestamp()
        self.widget.setToolTip("{} --- Last image received {:.3f} s "
                               "ago".format(self.proxy.key, diff))

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def value_update(self, proxy):
        image_data = proxy.value
        image_node = KaraboImageNode(image_data)

        if not image_node.is_valid:
            return

        if "stackAxis" in image_data:
            self._axis = image_data.stackAxis.value
        else:
            # NOTE: this might happen for RGB image
            self._axis = DIMENSIONS['Z']

        array = image_node.get_slice(self._axis, 0)

        self._plot.setData(array)
