from PyQt5.QtWidgets import QDialog
from qtpy import uic

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