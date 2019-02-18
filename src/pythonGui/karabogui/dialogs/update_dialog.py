from argparse import ArgumentParser
from contextlib import contextmanager
import os
import os.path as op
import pkg_resources
import re
import requests
from subprocess import check_output, STDOUT, CalledProcessError

from lxml import etree
from PyQt4 import uic
from PyQt4.QtCore import QProcess, pyqtSlot
from PyQt4.QtGui import QDialog

from karabogui import icons


_TIMEOUT = 0.1
_TAG_REGEX = '^\d+\.\d+\.\d+$'
_PKG_NAME = 'GUI_Extensions'
_WHEEL_TEMPLATE = 'GUI_Extensions-{}-py3-none-any.whl'
_REMOTE_SVR = 'http://exflserv05.desy.de/karabo/karaboExtensions/tags/'

UNDEFINED_VERSION = 'Undefined'


def get_current_version():
    """Gets the current version of the package"""
    try:
        package = pkg_resources.get_distribution('GUI-Extensions')
        return package.version
    except pkg_resources.DistributionNotFound:
        return UNDEFINED_VERSION


def retrieve_remote_html():
    """Retrieves the remote tag url html and decodes it"""
    try:
        result = requests.get(_REMOTE_SVR, timeout=_TIMEOUT)
    except requests.Timeout:
        return None

    html = result.content.decode()
    return html


def get_latest_version():
    """Gets the latest version of the package"""
    html = retrieve_remote_html()
    if not html:
        return UNDEFINED_VERSION

    table = etree.HTML(html).find('body/table')
    if not len(table):
        return UNDEFINED_VERSION

    # Match entries of the type N.N.N
    tag_regex = re.compile(_TAG_REGEX)

    # All the tags are in an href tag
    for href in reversed(table.xpath('//a/@href')):
        tag = href.strip('/')
        if tag_regex.match(tag):
            return tag

    return UNDEFINED_VERSION


def _download_file_for_tag(tag):
    """Downloads the wheel for the given tag and writes it to a file

    The file is removed on context exit."""
    wheel_file = _WHEEL_TEMPLATE.format(tag)
    wheel_path = '{}{}/{}'.format(_REMOTE_SVR, tag, wheel_file)

    try:
        wheel_request = requests.get(wheel_path, stream=True, timeout=_TIMEOUT)
    except requests.Timeout:
        return None, 'Timeout when updating version {}'.format(tag)

    if not wheel_request.status_code == requests.codes.ok:
        return None, 'Error {} downloading version {}'.format(
            wheel_request.status_code, tag)

    with open(wheel_file, 'wb') as f:
        f.write(wheel_request.content)

    return wheel_file, None


@contextmanager
def download_file_for_tag(tag):
    """Takes care of also cleaning resources after downloading"""
    wheel_file, err = _download_file_for_tag(tag)

    yield wheel_file, err

    if wheel_file is not None:
        os.remove(wheel_file)


def update_package(tag):
    """Updates the package to the given tag version.

    :returns str:
        The output given by the process.
    """
    assert isinstance(tag, str)

    with download_file_for_tag(tag) as (wheel_file, err):
        if err is not None:
            return err

        # Install it using a system call
        try:
            output = check_output(['pip', 'install', '--upgrade', wheel_file],
                                  stderr=STDOUT)
            return output.decode()
        except CalledProcessError as e:
            return str(e)


class UpdateDialog(QDialog):
    """Update dialog for the Karabo GUI extensions package

    This dialog is used for checking the current and latest versions of
    karabo-extensions package. The user may also update the package, where a
    pip install install call is done.
    """

    def __init__(self, parent=None):
        super(UpdateDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), 'update_dialog.ui'), self)

        self.lb_current.setText(UNDEFINED_VERSION)
        self.lb_latest.setText(UNDEFINED_VERSION)
        self.bt_refresh.setIcon(icons.refresh)
        self.bt_clear_log.setIcon(icons.editClear)

        self.bt_stop.setEnabled(False)
        self.bt_update.setEnabled(False)

        # Connect signals
        self.bt_close.clicked.connect(self.accept)
        self.bt_clear_log.clicked.connect(self.ed_log.clear)

        # Store the current running process
        self._process = None
        self._wheel_file = None

        self.refresh_versions()

    def refresh_versions(self):
        """Refreshing the current and latest package versions"""
        self._update_current_version()
        self._update_latest_version()

        current = self.lb_current.text()
        latest = self.lb_latest.text()

        needs_updating = current != latest != UNDEFINED_VERSION
        self.bt_update.setEnabled(needs_updating)

    def _update_current_version(self):
        """Updates the current version of the device"""
        self.lb_current.setVisible(False)

        current_version = get_current_version()
        self._update_log('Versions refreshed')

        self.lb_current.setVisible(True)
        self.lb_current.setText(current_version)

    def _update_latest_version(self):
        """Updates the latest version of the device"""
        self.lb_latest.setVisible(False)

        latest_version = get_latest_version()

        self.lb_latest.setVisible(True)
        self.lb_latest.setText(latest_version)

    def _start_update_process(self):
        """Create a QProcess to update to the latest tag. This
        process' signals are connected to the given callbacks."""
        tag = self.lb_latest.text()
        self._wheel_file, err = _download_file_for_tag(tag)
        if err is not None:
            self.ed_log.append(err)
            return None

        if self._process is None:
            # Create a process and connect its signals
            self._process = QProcess(self)
            self._process.setProcessChannelMode(QProcess.MergedChannels)
            self._process.readyRead.connect(self._on_output)
            self._process.finished.connect(self._on_finished)

        self._process.start('pip install --upgrade {}'
                            .format(self._wheel_file))

    def _update_log(self, text):
        self.ed_log.append(text)

    # --------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot()
    def on_bt_refresh_clicked(self):
        self.refresh_versions()

    @pyqtSlot()
    def on_bt_stop_clicked(self):
        """Kills the running process"""
        if (self._process is not None and
                    self._process.state() == QProcess.Running):
            self._process.kill()
            self._on_finished()

    @pyqtSlot()
    def on_bt_update_clicked(self):
        """Updates gui extensions to the latest tag"""
        self.bt_refresh.setEnabled(False)
        self.bt_update.setEnabled(False)
        self.bt_stop.setEnabled(True)
        self._start_update_process()

    @pyqtSlot()
    def _on_output(self):
        size = self._process.size()
        data = self._process.readData(size)
        self._update_log(data.decode())

    @pyqtSlot()
    def _on_finished(self):
        """Method called whenever the process finishes or crashes. It's used
        to clean any created resources and restore the interface states."""
        if self._wheel_file and os.path.isfile(self._wheel_file):
            os.remove(self._wheel_file)

        self._wheel_file = None
        self._process = None

        self.bt_refresh.setEnabled(True)
        self.bt_stop.setEnabled(False)
        self.refresh_versions()


def main():
    description = """Command-line tool to update the GUI-Extensions package"""
    ap = ArgumentParser(description=description)
    ap.add_argument('-t', '--tag', nargs=1, required=True, type=str,
                    help='Desired package version (tag)')

    args = ap.parse_args()

    if args.tag:
        tag = args.tag[0]

        print('Installing GUI-Extensions version {}\n'.format(tag))
        output = update_package(tag)
        print(output)


if __name__ == '__main__':
    main()
