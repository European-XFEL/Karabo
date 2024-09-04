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
import datetime
import time
from functools import partial

import numpy as np
from qtpy.QtCore import QPoint, Qt
from qtpy.QtGui import QColor, QPainter, QPainterPath, QPen
from qtpy.QtWidgets import (
    QAction, QActionGroup, QHBoxLayout, QInputDialog, QLineEdit, QWidget)
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import SparklineModel
from karabo.native import Timestamp
from karabogui.binding.api import FloatBinding, IntBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import SignalBlocker

SPARK_MIN_HEIGHT = 50
SPARK_MIN_WIDTH = 200
# timebases available for the widget, values in seconds
TIMEBASES = (('60s', 60), ('10m', 600), ('10h', 36000))


class SparkRenderer(QWidget):
    """A renderer for sparklines using QPainter
    """
    # the widget has a fixed time base
    time_base = 600  # seconds
    # maximum number of points to use, determines the binning of data
    _p_max = 120
    # plot fraction in y-direction to use
    _plot_frac = 0.9
    # size in pixels of the min max indication area
    _min_max_indic_size = 60
    # relative epsilon within within the tendency will be considered stable
    _tendency_eps = 0.05
    # size of the window in bins which is used for evaluating trend indication
    _indicator_window_size = 5

    def __init__(self, parent):
        super().__init__(parent)

        self.pen = QPen(Qt.black, 2, Qt.SolidLine)
        self.no_pen = QPen(Qt.blue, 1, Qt.SolidLine)

        self.yrange = None
        self.reset_range = False
        self.ystep = None
        self.tvals = np.arange(self._p_max)

        self.yvals = np.zeros(self._p_max, np.double)

        self.finfo = np.finfo(np.float64)
        self.ymax = np.ones(self._p_max, np.float64) * self.finfo.min
        self.ymin = np.ones(self._p_max, np.float64) * self.finfo.max
        self.ycnts = np.zeros(self._p_max, int)

        self.painter_path = None
        self.painter_path_min = None
        self.painter_path_max = None

        self.then = time.time()

        self.tendency = 0
        self.tendency_indicators = {}
        self.tendency_indicators[0] = {'x': np.array([0., 10., 0., 0.]),
                                       'y': np.array([-5., 0., 5., -5.])}

        self.tendency_indicators[1] = {'x': np.array([-5., 0., 5., -5.]),
                                       'y': np.array([10., 0., 10., 10.])}

        self.tendency_indicators[-1] = {'x': np.array([-5., 0., 5., -5.]),
                                        'y': np.array([-7., 3., -7., -7.])}

    def _scale_y(self, val, height):
        if self.yrange is None:
            return

        dy = self.yrange[1] - self.yrange[0]
        if dy == 0:
            return

        scaled = (val - self.yrange[0]) / dy
        scaled /= self.ystep
        scaled = (height - scaled + self.offset).astype(np.int64)
        return scaled

    def resizeEvent(self, event):
        self._update_ranges(self.yvals[-1])
        self._generate_curve()
        event.accept()

    def _generate_curve(self):
        width = self.width() - self._min_max_indic_size
        height = self._plot_frac * self.height()
        step = width / self._p_max
        nonzero = np.nonzero(self.ycnts)
        x = (self.tvals[nonzero] * step).astype(np.int32)
        y = self._scale_y(self.yvals[nonzero] / self.ycnts[nonzero], height)

        if y is None:
            return

        ymin = self._scale_y(self.ymin[nonzero], height)
        ymax = self._scale_y(self.ymax[nonzero], height)

        # evaluate tendency for last 5 point intervals
        iws = self._indicator_window_size
        c_idx_nz = np.nonzero(self.ycnts[-iws:])
        t_idx_nz = np.nonzero(self.ycnts[-2 * iws:-iws])
        if np.any(c_idx_nz) and np.any(t_idx_nz):
            current_avg = np.mean(self.yvals[-iws:][c_idx_nz]
                                  / self.ycnts[-iws:][c_idx_nz])
            test_avg = np.mean(self.yvals[-2 * iws:-iws][t_idx_nz]
                               / self.ycnts[-2 * iws:-iws][t_idx_nz])
            delta = (current_avg - test_avg) / test_avg if test_avg else 0.
            if abs(delta) <= self._tendency_eps or not np.isfinite(delta):
                self.tendency = 0
            else:
                self.tendency = np.sign(delta)

        self.painter_path = self._createPainterPath(x, y)
        self.painter_path_min = self._createPainterPath(x, ymin)
        self.painter_path_max = self._createPainterPath(x, ymax)

    def paintEvent(self, event):
        if self.painter_path is None:
            return

        width = self.width() - self._min_max_indic_size
        height = self._plot_frac * self.height()

        with QPainter(self) as painter:
            painter.setPen(self.pen)

            painter.setPen(self.pen)
            painter.drawPath(self.painter_path)

            pen = QPen(Qt.gray, 1, Qt.SolidLine)
            painter.setPen(pen)

            painter.drawPath(self.painter_path_min)
            painter.drawPath(self.painter_path_max)

            # add min and max value indicators
            pen = QPen(Qt.black, 1, Qt.SolidLine)
            painter.setPen(pen)
            x_points = [width, width]
            y_points = [0, self.height()]
            painter.drawPath(self._createPainterPath(np.array(x_points),
                                                     np.array(y_points)))

            # add min max indicator
            font = painter.font()
            font.setPointSize(8)
            font.setFamily("Monospace")
            painter.setFont(font)
            painter.drawText(QPoint(int(width) + 5, 10),
                             self._returnFormatted(self.yrange[1]))
            painter.drawText(QPoint(int(width) + 5, int(height) + 2),
                             self._returnFormatted(self.yrange[0]))

            #  add tendency indicator
            x_points = self.tendency_indicators[self.tendency]['x'] + width
            y_points = self.tendency_indicators[self.tendency]['y']
            dy = height / 2 + 2.5
            if self.tendency != 0:
                dy = height / 2 + (-1) * self.tendency * height / 2
            path = self._createPainterPath(np.array(x_points),
                                           np.array(y_points) + dy)
            painter.fillPath(path, Qt.magenta)
            painter.drawPath(path)

            # add timebase info
            tb = self.time_base
            if tb >= 3600:
                tb = f"{tb // 3600:d}h"
            elif tb >= 600:
                tb = f"{tb // 60:d}m"
            else:
                tb = f"{tb:d}s"

            dy = self.height() - 13
            path = self._createPainterPath(np.array([3., 28., 28., 3., 3.]),
                                           np.array(
                                               [0., 0., 12., 12., 0.]) + dy)
            painter.fillPath(path, QColor(255, 255, 255, 200))
            painter.drawPath(path)
            painter.drawText(QPoint(5, self.height() - 2), tb)

    def _returnFormatted(self, v):
        if v == 0:
            return f"{v:+0.2f}"

        exp = np.log10(abs(v))
        if abs(exp) <= 2:
            return f"{v:+0.2f}"
        else:
            return f"{v:+0.1e}"

    def _createPainterPath(self, x, y):
        path = QPainterPath()
        path.moveTo(x[0], y[0])
        for i in range(1, x.size):
            path.lineTo(x[i], y[i])
        return path

    def setData(self, value, timestamp):
        """Set a new data point with associated timestamp to the spark line
        Set the value to `None` if no bound is set.
        """
        t = timestamp.toTimestamp()

        # check if we need to roll to the next bin
        dt = int((t - self.then) / (self.time_base / self._p_max))
        if dt >= 1:
            # the sparkline might need to cover "gaps" in its data series
            # e.g. when for a given bin, no data was pushed to the widget
            # in this case we fill with the value of the previous bin.
            # If the sparkline does not receive any value updates from a
            # device for dt > time_base, indexing by [-dt:] would result
            # in an IndexError. In these cases we set all values in the
            # sparkline to the value of the last update, consitent with the
            # gap filling behaviour for valid "dt"s.
            if dt < self.yvals.size:
                self.yvals = np.roll(self.yvals, -dt)
                self.yvals[-dt:-1] = self.yvals[-dt - 1]
                self.yvals[-1] = 0.
                self.ycnts = np.roll(self.ycnts, -dt)
                self.ycnts[-dt:-1] = self.ycnts[-dt - 1]
                self.ycnts[-1] = 0.
                self.ymax = np.roll(self.ymax, -dt)
                self.ymax[-dt:-1] = self.ymax[-dt - 1]
                self.ymin = np.roll(self.ymin, -dt)
                self.ymin[-dt:-1] = self.ymin[-dt - 1]
            else:
                self.yvals[:] = self.yvals[-1]
                self.ycnts[:] = self.ycnts[-1]
                self.ymax[:] = self.ymax[-1]
                self.ymin[:] = self.ymin[-1]

            self.then = time.time()

        self.yvals[-1] += value
        self.ycnts[-1] += 1
        self.ymax[-1] = max(self.ymax[-1], value)
        self.ymin[-1] = min(self.ymin[-1], value)
        self._update_ranges(value)
        # generate the curve with the painter path
        self._generate_curve()

    def setSeries(self, x, y):
        """Set a series of data to the sparkline, e.g. historic data

        Note that this will not redraw the sparkline until the next
        setData update.
        """
        assert x.size == y.size

        if y.size == 0:
            return

        now = time.time()
        x = now - x
        binned, bins = np.histogram(x, bins=self._p_max,
                                    range=[0, self.time_base], weights=y)

        counts, _ = np.histogram(x, bins=self._p_max,
                                 range=[0, self.time_base])

        self.yvals = binned[::-1]
        self.ycnts = counts[::-1]

        bin_idx = np.digitize(x, bins)

        self.ymin = np.zeros(self.yvals.shape, np.float64)
        self.ymax = np.zeros(self.yvals.shape, np.float64)

        for i in range(self._p_max):
            idx = bin_idx == i
            if idx.any():
                self.ymin[i] = np.min(y[idx])
                self.ymax[i] = np.max(y[idx])

        self.ymin = self.ymin[::-1]
        self.ymax = self.ymax[::-1]

        self.yrange = (np.min(y), np.max(y))

    def _update_ranges(self, value):
        """Update data ranges as needed, depending on values.
        """
        # the y range which simply needs to scale to new maxima and minima
        if self.yrange is None or self.reset_range:
            # initially only single value
            self.yrange = (min(0.9 * value, 1.1 * value),
                           max(0.9 * value, 1.1 * value))
            self.reset_range = False
        else:
            self.yrange = min(value, *self.yrange), max(value, *self.yrange)

        # scale to plot fraction
        pheight = self._plot_frac * self.height()
        self.offset = (self.height() - pheight) // 2
        self.ystep = 1. / pheight

    def update_time_base(self, time_base):
        self.time_base = time_base


