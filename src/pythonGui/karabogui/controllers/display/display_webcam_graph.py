from PyQt4.QtCore import pyqtSlot
from traits.api import Instance

from karabo.common.scenemodel.api import WebCamGraphModel

from karabogui.graph.image.api import (
    KaraboImagePlot, KaraboImageNode, KaraboImageView)
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.images import DIMENSIONS


@register_binding_controller(ui_name='WebCam Graph',
                             klassname='WebCamGraph',
                             binding_type=ImageBinding, priority=90,
                             can_show_nothing=False)
class DisplayWebCamGraph(BaseBindingController):
    model = Instance(WebCamGraphModel, args=())

    _plot = Instance(KaraboImagePlot)
    _image_node = Instance(KaraboImageNode, args=())

    def create_widget(self, parent):
        widget = KaraboImageView()
        widget.stateChanged.connect(self._change_model)

        self._plot = widget.plot()
        self._plot.imageItem.enable_downsampling(True)

        # Disable Context Menu!
        self._plot.set_context_menu(None)

        # Disable mouse and axis!
        self._plot.setMouseEnabled(False, False)
        self._plot.hide_all_axis()

        # Colormap
        widget.add_colormap_action()
        widget.restore({"colormap": self.model.colormap})

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**content)

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        self._image_node.set_value(proxy.value)

        if not self._image_node.is_valid:
            return

        if self._image_node.dim_z:
            array = self._image_node.get_slice(DIMENSIONS['Z'], 0)
        else:
            array = self._image_node.get_data()

        self._plot.setData(array)
