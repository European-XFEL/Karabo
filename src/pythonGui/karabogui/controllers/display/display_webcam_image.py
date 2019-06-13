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
                             binding_type=ImageBinding, priority=0,
                             can_show_nothing=False)
class DisplayWebCamGraph(BaseBindingController):
    model = Instance(WebCamGraphModel)

    _plot = Instance(KaraboImagePlot)

    def create_widget(self, parent):
        widget = KaraboImageView()
        widget.stateChanged.connect(self._change_model)

        self._plot = widget.plot()

        # Disable Context Menu!
        self._plot.set_context_menu(None)

        # Disable mouse and axis!
        self._plot.setMouseEnabled(False, False)
        self._plot.hide_all_axis()

        # Colormap
        widget.add_colormap_action(self.model.colormap)
        widget.set_colormap(self.model.colormap, update=False)

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**content)

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        image_node = KaraboImageNode(proxy.value)

        if not image_node.is_valid:
            return

        if image_node.dim_z:
            array = image_node.get_slice(DIMENSIONS['Z'], 0)
        else:
            array = image_node.get_data()

        self._plot.setData(array)
