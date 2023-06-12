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
import importlib
import os
import re
import sys
from argparse import ArgumentParser
from contextlib import contextmanager
from subprocess import STDOUT, CalledProcessError, check_output

import pkg_resources
import requests
from lxml import etree
from qtpy import uic
from qtpy.QtCore import QProcess, Qt, Slot
from qtpy.QtWidgets import QDialog

from karabogui import icons
from karabogui.controllers.util import load_extensions
from karabogui.dialogs.utils import get_dialog_ui

_TIMEOUT = 0.5
_TAG_REGEX = r'^\d+.\d+.\d+$'
_PKG_NAME = 'GUIExtensions'
_WHEEL_TEMPLATE = 'GUIExtensions-{}-py3-none-any.whl'
EXTENSIONS_URL_TEMPLATE = 'http://{}/karabo/karaboExtensions/tags/'
KARABO_CHANNEL = 'exflctrl01.desy.de'

UNDEFINED_VERSION = 'Undefined'


def get_current_version():
    """Gets the current version of the package"""
    try:
        importlib.reload(pkg_resources)
        package = pkg_resources.get_distribution(_PKG_NAME)
        return package.version
    except pkg_resources.DistributionNotFound:
        return UNDEFINED_VERSION


def retrieve_remote_html(remote_server):
    """Retrieves the remote tag url html and decodes it"""
    try:
        result = requests.get(remote_server, timeout=_TIMEOUT)
    except requests.Timeout:
        return None

    html = result.content.decode()
    return html


def get_latest_version(remote_server):
    """Gets the latest version of the package"""
    html = retrieve_remote_html(remote_server)
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


def _download_file_for_tag(tag, remote_server):
    """Downloads the wheel for the given tag and writes it to a file

    The file is removed on context exit."""
    wheel_file = _WHEEL_TEMPLATE.format(tag)
    wheel_path = f'{remote_server}{tag}/{wheel_file}'

    try:
        wheel_request = requests.get(wheel_path, stream=True, timeout=_TIMEOUT)
    except requests.Timeout:
        return None, f'Timeout when updating version {tag}'

    if not wheel_request.status_code == requests.codes.ok:
        return None, 'Error {} downloading version {}'.format(
            wheel_request.status_code, tag)

    with open(wheel_file, 'wb') as f:
        f.write(wheel_request.content)

    return wheel_file, None


@contextmanager
def download_file_for_tag(tag, remote_server):
    """Takes care of also cleaning resources after downloading"""
    wheel_file, err = _download_file_for_tag(tag, remote_server)

    yield wheel_file, err
    if wheel_file is not None:
        os.remove(wheel_file)


def uninstall_package():
    try:
        output = check_output([sys.executable, '-m', 'pip',
                               'uninstall', '--yes', _PKG_NAME],
                              stderr=STDOUT)
        return output.decode()
    except CalledProcessError as e:
        return f'Error uninstalling package. Is it installed? {e}'


def install_package(wheel_file):
    """Installs a given wheel file"""
    try:
        output = check_output([sys.executable, '-m', 'pip',
                               'install', '--upgrade', wheel_file],
                              stderr=STDOUT)
        # Reload the entry points
        load_extensions()
        return output.decode()
    except CalledProcessError as e:
        return f'Error installing {_PKG_NAME} package: {str(e)}'


def update_package(tag, remote_server):
    """Updates the package to the given tag version.

    :returns str:
        The output given by the process.
    """
    assert isinstance(tag, str)

    with download_file_for_tag(tag, remote_server) as (wheel_file, err):
        if err is not None:
            return err

        # Install it using a system call
        return install_package(wheel_file)


