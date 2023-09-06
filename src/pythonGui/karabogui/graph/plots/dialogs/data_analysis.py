# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from pathlib import Path

import numpy as np
from pyqtgraph import InfiniteLine
from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog
from scipy.optimize import curve_fit

from karabogui import icons
from karabogui.controllers.api import get_array_data
from karabogui.graph.common.api import create_button, make_pen
from karabogui.graph.common.fitting import (
    gaussian_fit, guess_initial_parameters, linear_function_fit, normal_cdf,
    sqsech)
from karabogui.graph.common.formatting import (
    table_body, table_header, table_row)
from karabogui.graph.common.utils import float_to_string
from karabogui.graph.plots.base import KaraboPlotView
from karabogui.graph.plots.utils import generate_baseline

header = """<b><u>{FUNC} Fit Result</b></u>"""
row = table_row(table_body(header="{param}", tabs=("{value}", "{error}")))

FWHM_COEFF = 2 * np.sqrt(2 * np.log(2))
BELL_CURVES = ("Gaussian", "Sech Square")

FIT_FUNCTION_MAP = {
    "Gaussian": {
        "func": gaussian_fit, "param_names": ["Amplitude", "Max Pos", "fwhm"]},
    "Sech Square": {
        "func": sqsech, "param_names": ["Amplitude", "Max Pos", "Center",
                                        "Sigma"]},
    "Step function (CDF)": {
        "func": normal_cdf, "param_names": ["Sigma"]},
    "Linear function": {
        "func": linear_function_fit, "param_names": ["Slope", "Interception"]},
    }


LINE_EDIT_STYLE = """
QLineEdit {color:gray;}
"""


