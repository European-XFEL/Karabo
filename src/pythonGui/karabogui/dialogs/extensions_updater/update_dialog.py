from lxml import etree
from pathlib import Path
import pkg_resources
from PyQt4 import uic
from PyQt4.QtCore import QProcess
from PyQt4.QtGui import QDialog
import re
import requests
import os

from karabogui import icons


class UpdateDialog(QDialog):
    """
    This dialog is used for checking the current and latest versions of
    karabo-extensions package. The user may also update the package, where a
    pip install install call is done.
    """
    _PKG_NAME = 'GUI_Extensions'
    _WHEEL_TEMPLATE = 'GUI_Extensions-{}-py3-none-any.whl'
    _REMOTE_SVR = 'http://exflserv05.desy.de/karabo/karaboExtensions/tags/'

    UNDEFINED_VERSION = 'Undefined'

    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(Path(__file__).parent.joinpath('update_dialog.ui'), self)

        self.lb_current.setText(self.UNDEFINED_VERSION)
        self.lb_latest.setText(self.UNDEFINED_VERSION)

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

        if current != latest != self.UNDEFINED_VERSION:
            self.bt_update.setEnabled(True)
        else:
            self.bt_update.setEnabled(False)

    def _update_current_version(self):
        """Updates the current version of the device"""
        self.pb_current.setVisible(True)
        self.lb_current.setVisible(False)

        try:
            package = pkg_resources.get_distribution('GUI-Extensions')
            current_version = package.version
        except pkg_resources.DistributionNotFound:
            current_version = self.UNDEFINED_VERSION

        self.pb_current.setVisible(False)
        self.lb_current.setVisible(True)
        self.lb_current.setText(current_version)

    def _update_latest_version(self):
        """Updates the latest version of the device"""
        self.pb_latest.setVisible(True)
        self.lb_latest.setVisible(False)

        # Request and parse the current tag list
        result = requests.get(self._REMOTE_SVR)
        html = result.content.decode()

        if not html:
            return self.UNDEFINED_VERSION

        table = etree.HTML(html).find('body/table')
        if not table:
            return self.UNDEFINED_VERSION

        tag_regex = re.compile('^\d+\.\d+\.\d+$')

        tags = []
        for href in table.xpath('//a/@href'):
            tag = href.strip('/')
            if tag_regex.match(tag):
                tags.append(tag)

        if not tags:
            return self.UNDEFINED_VERSION

        latest_tag = tags[-1]

        self.pb_latest.setVisible(False)
        self.lb_latest.setVisible(True)
        self.lb_latest.setText(latest_tag)

    def _on_update_clicked(self):
        """Updates guiextensions to the latest tag"""
        self.bt_refresh.setEnabled(False)
        self.bt_update.setEnabled(False)
        self.bt_update.setText('Updating...')

        tag = self.lb_latest.text()

        wheel_file = self._WHEEL_TEMPLATE.format(tag)
        wheel_path = '{}{}/{}'.format(self._REMOTE_SVR,
                                      tag,
                                      wheel_file)

        with requests.get(wheel_path, stream=True) as wheel_request:
            if not wheel_request.status_code == requests.codes.ok:
                return

            temp_file = Path(wheel_file)
            temp_file.write_bytes(wheel_request.content)

            # Install it using a system call
            process = QProcess(self)
            process.start('pip install --upgrade {}'.format(temp_file))
            process.waitForFinished()

            self.bt_update.setText('Update')
            self.bt_refresh.setEnabled(True)
            self.bt_update.setEnabled(True)

            # Remove downloaded wheel
            os.remove(temp_file)

            self.refresh_versions()
