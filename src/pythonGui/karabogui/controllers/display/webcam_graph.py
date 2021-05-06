from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import WebCamGraphModel
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.image.api import (
    KaraboImageNode, KaraboImagePlot, KaraboImageView)


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
        self._plot.enable_viewbox_menu(False)

        # Disable mouse and axis!
        self._plot.setMouseEnabled(False, False)
        self._plot.hide_all_axis()

        # Colormap
        widget.add_colormap_action()
        widget.restore({"colormap": self.model.colormap})

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        self._image_node.set_value(proxy.value)

        if not self._image_node.is_valid:
            return

        self._plot.setData(self._image_node.get_data())
