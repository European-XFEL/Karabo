#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 21, 2022
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
#############################################################################
from datetime import datetime, timezone

import numpy as np
from qtpy.QtCore import QDateTime, QSortFilterProxyModel, QStringListModel, Qt
from qtpy.QtGui import QFont
from qtpy.QtWidgets import (
    QAbstractItemView, QAction, QComboBox, QDialog, QFrame, QHBoxLayout,
    QLabel, QLineEdit, QListView, QMenu, QPushButton, QTextEdit, QVBoxLayout,
    QWidget)
from traits.api import Instance, Int, String, WeakRef, on_trait_change

from karabo.common.scenemodel.api import HistoricTextModel
from karabo.native import Timestamp
from karabogui import icons
from karabogui.binding.api import (
    StringBinding, UnsignedIntBinding, VectorStringBinding, get_binding_value,
    get_numpy_binding)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.dialogs.api import RequestTimeDialog
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName

# Time Ranges
LAST_WEEK = "Last Week"
LAST_DAY = "Last Day"
LAST_HOUR = "Last Hour"
LAST_MIN = "Last Ten Minutes"


def get_formatted(ts):
    stamp = (f"[{ts.day:02}-{ts.month:02}-{ts.year} "
             f"{ts.hour:02}:{ts.minute:02}:"
             f"{ts.second:02}.{ts.microsecond // 1000:03d}]")
    return stamp


def get_dtype(binding):
    """Return a dtype corresponding to binding"""
    fmt = "{}"
    if isinstance(binding, UnsignedIntBinding):
        dt = binding.displayType.split("|")[0]
        _fmt = {
            "bin": "0b{{:0{}b}}".format(
                np.iinfo(get_numpy_binding(binding)).bits),
            "oct": "o{:o}",
            "hex": "0x{:X}"}
        try:
            fmt = _fmt[dt]
        except (TypeError, KeyError):
            pass

    return fmt


def get_start_end_date_time(time_span):
    """ Return beginning and end time for given ``time_span``

    :returns: tuple (float, float) of start, end time.
    """
    current_date_time = QDateTime.currentDateTime()
    if time_span == LAST_WEEK:
        # One week
        start_date_time = current_date_time.addDays(-7)
    if time_span == LAST_DAY:
        # One day
        start_date_time = current_date_time.addDays(-1)
    elif time_span == LAST_HOUR:
        # One hour
        start_date_time = current_date_time.addSecs(-3600)
    elif time_span == LAST_MIN:
        # Ten minutes
        start_date_time = current_date_time.addSecs(-600)

    start_time = start_date_time.toMSecsSinceEpoch() / 1000
    current_time = current_date_time.toMSecsSinceEpoch() / 1000

    return start_time, current_time


_BINDINGS = (UnsignedIntBinding, StringBinding, VectorStringBinding)


@register_binding_controller(ui_name="Historic Text Data", can_edit=False,
                             klassname="HistoricText",
                             binding_type=_BINDINGS,
                             priority=-10)
