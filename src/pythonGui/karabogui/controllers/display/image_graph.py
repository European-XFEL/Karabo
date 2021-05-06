from qtpy.QtWidgets import QAction
from traits.api import Bool, Instance, WeakRef

from karabo.common.scenemodel.api import (
    ImageGraphModel, build_graph_config, restore_graph_config)
from karabo.native import EncodingType, Timestamp
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import AuxPlots
from karabogui.graph.image.api import (
    KaraboImageNode, KaraboImagePlot, KaraboImageView)


@register_binding_controller(ui_name='Image Graph',
                             klassname='ImageGraph',
                             binding_type=ImageBinding,
                             priority=10,
                             can_show_nothing=False)
class DisplayImageGraph(BaseBindingController):
    # Our Image Graph Model
    model = Instance(ImageGraphModel, args=())
    grayscale = Bool(True)

    _plot = WeakRef(KaraboImagePlot)
    _image_node = Instance(KaraboImageNode, args=())

    _colormap_action = Instance(QAction)

    def create_widget(self, parent):
        widget = KaraboImageView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.toolTipChanged.connect(self.show_timestamp_tooltip)
        widget.add_colorbar()
        widget.add_picker()
        widget.add_roi()
        widget.add_aux(plot=AuxPlots.ProfilePlot, smooth=True)
        widget.add_aux(plot=AuxPlots.Histogram)

        # Finalize
        widget.add_toolbar()

        # Get a reference for our plotting
        self._plot = widget.plot()

        # QActions
        widget.add_axes_labels_dialog()
        widget.add_transforms_dialog()

        # Restore the model information
        widget.restore(build_graph_config(self.model))

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    def show_timestamp_tooltip(self):
        image_node = self.proxy.value
        if image_node is None:
            return
        timestamp = image_node.pixels.value.data.timestamp
        if timestamp is None:
            # Can happen when we toggle aux plots
            return
        diff = Timestamp().toTimestamp() - timestamp.toTimestamp()
        self.widget.setToolTip("{} --- Last image received {:.3f} s "
                               "ago".format(self.proxy.key, diff))

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def value_update(self, proxy):
        image_data = proxy.value
        self._image_node.set_value(image_data)

        if not self._image_node.is_valid:
            return

        array = self._image_node.get_data()

        # Enable/disable some widget features depending on the encoding
        self.grayscale = (self._image_node.encoding == EncodingType.GRAY
                          and array.ndim == 2)

        self._plot.setData(array)

    def _grayscale_changed(self, grayscale):
        if grayscale:
            self.widget.add_colorbar()
            self.widget.restore({"colormap": self.model.colormap})
            self.widget.enable_aux()
        else:
            self.widget.remove_colorbar()
            self.widget.disable_aux()
