from itertools import cycle
from weakref import WeakValueDictionary

from qtpy.QtGui import QColor
from qtpy.QtWidgets import QAction, QDialog
from traits.api import Bool, Instance

from karabo.common.scenemodel.api import (
    KARABO_CURVE_OPTIONS, CurveType, build_graph_config,
    extract_graph_curve_option, restore_graph_config)
from karabogui import icons
from karabogui.controllers.arrays import get_array_data
from karabogui.controllers.base import BaseBindingController
from karabogui.graph.common.colors import get_pen_cycler
from karabogui.graph.common.toolbar.widgets import create_button
from karabogui.graph.plots.base import KaraboPlotView
from karabogui.graph.plots.dialogs import CurveOptionsDialog
from karabogui.graph.plots.dialogs.data_analysis import DataAnalysisDialog
from karabogui.graph.plots.dialogs.transform_config import TransformDialog
from karabogui.graph.plots.utils import (
    create_curve_options, generate_baseline, generate_down_sample,
    get_view_range)


class BaseArrayGraph(BaseBindingController):
    _curves = Instance(WeakValueDictionary, args=())
    _pens = Instance(cycle, allow_none=False)

    _has_curve_options = Bool(False)

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

        if KARABO_CURVE_OPTIONS in self.model.copyable_trait_names():
            self._has_curve_options = True
            curve_options = QAction("Curve Options", widget)
            curve_options.setIcon(icons.settings)
            curve_options.triggered.connect(self.show_options_dialog)
            widget.addAction(curve_options)

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
            proxies=self.proxies, config=config, parent=self.widget)
        data_analysis_dialog.show()

    def __pens_default(self):
        return get_pen_cycler()

    # ----------------------------------------------------------------
    # Plot Options

    def show_options_dialog(self):
        config = build_graph_config(self.model)
        options = config.get(KARABO_CURVE_OPTIONS, {})
        curve_options = create_curve_options(
            curves=self._curves, options=options,
            curve_type=CurveType.Curve)

        dialog = CurveOptionsDialog(curve_options, parent=self.widget)
        if dialog.exec() != QDialog.Accepted:
            return

        curve_options = dialog.get_curve_options()
        if curve_options is None:
            self.reset_curve_options()
            return

        if curve_options:
            self.set_curve_options(curve_options)

    def set_curve_options(self, curve_options: dict):
        """Method to set the curve options on the model and plot"""
        self._change_model(
            {KARABO_CURVE_OPTIONS: list(curve_options.values())})
        self.widget.apply_curve_options(curve_options)

    def reset_curve_options(self):
        """Restore the default curve options."""
        self.reset_traits(["_pens"])
        options = {}
        for proxy, curve in self._curves.items():
            pen = next(self._pens)
            options[curve] = {
                "name": proxy.key,
                "curve_type": CurveType.Curve,
                "pen_color": pen.color().name()}

        self.model.curve_options = []
        self.widget.apply_curve_options(options)

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
        if self._has_curve_options:
            options = [option for option in self.model.curve_options
                       if option.key != proxy.key]
            self.model.trait_set(curve_options=options)

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
        pen = next(self._pens)
        if self._has_curve_options:
            options = extract_graph_curve_option(self.model, name)
            if options:
                pen_color = options.get("pen_color")
                pen.setColor(QColor(pen_color))
                name = options.get("name")

        curve = widget.add_curve_item(name=name, pen=pen)
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
