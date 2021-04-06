#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import ArrayOrNone, Instance, WeakRef

from karabogui.binding.api import (
    get_binding_value, PropertyProxy, VectorNumberBinding)
from karabo.common.scenemodel.api import (
    build_model_config, VectorScatterGraphModel)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.plots.api import KaraboPlotView, ScatterGraphPlot
from karabogui import icons

MIN_POINT_SIZE = 0.1
MAX_POINT_SIZE = 10.0


@register_binding_controller(ui_name='Vector XY Scatter',
                             klassname='VectorScatterGraph',
                             binding_type=VectorNumberBinding,
                             can_show_nothing=False)
class DisplayVectorScatterGraph(BaseBindingController):
    """The scatter graph widget provides a scatter plot from property proxies
    """
    model = Instance(VectorScatterGraphModel, args=())

    # Internal traits
    _x_proxy = Instance(PropertyProxy)
    _y_proxy = Instance(PropertyProxy)
    _last_x_value = ArrayOrNone
    _plot = WeakRef(ScatterGraphPlot)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.enable_export()
        widget.stateChanged.connect(self._change_model)

        self._plot = widget.add_scatter_item(cycle=False)
        # assigning proxy is safe and wanted here!
        self._x_proxy = self.proxy
        self._last_x_value = get_binding_value(self.proxy)
        widget.restore(build_model_config(self.model))
        self._plot.setSize(self.model.psize)

        point_size_action = QAction("Point Size", widget)
        point_size_action.triggered.connect(self._configure_point_size)
        point_size_action.setIcon(icons.scatter)
        widget.addAction(point_size_action)

        return widget

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        if self._y_proxy is None:
            self._y_proxy = proxy
            return True

        return False

    def value_update(self, proxy):
        value = proxy.value
        if proxy is self._x_proxy:
            self._last_x_value = value
        elif proxy is self._y_proxy:
            if value is None:
                return
            if self._last_x_value is not None and self.widget is not None:
                self._plot.setData(self._last_x_value, value)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)

    def _configure_point_size(self):
        psize, ok = QInputDialog.getDouble(self.widget, 'Size of points',
                                           'Pointsize:', self.model.psize,
                                           MIN_POINT_SIZE, MAX_POINT_SIZE)
        if ok:
            self._plot.setSize(psize)
            self.model.psize = psize
