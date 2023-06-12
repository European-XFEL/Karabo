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
from qtpy.QtWidgets import QDialog

from .utils import get_dialog_ui


class RequestTimeDialog(QDialog):
    """The RequestTime Dialog class that provides a time range"""
    def __init__(self, start, end, parent=None):
        super().__init__(parent)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("request_time_dialog.ui"), self)
        self.dt_start.setDateTime(start)
        self.dt_end.setDateTime(end)

    def get_start_and_end_time(self):
        """Return the start and end time in seconds"""
        start = self.dt_start.dateTime().toMSecsSinceEpoch() / 1000
        end = self.dt_end.dateTime().toMSecsSinceEpoch() / 1000
        return start, end