class DataAnalysisDialog(QDialog):
    """
    A dialog to perform curve fitting based on different fit functions.
        - Gaussian function
        - Step function
        - Linear function.
    """

    def __init__(self, proxies, config=None, baseline_proxy=None, parent=None):
        """
        Define the optional baseline_proxy to set the values on X-axis,
        otherwise they are generated based on Y-axis values.

        :param proxies: Proxies to be plotted on Y-axis
        :type proxies: List[PropertyProxy]
        :param config: Graph Configuration
        :type config: Dict
        :param baseline_proxy: Optional Values to be set on X-axis
        :type baseline_proxy: None or PropertyProxy
        :param parent: Parent Widget
        :type parent: QObject
        """
        super().__init__(parent)
        ui_path = Path(__file__).parent / "data_analysis.ui"
        uic.loadUi(ui_path, self)
        self.config = config
        self.x_values = self.y_values = []
        self.baseline_proxy = baseline_proxy

        self._plot_widget = KaraboPlotView()
        self._plot_widget.restore(config)
        self._plot_widget.add_legend()
        self._plot_widget.enable_data_toggle()
        toolbar = self._plot_widget.add_toolbar()
        self._plot_widget.enable_export()
        self.plot_layout.addWidget(self._plot_widget)
        pen = make_pen(color="r")
        self.fit_curve = self._plot_widget.add_curve_item(
            name="fitting", pen=pen)
        self.data_curve = self._plot_widget.add_curve_item(
            name="data")

        self.fit_options_combobox.addItems(list(FIT_FUNCTION_MAP.keys()))
        self.fit_on_update_checkbox.setChecked(False)

        self.auto_update.setChecked(False)
        self.auto_update.toggled.connect(self._enable_auto_update)
        self.fit_button.clicked.connect(self.fit)
        self.update_data_button.clicked.connect(self.update_data)

        self.set_up_property_combobox(proxies)
        self.proxy = proxies[0] if proxies else None
        title = self.proxy.key if proxies else self.baseline_proxy.key
        self.setWindowTitle(f"Data Analysis Dialog|{title}")
        self.update_data()

        self.setup_roi()
        self.set_line_pos()

        self.show_line_roi_button = create_button(
            icon=icons.split, checkable=True,
            tooltip="Fit Data from Selection",
            on_clicked=self._show_vertical_line_roi)
        toolbar.add_button(self.show_line_roi_button)

        self.stackedWidget.setCurrentIndex(0)

        self.left_pos_line_edit.setStyleSheet(LINE_EDIT_STYLE)
        self.right_pos_line_edit.setStyleSheet(LINE_EDIT_STYLE)
        self.on_left_line_moved()
        self.on_right_line_moved()

    def setup_roi(self):
        """Create two vertical lines to define the selection. By default,
        both are hidden"""
        pen = make_pen('o', alpha=255, width=2)
        self.left_line = InfiniteLine(
            angle=90, pen=pen, movable=True, hoverPen=pen)
        self.right_line = InfiniteLine(
            angle=90, pen=pen, movable=True, hoverPen=pen)
        self._plot_widget.plotItem.addItem(
            self.left_line, ignoreBounds=True)
        self._plot_widget.plotItem.addItem(
            self.right_line, ignoreBounds=True)
        self.left_line.sigPositionChangeFinished.connect(
            self.on_left_line_moved)
        self.right_line.sigPositionChangeFinished.connect(
            self.on_right_line_moved)

        self.left_line.setVisible(False)
        self.right_line.setVisible(False)

    def set_line_pos(self):
        """
        Position the lines at first and last points on X-axis. Set their
        bounds by keeping one unit distance between them so that they never
        come at the same position.
        """
        if not len(self.x_values) or not len(self.y_values):
            return
        left = min(self.x_values)
        right = max(self.x_values)
        min_width = self._min_width
        self.left_line.setBounds((left, right - min_width))
        self.right_line.setBounds((left + min_width, right))
        self.left_line.setPos(left)
        self.right_line.setPos(right)
        self.left_pos_line_edit.setText(float_to_string(left))
        self.right_pos_line_edit.setText(float_to_string(right))

    @Slot(bool)
    def _show_vertical_line_roi(self, toggled):
        """ Set visibility of the lines."""
        self.left_line.setVisible(toggled)
        self.right_line.setVisible(toggled)
        self.stackedWidget.setCurrentIndex(int(toggled))

    @Slot()
    def on_left_line_moved(self):
        if not len(self.x_values):
            return
        left_pos = self.left_line.pos().x()
        self.left_pos_line_edit.setText(float_to_string(left_pos))
        bounds = (left_pos + self._min_width, max(self.x_values))
        self.right_line.setBounds(bounds)

    @Slot()
    def on_right_line_moved(self):
        if not len(self.x_values):
            return
        right_pos = self.right_line.pos().x()
        self.right_pos_line_edit.setText(float_to_string(right_pos))
        bounds = (min(self.x_values), right_pos - self._min_width)
        self.left_line.setBounds(bounds)

    @property
    def _min_width(self):
        return (max(self.x_values) - min(self.x_values) + 1) / len(
            self.x_values)

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
        self.set_line_pos()

    def update_data(self):
        """Fetch the latest data points from the parent plot"""
        if self.proxy is None:
            return
        value, _ = get_array_data(self.proxy)
        if value is None:
            return
        self.y_values = value

        if self.baseline_proxy is None:
            offset = self.config.get("offset", 0.0)
            step = self.config.get("step", 1.0)
            self.x_values = generate_baseline(
                self.y_values, offset=offset, step=step)
        else:
            value, _ = get_array_data(self.baseline_proxy)
            if value is None:
                return
            self.x_values = value

        if len(self.x_values) != len(self.y_values):
            return
        self.data_curve.setData(self.x_values, self.y_values)
        if self.fit_on_update_checkbox.isChecked():
            self.fit()
        else:
            self.fit_curve.setVisible(False)

    @Slot()
    def fit(self):
        """Fit data with the selected function"""
        if not len(self.x_values) or not len(self.y_values):
            return

        x_values = self.x_values
        y_values = self.y_values
        if self.show_line_roi_button.isChecked():
            min_value = self.left_line.getPos()[0]
            max_value = self.right_line.getPos()[0]

            # Slice the data according to position
            size = len(x_values) - 1
            dx = float(x_values[-1] - x_values[0]) / size
            x_min = np.clip(int((min_value - x_values[0]) / dx), 0, size)
            x_max = np.clip(int((max_value - x_values[0]) / dx), 0, size)
            x_values = x_values[x_min:x_max]
            y_values = y_values[x_min:x_max]
        self._fit(x_values, y_values)

    def _fit(self, x_values, y_values):
        xfit = x_values
        fit_option = self.fit_options_combobox.currentText()
        fit_func = FIT_FUNCTION_MAP[fit_option].get("func")
        initial_parameters = None
        if fit_option in BELL_CURVES:
            initial_parameters = guess_initial_parameters(x_values, y_values)
        try:
            params, pcov = curve_fit(f=fit_func, xdata=x_values,
                                     ydata=y_values, p0=initial_parameters)
        except (RuntimeError, TypeError, ValueError):
            xfit = yfit = np.array([])
            params = perr = np.array([])
        else:
            yfit = fit_func(x_values, *params)
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

    @Slot(bool)
    def _enable_auto_update(self, enabled):
        """Update the data in the plot on the parent plot update."""
        self.proxy.on_trait_change(self.update_data,
                                   "binding:config_update", remove=not enabled)
