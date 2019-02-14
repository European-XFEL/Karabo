from argparse import ArgumentParser
from contextlib import contextmanager
from lxml import etree
from pathlib import Path
import pkg_resources
from PyQt4 import uic
from PyQt4.QtCore import QProcess, pyqtSlot
from PyQt4.QtGui import QDialog
import re
import requests

from karabogui import icons

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
    result = requests.get(_REMOTE_SVR)
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


@contextmanager
def download_file_for_tag(tag):
    """Downloads the wheel for the given tag and writes it to a file,
    removing it when the context is finished."""
    wheel_file = _WHEEL_TEMPLATE.format(tag)
    wheel_path = '{}{}/{}'.format(_REMOTE_SVR,
                                  tag,
                                  wheel_file)

    wheel_request = requests.get(wheel_path, stream=True)
    if not wheel_request.status_code == requests.codes.ok:
        yield None, 'Error downloading wheel {}'.format(tag)
        return

    temp_file = Path(wheel_file)
    temp_file.write_bytes(wheel_request.content)

    yield temp_file, None

    # Remove downloaded wheel
    temp_file.unlink()


def update_package(tag: str):
    """Updates the package to the given tag version.

    :returns str:
        The output given by the process.
    """
    with download_file_for_tag(tag) as (wheel_file, err):
        if err is not None:
            return err

        # Install it using a system call
        process = QProcess()
        process.setProcessChannelMode(QProcess.MergedChannels)
        process.start('pip install --upgrade {}'.format(wheel_file))
        process.waitForFinished()

        output = process.readAllStandardOutput().data().decode()

        return output


class UpdateDialog(QDialog):
    """
    This dialog is used for checking the current and latest versions of
    karabo-extensions package. The user may also update the package, where a
    pip install install call is done.
    """
    def __init__(self, parent=None):
        super(UpdateDialog, self).__init__(parent)
        uic.loadUi(Path(__file__).parent.joinpath('update_dialog.ui'), self)

        self.lb_current.setText(UNDEFINED_VERSION)
        self.lb_latest.setText(UNDEFINED_VERSION)

        self.bt_refresh.setIcon(icons.refresh)
        self.bt_update.setEnabled(False)

        # Connect signals
        self.bt_close.clicked.connect(self.accept)
        self.bt_refresh.clicked.connect(self.refresh_versions)
        self.bt_clear_log.clicked.connect(self.ed_log.clear)

    @pyqtSlot()
    def on_bt_refresh_clicked(self):
        self.refresh_versions(should_log=True)

    def refresh_versions(self, should_log=False):
        """Method responsible for refreshing the current and latest package
        versions"""
        self._update_current_version()
        self._update_latest_version()

        if should_log:
            self.ed_log.append('Versions refreshed')

        current = self.lb_current.text()
        latest = self.lb_latest.text()

        if current != latest != UNDEFINED_VERSION:
            self.bt_update.setEnabled(True)
        else:
            self.bt_update.setEnabled(False)

    def _update_current_version(self):
        """Updates the current version of the device"""
        self.lb_current.setVisible(False)

        current_version = get_current_version()

        self.lb_current.setVisible(True)
        self.lb_current.setText(current_version)

    def _update_latest_version(self):
        """Updates the latest version of the device"""
        self.lb_latest.setVisible(False)

        latest_version = get_latest_version()

        self.lb_latest.setVisible(True)
        self.lb_latest.setText(latest_version)

    @pyqtSlot()
    def on_bt_update_clicked(self):
        """Updates guiextensions to the latest tag"""
        self.bt_refresh.setEnabled(False)
        self.bt_update.setEnabled(False)

        output = update_package(self.lb_latest.text())
        self.ed_log.append(output)

        self.bt_update.setText('Update')
        self.bt_refresh.setEnabled(True)
        self.bt_update.setEnabled(True)

        self.refresh_versions()


def main():
    description = """Command-line tool to update the GUI-Extensions package"""
    ap = ArgumentParser(description=description)
    ap.add_argument('-t', '--tag', nargs=1, required=True, help='Desired '
                                                                'package '
                                                                'version ('
                                                                'tag)')

    args = ap.parse_args()

    if args.tag:
        tag = args.tag[0]

        print('Installing GUI-Extensions version {}'.format(tag))
        output = update_package(tag)
        print(output)


if __name__ == '__main__':
    main()