@register_binding_controller(ui_name='Sparkline', klassname='DisplaySparkline',
                             binding_type=(FloatBinding, IntBinding),
                             can_show_nothing=False)
class DisplaySparkline(BaseBindingController):
    """Displays a spark line with a fixed time basis and coarsing of data

    On initialization and visibility changes historic data is requested.
    Rendering is performed via the SparkRenderer class
    """
    # The scene model class used by this controller
    model = Instance(SparklineModel, args=())

    line_edit = Instance(QLineEdit)
    render_area = Instance(SparkRenderer)
    action_show_format = Instance(QAction)

    def create_widget(self, parent):
        widget = QWidget(parent)
        layout = QHBoxLayout(widget)
        widget.setMinimumHeight(SPARK_MIN_HEIGHT)

        self.line_edit = QLineEdit(widget)
        self.line_edit.setReadOnly(True)
        self.render_area = SparkRenderer(widget)
        self.render_area.setMinimumWidth(SPARK_MIN_WIDTH)
        self.render_area.time_base = self.model.time_base

        layout.addWidget(self.line_edit)
        self.line_edit.setVisible(self.model.show_value)
        layout.addWidget(self.render_area)

        action_show_value = QAction("Show value", widget, checkable=True)
        action_show_value.toggled.connect(self.toggle_show_value)
        action_show_value.setChecked(self.model.show_value)
        widget.addAction(action_show_value)

        self.action_show_format = QAction("Set value format", widget)
        self.action_show_format.triggered.connect(self._change_show_format)
        self.action_show_format.setEnabled(self.model.show_value)
        widget.addAction(self.action_show_format)

        def change_time_base(tb):
            self.model.time_base = tb

        time_base_separator = QAction("Timebase", widget)
        time_base_separator.setSeparator(True)
        widget.addAction(time_base_separator)
        time_base_group = QActionGroup(widget)
        for label, base in TIMEBASES:
            tbAction = QAction(f"{label} time base",
                               time_base_group, checkable=True)
            tbAction.toggled.connect(partial(change_time_base, base))

            if self.model.time_base == base:
                tbAction.setChecked(True)

            time_base_group.addAction(tbAction)

        widget.addActions(time_base_group.actions())
        return widget

    def binding_update(self, proxy):
        now = time.time()
        self._fetch_property_history(now - self.model.time_base, now)

    def value_update(self, proxy):
        self._draw(proxy.value, proxy.binding.timestamp)

    def toggle_show_value(self, value):
        self.model.show_value = value

    def toggle_alarm_range(self, value):
        self.model.alarm_range = value

    @on_trait_change('proxy:binding:historic_data')
    def _historic_data_arrived(self, data):
        if not data:
            return

        datasize = len(data)
        x = np.empty(datasize, dtype=float)
        y = np.empty(datasize, dtype=float)

        for i, d in enumerate(data):
            x[i] = Timestamp.fromHashAttributes(d['v', ...]).toTimestamp()
            y[i] = d["v"]

        self.render_area.setSeries(x[:-1], y[:-1])
        # use the last data point to trigger a redraw
        self._draw(y[-1], Timestamp.fromHashAttributes(data[-1]['v', ...]))

    @on_trait_change('proxy.visible', post_init=True)
    def _visibility_update(self, visible):
        if visible:
            now = time.time()
            self._fetch_property_history(now - self.model.time_base, now)

    @on_trait_change('model:time_base')
    def _update_time_base(self, timebase):
        self.render_area.update_time_base(timebase)
        now = time.time()
        self._fetch_property_history(now - timebase, now)

    @on_trait_change('model:show_value')
    def _show_value_update(self, value):
        self.line_edit.setVisible(value)
        self.action_show_format.setEnabled(value)

    @on_trait_change('model:show_format')
    def _show_format_update(self):
        self._set_text()

    def _change_show_format(self):
        form, ok = QInputDialog.getText(self.widget, "Enter Format",
                                        "", text=self.model.show_format)
        if ok:
            self.model.show_format = form

    def _draw(self, value, timestamp):
        """ Draw data vs. time"""
        self.render_area.setData(value, timestamp)
        self._set_text()

    def _fetch_property_history(self, t0, t1):
        if not self.proxy.visible:
            return
        t0 = str(datetime.datetime.fromtimestamp(t0,
                                                 tz=datetime.UTC).isoformat())
        t1 = str(datetime.datetime.fromtimestamp(t1,
                                                 tz=datetime.UTC).isoformat())
        self.proxy.get_history(t0, t1)

    def _set_text(self):
        value = get_binding_value(self.proxy)
        if self.line_edit is None or value is None:
            return
        template = "{:" + self.model.show_format + "}"
        with SignalBlocker(self.line_edit):
            self.line_edit.setText(template.format(value))