class UpdateDialog(QDialog):
    """Update dialog for the Karabo GUI extensions package

    This dialog is used for checking the current and latest versions of
    karabo-extensions package. The user may also update the package, where a
    pip install install call is done.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('update_dialog.ui'), self)
        self.setAttribute(Qt.WA_DeleteOnClose)

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
        self._remote_server = EXTENSIONS_URL_TEMPLATE.format(KARABO_CHANNEL)
        self.refresh_versions()

    def refresh_versions(self):
        """Refreshing the current and latest package versions"""
        self._update_current_version()
        self._update_latest_version()

        current = self.lb_current.text()
        latest = self.lb_latest.text()

        needs_updating = current != latest != UNDEFINED_VERSION
        self.bt_update.setEnabled(needs_updating)
        self.bt_uninstall.setEnabled(current != UNDEFINED_VERSION)

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

        latest_version = get_latest_version(self._remote_server)

        self.lb_latest.setVisible(True)
        self.lb_latest.setText(latest_version)

    def _start_process(self, cmd):
        """Starts a process with the given arguments"""
        if self._process is not None:
            self._clear_process()

        if self._process is None:
            # Create a process and connect its
            self._process = QProcess(self)
            self._process.setProcessChannelMode(QProcess.MergedChannels)
            self._process.readyRead.connect(self._on_output)
            self._process.finished.connect(self._on_finished)

            self._process.start(cmd)

    def _start_update_process(self):
        """Create a QProcess to update to the latest tag.

        This process' signals are connected to the given callbacks."""
        tag = self.lb_latest.text()
        self._wheel_file, err = _download_file_for_tag(tag,
                                                       self._remote_server)
        if err is not None:
            self.ed_log.append(err)
            return None

        cmd = f'pip install --upgrade {self._wheel_file}'
        self._start_process(cmd)

    def _start_uninstall_process(self):
        """Uninstalls the current GUIExtensions package"""
        cmd = f'pip uninstall --yes {_PKG_NAME}'
        self._start_process(cmd)

    def _update_log(self, text):
        self.ed_log.append(text)

    def _clear_process(self):
        """Clears all running process resources"""
        if self._process is not None:
            self._process.kill()

        if self._wheel_file and os.path.isfile(self._wheel_file):
            os.remove(self._wheel_file)

        self._wheel_file = None
        self._process = None

    # --------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def on_bt_refresh_clicked(self):
        self.bt_refresh.setEnabled(False)
        self.refresh_versions()
        self.bt_refresh.setEnabled(True)

    @Slot()
    def on_bt_stop_clicked(self):
        """Kills the running process"""
        self.bt_stop.setEnabled(False)
        if (self._process is not None and
                self._process.state() == QProcess.Running):
            self._clear_process()
        self.bt_stop.setEnabled(True)

    @Slot()
    def on_bt_uninstall_clicked(self):
        self.bt_uninstall.setEnabled(False)
        self.bt_update.setEnabled(False)
        self.bt_stop.setEnabled(True)
        self._start_uninstall_process()

    @Slot()
    def on_bt_update_clicked(self):
        """Updates gui extensions to the latest tag"""
        self.bt_refresh.setEnabled(False)
        self.bt_update.setEnabled(False)
        self.bt_stop.setEnabled(True)
        self._start_update_process()

    @Slot()
    def _on_output(self):
        while self._process.canReadLine():
            line = self._process.readLine()
            data = line.data().decode('utf-8')
            self._update_log(data)

    @Slot()
    def _on_finished(self):
        """Called when the uninstall finishes or crashes"""
        self.bt_refresh.setEnabled(True)
        self.bt_stop.setEnabled(False)
        self._clear_process()
        self.refresh_versions()

        # Reload the entry points
        importlib.reload(pkg_resources)
        load_extensions()

    @Slot()
    def _on_update_finished(self):
        """Called whenever the process finishes or crashes.

        Cleans any created resources and restore the interface states."""
        self._clear_process()
        self.refresh_versions()

        # Reload the entry points
        importlib.reload(pkg_resources)
        load_extensions()


def main():
    description = """Command-line tool to update the {} package"""
    ap = ArgumentParser(description=description.format(_PKG_NAME))
    ap.add_argument('-t', '--tag', nargs=1, type=str,
                    help='Desired package version (tag)')
    ap.add_argument('-u', '--uninstall', action='store_true', required=False,
                    help='Uninstalls any previous versions')
    ap.add_argument('-l', '--latest', action='store_true', required=False,
                    help='Upgrade to the latest tag')
    ap.add_argument('-V', '--version', action='store_true', required=False,
                    help='Print the installed version')
    ap.add_argument('-c', '--channel', type=str, default=KARABO_CHANNEL,
                    required=False, help='Remote Karabo server')

    args = ap.parse_args()

    if not any([args.uninstall, args.tag, args.latest, args.version]):
        print('At least one option must be given! Please use -h for help.')
        return

    remote_server = EXTENSIONS_URL_TEMPLATE.format(args.channel)

    if args.version:
        version = get_current_version()
        if version == UNDEFINED_VERSION:
            print('{} package not installed')
        else:
            print(f'{_PKG_NAME}: version {version} installed')

    elif args.uninstall:
        print(f'Uninstalling {_PKG_NAME} package')
        output = uninstall_package()
        print(output)

    elif args.tag:
        tag = args.tag[0]

        print(f'Installing {_PKG_NAME} version {tag}\n')
        output = update_package(tag, remote_server)
        print(output)

    elif args.latest:
        tag = get_latest_version(remote_server)
        print(f'Installing {_PKG_NAME} version {tag}\n')
        output = update_package(tag, remote_server)
        print(output)


if __name__ == '__main__':
    main()
