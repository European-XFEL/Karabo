from PyQt4 import uic
from PyQt4.QtCore import QProcess
from PyQt4.QtGui import QDialog
from karabogui.dialogs.extensions_updater import extensions_updater as updater
from pathlib import Path

from karabogui import icons


class UpdateDialog(QDialog):
    """
    This dialog is used for checking the current and latest versions of
    karabo-extensions package. The user may also update the package, where a
    pip install install call is done.
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(Path(__file__).parent.joinpath('update_dialog.ui'), self)

        self.lb_current.setText(updater.UNDEFINED_VERSION)
        self.lb_latest.setText(updater.UNDEFINED_VERSION)

        self.bt_refresh.setIcon(icons.refresh)
        self.pb_current.setVisible(False)
        self.pb_latest.setVisible(False)
        self.bt_update.setEnabled(False)

        # Connect signals
        self.bt_close.clicked.connect(self.accept)
        self.bt_refresh.clicked.connect(self.refresh_versions)
        self.bt_update.clicked.connect(self._on_update_clicked)

    def refresh_versions(self):
        """Method responsible for refreshing the current and latest package
        versions"""
        self._update_current_version()
        self._update_latest_version()

        current = self.lb_current.text()
        latest = self.lb_latest.text()

        if current != latest != updater.UNDEFINED_VERSION:
            self.bt_update.setEnabled(True)
        else:
            self.bt_update.setEnabled(False)

    def _update_current_version(self):
        """Updates the current version of the device"""
        self.pb_current.setVisible(True)
        self.lb_current.setVisible(False)

        current_version = updater.get_current_version()

        self.pb_current.setVisible(False)
        self.lb_current.setVisible(True)
        self.lb_current.setText(current_version)

    def _update_latest_version(self):
        """Updates the latest version of the device"""
        self.pb_latest.setVisible(True)
        self.lb_latest.setVisible(False)

        latest_version = updater.get_latest_version()

        self.pb_latest.setVisible(False)
        self.lb_latest.setVisible(True)
        self.lb_latest.setText(latest_version)

    def _on_update_clicked(self):
        """Updates guiextensions to the latest tag"""
        self.bt_refresh.setEnabled(False)
        self.bt_update.setEnabled(False)
        self.bt_update.setText('Updating...')

        tag = self.lb_latest.text()
        with updater.download_file_for_tag(tag) as wheel_file:
            # Install it using a system call
            process = QProcess(self)
            process.start('pip install --upgrade {}'.format(wheel_file))
            process.waitForFinished()

        self.bt_update.setText('Update')
        self.bt_refresh.setEnabled(True)
        self.bt_update.setEnabled(True)

        self.refresh_versions()
