# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pathlib import Path

import numpy as np
from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog
from scipy.optimize import curve_fit

from karabogui.binding.api import get_binding_value
from karabogui.graph.common.api import make_pen
from karabogui.graph.common.fitting import (
    gaussian_fit, linear_function_fit, normal_cdf)
from karabogui.graph.common.formatting import (
    table_body, table_header, table_row)
from karabogui.graph.common.utils import float_to_string
from karabogui.graph.plots.base import KaraboPlotView
from karabogui.graph.plots.utils import generate_baseline

header = """<b><u>{FUNC} Fit Result</b></u>"""
row = table_row(table_body(header="{param}", tabs=("{value}", "{error}")))

FWHM_COEFF = 2 * np.sqrt(2 * np.log(2))

FIT_FUNCTION_MAP = {
    "Gaussian": {
        "func": gaussian_fit, "param_names": ["Amplitude", "Max Pos", "fwhm"]},
    "Step function (CDF)": {
        "func": normal_cdf, "param_names": ["Sigma"]},
    "Linear function": {
        "func": linear_function_fit, "param_names": ["Slope", "Interception"]},
    }


class DataAnalysisDialog(QDialog):
    """
    A dialog to perform curve fitting based on different fit functions.
        - Gaussian function
        - Step function
        - Linear function.
    """

    def __init__(self, proxies, config=None, parent=None):
        super().__init__(parent)
        ui_path = Path(__file__).parent / "data_analysis.ui"
        uic.loadUi(ui_path, self)
        self.config = config
        self.x_values = self.y_values = []

        self._plot_widget = KaraboPlotView()
        self._plot_widget.restore(config)
        self._plot_widget.add_legend()
        self._plot_widget.enable_data_toggle()
        self._plot_widget.add_toolbar()
        self._plot_widget.enable_export()
        self.plot_layout.addWidget(self._plot_widget)
        pen = make_pen(color="r")
        self.fit_curve = self._plot_widget.add_curve_item(
            name="fitting", pen=pen)
        self.data_curve = self._plot_widget.add_curve_item(
            name="data")

        self.fit_options_combobox.addItems(list(FIT_FUNCTION_MAP.keys()))
        self.auto_update_checkbox.setChecked(False)

        self.fit_button.clicked.connect(self.fit)
        self.update_data_button.clicked.connect(self.update_data)

        self.set_up_property_combobox(proxies)
        self.proxy = proxies[0]
        self.setWindowTitle(f"Data Analysis Dialog|{self.proxy.key}")
        self.update_data()

    def set_up_property_combobox(self, proxies: list):
        """
        Add all the available proxies to the combobox as internal
        data and its key as visible text. Also select the item
        corresponding to the proxy which is plotted.

        :param proxies: All available proxies.
        """
        visible = len(proxies) > 1
        self.property_comboBox.setVisible(visible)
        self.property_label.setVisible(visible)

        if not visible:
            return

        for index, proxy in enumerate(proxies):
            self.property_comboBox.addItem(proxy.key)
            self.property_comboBox.setItemData(index, proxy)
        self.property_comboBox.currentIndexChanged.connect(
            self._on_property_changed)

    @Slot(int)
    def _on_property_changed(self):
        """
        Update the plot and dialog's window title with respect to the selected
        proxy.
        """
        proxy = self.property_comboBox.currentData()
        self.proxy = proxy
        self.update_data()
        self.setWindowTitle(f"Data Analysis Dialog|{proxy.key}")

    def update_data(self):
        """Fetch the latest data points from the parent plot"""
        value = get_binding_value(self.proxy)
        if value is None:
            return
        self.y_values = value
        offset = self.config.get("offset", 0.0)
        step = self.config.get("step", 1.0)
        self.x_values = generate_baseline(self.y_values, offset=offset,
                                          step=step)
        self.data_curve.setData(self.x_values, self.y_values)
        if self.auto_update_checkbox.isChecked():
            self.fit()
        else:
            self.fit_curve.setVisible(False)

    @Slot()
    def fit(self):
        """Fit data with the selected function"""
        if self.x_values is None or self.y_values is None:
            return
        xfit = self.x_values
        fit_option = self.fit_options_combobox.currentText()
        fit_func = FIT_FUNCTION_MAP[fit_option].get("func")
        try:
            params, pcov = curve_fit(fit_func, self.x_values, self.y_values)
        except (RuntimeError, TypeError, ValueError):
            xfit = yfit = np.array([])
            params = perr = np.array([])
        else:
            yfit = fit_func(self.x_values, *params)
            perr = np.sqrt(np.diag(pcov))

        self.fit_curve.setData(xfit, yfit, width=10)

        self._update_fit_result(heading=fit_option, values=params, perr=perr)
        self.fit_curve.setVisible(True)

    def _update_fit_result(self, heading, values, perr):
        """
        Display the best fit params on the  gui.
        """
        text = "<table style='font-size: 11px'>" + header.format(
            FUNC=heading)
        text = text + table_row(table_header(tabs=("Value", "Error")))
        param_names = FIT_FUNCTION_MAP[heading].get("param_names")
        if not values.size:
            values = [np.nan] * len(param_names)
        if not perr.size:
            perr = [np.nan] * len(param_names)

        for param, value, error in zip(param_names, values, perr):
            if param == "fwhm":
                value = FWHM_COEFF * value
            value = float_to_string(value, precision=3)
            error = float_to_string(error, precision=3)
            text = text + row.format(param=param, value=value, error=error)
        text = text + "</table>"
        self.result_label.setText(text)

    def set_axis_labels(self, x_label=None, y_label=None):
        """
        Set the Labels on the X and Y axis items.
        """
        if x_label is not None:
            self._plot_widget.plotItem.setLabel("bottom", x_label)
        if y_label is not None:
            self._plot_widget.plotItem.setLabel("left", y_label)
