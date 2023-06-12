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
from qtpy import uic
from qtpy.QtCore import QDateTime, Qt, Slot
from qtpy.QtWidgets import QDialog

from karabogui import icons
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.singletons.api import get_network

from .utils import get_dialog_ui

ONE_WEEK = "One Week"
ONE_DAY = "One Day"
ONE_HOUR = "One Hour"
TEN_MINUTES = "Ten Minutes"


class ConfigurationFromPastDialog(QDialog):
    def __init__(self, instance_id, parent=None):
        super().__init__(parent)
        filepath = get_dialog_ui('conftime.ui')
        uic.loadUi(filepath, self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.instance_id = instance_id
        self.setModal(False)
        self.setWindowFlags(self.windowFlags() | Qt.WindowCloseButtonHint)
        self.ui_timepoint.setDateTime(QDateTime.currentDateTime())
        self.ui_instance_id.setText(instance_id)
        self.ui_request.clicked.connect(self._request_configuration)
        self.ui_show_device.clicked.connect(self._show_device)
        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        self.ui_show_device.setIcon(icons.deviceInstance)
        register_for_broadcasts(self.event_map)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    @Slot()
    def _show_device(self):
        broadcast_event(KaraboEvent.ShowDevice, {'deviceId': self.instance_id,
                                                 'showTopology': True})

    @Slot()
    def accept(self):
        """The dialog was accepted and we can request a configuration"""
        self._request_configuration()
        super().accept()

    @Slot()
    def _request_configuration(self):
        # Karabo time points are in UTC
        time_point = self.ui_timepoint.dateTime().toUTC()
        # Explicitly specifiy ISODate!
        time = str(time_point.toString(Qt.ISODate))
        preview = self.ui_preview.isChecked()
        get_network().onGetConfigurationFromPast(self.instance_id, time=time,
                                                 preview=preview)

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    def _get_time_information(self, selected_time_point):
        current_date_time = QDateTime.currentDateTime()
        if selected_time_point == ONE_WEEK:
            # One week
            time_point = current_date_time.addDays(-7)
        elif selected_time_point == ONE_DAY:
            # One day
            time_point = current_date_time.addDays(-1)
        elif selected_time_point == ONE_HOUR:
            # One hour
            time_point = current_date_time.addSecs(-3600)
        elif selected_time_point == TEN_MINUTES:
            # Ten minutes
            time_point = current_date_time.addSecs(-600)

        return time_point

    def _set_timepoint(self, selected_time_point):
        time = self._get_time_information(selected_time_point)
        self.ui_timepoint.setDateTime(time)

    @Slot()
    def on_ui_one_week_clicked(self):
        self._set_timepoint(ONE_WEEK)

    @Slot()
    def on_ui_one_day_clicked(self):
        self._set_timepoint(ONE_DAY)

    @Slot()
    def on_ui_one_hour_clicked(self):
        self._set_timepoint(ONE_HOUR)

    @Slot()
    def on_ui_ten_minutes_clicked(self):
        self._set_timepoint(TEN_MINUTES)
