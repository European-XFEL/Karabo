#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from itertools import cycle
from weakref import WeakValueDictionary

from qtpy.QtWidgets import QAction
from traits.api import Instance

from karabo.common.scenemodel.api import (
    VectorGraphModel, build_graph_config, restore_graph_config)
from karabo.common.scenemodel.widgets.graph_plots import NDArrayGraphModel
from karabogui import icons
from karabogui.binding.api import VectorNumberBinding
from karabogui.binding.binding_types import NDArrayBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.arrays import get_array_data
from karabogui.graph.common.api import create_button, get_pen_cycler
from karabogui.graph.plots.api import (
    KaraboPlotView, TransformDialog, generate_baseline, generate_down_sample,
    get_view_range)
from karabogui.graph.plots.dialogs.data_analysis import DataAnalysisDialog


class BaseArrayGraph(BaseBindingController):
    _curves = Instance(WeakValueDictionary, args=())
    _pens = Instance(cycle, allow_none=False)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_legend(visible=False)
        widget.add_cross_target()
        widget.add_roi()
        toolbar = widget.add_toolbar()
        config = build_graph_config(self.model)
        widget.restore(config)
        widget.enable_data_toggle()
        widget.enable_export()

        trans_action = QAction("X-Transformation", widget)
        trans_action.triggered.connect(self.configure_transformation)

        widget.addAction(trans_action)

        # Add first curve for the main proxy
        self._add_curve(self.proxy, widget=widget)

        button = create_button(
            icon=icons.data_analysis, checkable=False, tooltip="Data Analysis",
            on_clicked=self.show_data_analysis_dialog)
        toolbar.add_button(button)
        return widget

    def show_data_analysis_dialog(self):
        config = build_graph_config(self.model)
        data_analysis_dialog = DataAnalysisDialog(
            proxy=self.proxy, config=config, parent=self.widget)
        data_analysis_dialog.show()

    def __pens_default(self):
        return get_pen_cycler()

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        """Add a proxy curve to the plot widget

        We only rely here on the `key` information of the binding, which is
        always available. This is required for the plot item `name`.
        We are protected by the base binding controller that a proxy is not
        added multiple times.
        """
        self._add_curve(proxy)
        if len(self._curves) > 1:
            self.widget.set_legend(True)
        return True

    def remove_proxy(self, proxy):
        """Remove a `proxy` from the controller"""
        item = self._curves.pop(proxy)
        self.widget.remove_item(item)
        item.deleteLater()
        legend_visible = len(self._curves) > 1
        self.widget.set_legend(legend_visible)
        return True

    def _add_curve(self, proxy, widget=None):
        """The widget is passed as an argument in create_widget as it is not
           yet bound to self.widget then"""
        if widget is None:
            widget = self.widget

        name = proxy.key
        curve = widget.add_curve_item(name=name, pen=next(self._pens))
        self._curves[proxy] = curve

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def configure_transformation(self):
        content, ok = TransformDialog.get(build_graph_config(self.model),
                                          parent=self.widget)
        if ok:
            self.model.trait_set(**content)
            for proxy in self.proxies:
                self.value_update(proxy)

    def value_update(self, proxy):
        if proxy not in self._curves:
            return

        plot = self._curves[proxy]
        # NOTE: With empty data we will clear the plot!
        y, _ = get_array_data(proxy, default=[])
        if not len(y):
            plot.setData([], [])
            return

        # NOTE: WE cast boolean as int, as numpy method is deprecated
        if y.dtype == bool:
            y = y.astype(int)

        model = self.model
        # Generate the baseline for the x-axis
        x = generate_baseline(y, offset=model.offset, step=model.step)

        rect = get_view_range(plot)
        x, y = generate_down_sample(y, x=x, rect=rect, deviation=True)
        plot.setData(x, y)


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