class DisplayHistoricText(BaseBindingController):
    """
    The HistoricText Controller for viewing historic data of strings and
    vectors of strings
    """
    model = Instance(HistoricTextModel, args=())

    list_model = WeakRef(QStringListModel)
    list_view = WeakRef(QListView)
    time_span = WeakRef(QComboBox)
    status_widget = WeakRef(QLabel)
    request_button = WeakRef(QPushButton)
    text_widget = WeakRef(QTextEdit)

    maxHistory = Int(500)
    fmt = String("{}")

    def create_widget(self, parent):
        widget = QWidget(parent)
        layout = QVBoxLayout(widget)
        widget.setLayout(layout)

        bold_font = QFont()
        bold_font.setPointSize(10)
        bold_font.setBold(True)

        text_label = QLabel("Current Value")
        text_label.setFont(bold_font)

        text_widget = QTextEdit(widget)
        text_widget.setFocusPolicy(Qt.NoFocus)
        text_widget.setFixedHeight(60)
        text_widget.setAlignment(Qt.AlignCenter)
        objectName = generateObjectName(widget)
        sheet = ('QWidget#{} {{ background-color : rgba{}; }}'
                 ''.format(objectName, ALL_OK_COLOR))
        text_widget.setObjectName(objectName)
        text_widget.setStyleSheet(sheet)

        history_label = QLabel("Historic Values")
        history_label.setFont(bold_font)

        list_view = QListView(widget)
        list_view.setWordWrap(True)
        list_view.setAlternatingRowColors(True)
        list_view.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.list_model = QStringListModel(parent=list_view)

        # Set up the filter model!
        filter_model = QSortFilterProxyModel(parent=list_view)
        filter_model.setSourceModel(self.list_model)
        filter_model.setFilterRole(Qt.DisplayRole)
        filter_model.setFilterCaseSensitivity(False)
        filter_model.setFilterFixedString("")
        list_view.setModel(filter_model)

        search_layout = QHBoxLayout()
        search_layout.setContentsMargins(0, 0, 0, 0)
        search_line = QLineEdit(parent=widget)
        clear_button = QPushButton("Clear Filter", parent=widget)
        clear_button.clicked.connect(search_line.clear)
        search_line.textChanged.connect(filter_model.setFilterFixedString)

        search_layout.addWidget(search_line)
        search_layout.addWidget(clear_button)

        button_layout = QHBoxLayout()
        button_layout.setContentsMargins(0, 0, 0, 0)

        time_span = QComboBox()
        time_span.addItems([LAST_MIN, LAST_HOUR, LAST_DAY, LAST_WEEK])
        request_button = QPushButton("Request History")
        request_button.clicked.connect(self._get_history)
        button_layout.addWidget(time_span)
        button_layout.addWidget(request_button)

        status_widget = QLabel()
        status_widget.setFont(bold_font)
        status_widget.setFrameShape(QFrame.Box)

        layout.addWidget(text_label)
        layout.addWidget(text_widget)
        layout.addWidget(history_label)
        layout.addLayout(search_layout)
        layout.addWidget(list_view)
        layout.addLayout(button_layout)
        layout.addWidget(status_widget)

        self.list_view = list_view
        self.time_span = time_span
        self.status_widget = status_widget
        self.request_button = request_button
        self.text_widget = text_widget
        widget.setContextMenuPolicy(Qt.CustomContextMenu)
        widget.customContextMenuRequested.connect(self._context_menu)
        self._write_status("Historic text widget created ...")

        return widget

    def binding_update(self, proxy):
        self.fmt = get_dtype(proxy.binding)

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if value is None:
            return
        timestamp = proxy.binding.timestamp
        dt = datetime.fromtimestamp(timestamp.toTimestamp())
        stamp = get_formatted(dt)
        value = self._create_value(value)
        self.text_widget.setText(f"{stamp} {value}")

    # ----------------------------------------------------------------------
    # Internal interface

    def _create_value(self, value):
        """Transform the value"""
        try:
            return self.fmt.format(value)
        except BaseException:
            return str(value)

    def _convert_datetime_utc(self, start, end):
        """Convert datetime objects for `start` and `end` to UTC strings"""
        start = str(datetime.fromtimestamp(start, tz=timezone.utc).isoformat())
        end = str(datetime.fromtimestamp(end, tz=timezone.utc).isoformat())
        return start, end

    @on_trait_change("proxy:binding:historic_data")
    def _historic_data_arrival(self, data):
        """Historic data arrived, handle it"""
        size = len(data)
        self._write_status(f"Received historic data with {size} entries.")

        def format_entry(h):
            """Format an entry of the historic data"""
            timestamp = Timestamp.fromHashAttributes(h["v", ...])
            dt = datetime.fromtimestamp(timestamp.toTimestamp())
            stamp = get_formatted(dt)
            value = self._create_value(h["v"])
            return f"{stamp} {value}"

        data = [format_entry(h) for h in data][::-1]
        self.list_model.setStringList(data)

    # -----------------------------------------------------------------------
    # Slots

    def _context_menu(self, pos):
        menu = QMenu(parent=self.widget)
        dialog_ac = QAction(icons.clock, "Request Time", parent=self.widget)
        dialog_ac.triggered.connect(self._request_dialog)
        menu.addAction(dialog_ac)

        menu.exec(self.widget.mapToGlobal(pos))

    def _request_dialog(self):
        """Set a new time interval and request historic data"""
        start, end = QDateTime.currentDateTime(), QDateTime.currentDateTime()
        dialog = RequestTimeDialog(start=start, end=end, parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            start, end = self._convert_datetime_utc(
                *dialog.get_start_and_end_time())
            self.proxy.get_history(start, end, max_value_count=self.maxHistory)

    def _get_history(self):
        """Send a historic data request to the dataloggers"""
        start, end = self._convert_datetime_utc(
            *get_start_end_date_time(self.time_span.currentText()))
        self.proxy.get_history(start, end, max_value_count=self.maxHistory)
        self._write_status("Requesting historic data!")

    def _write_status(self, text):
        """Write a status on the label with the time point"""
        dt = datetime.fromtimestamp(Timestamp().toTimestamp())
        stamp = get_formatted(dt)
        self.status_widget.setText(f"{stamp}: {text}")
