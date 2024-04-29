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
import re
import sys
from argparse import ArgumentParser
from subprocess import STDOUT, CalledProcessError, check_output

import pkg_resources
import requests
from qtpy import uic
from qtpy.QtCore import QProcess, Qt, Slot
from qtpy.QtWidgets import QDialog

from karabogui import icons
from karabogui.controllers.util import load_extensions
from karabogui.dialogs.utils import get_dialog_ui

_TIMEOUT = 0.5
_TAG_REGEX = r"^\d+.\d+.\d+$"
_PKG_NAME = "GUIExtensions"
_PACKAGE_URL = "https://git.xfel.eu/api/v4/projects/4562/packages"
_PYPI_INDEX = f"{_PACKAGE_URL}/pypi/simple"
UNDEFINED_VERSION = "Undefined"


def get_current_version():
    """Gets the current version of the package"""
    try:
        importlib.reload(pkg_resources)
        return importlib.metadata.version(_PKG_NAME)
    except Exception:
        return UNDEFINED_VERSION


def _retrieve_package_list():
    """Retrieves the remote tag url html and decodes it"""
    try:
        result = requests.get(_PACKAGE_URL, timeout=_TIMEOUT)
    except requests.Timeout:
        return None

    return result.json()


def get_latest_version():
    """Gets the latest version of the package"""
    package_list = _retrieve_package_list()
    if package_list is None:
        return UNDEFINED_VERSION

    extension_package = [p for p in package_list if p["name"] == _PKG_NAME]
    if not extension_package:
        return UNDEFINED_VERSION

    packages = sorted(extension_package, key=lambda p: p["version"],
                      reverse=True)
    tag_regex = re.compile(_TAG_REGEX)
    for package in packages:
        tag = package["version"]
        if tag_regex.match(tag):
            return tag
    return UNDEFINED_VERSION


def uninstall_package():
    try:
        output = check_output([sys.executable, "-m", "pip",
                               "uninstall", "--yes", _PKG_NAME],
                              stderr=STDOUT)
        return output.decode()
    except CalledProcessError as e:
        return f"Error uninstalling package. Is it installed? {e}"


def install_package(version: str):
    """Installs a given wheel file"""
    try:
        output = check_output(
            [sys.executable, "-m", "pip", "install", f"{_PKG_NAME}=={version}",
             "--ignore-installed", "--disable-pip-version-check",
             "--index-url", _PYPI_INDEX],
            stderr=STDOUT)
        # Reload the entry points
        load_extensions()
        return output.decode()
    except CalledProcessError as e:
        return f"Error installing {_PKG_NAME} package: {str(e)}"


def install_specific_tag(tag: str):
    """Updates the package to the given tag version.

    :returns str:
        The output given by the process.
    """
    assert isinstance(tag, str)
    # Install it using a system call
    return install_package(tag)


class UpdateDialog(QDialog):
    """Update dialog for the Karabo GUI extensions package

    This dialog is used for checking the current and latest versions of
    karabo-extensions package. The user may also update the package, where a
    pip install install call is done.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("update_dialog.ui"), self)
        self.setModal(False)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.label_current.setText(UNDEFINED_VERSION)
        self.label_latest.setText(UNDEFINED_VERSION)
        self.button_refresh.setIcon(icons.refresh)
        self.button_clear_log.setIcon(icons.editClear)

        self.button_stop.setEnabled(False)
        self.button_update.setEnabled(False)
        self.button_close.clicked.connect(self.accept)
        self.button_clear_log.clicked.connect(self.text_log.clear)

        # Store the current running process
        self._process = None
        self.extension_url.setText(
            "The extensions can be installed via PyPi Index: "
            f"<b>{_PYPI_INDEX}</b>")
        self.refresh_versions()

    def refresh_versions(self):
        """Refreshing the current and latest package versions"""
        self._update_current_version()
        self._update_latest_version()
        current = self.label_current.text()
        latest = self.label_latest.text()

        needs_updating = current != latest != UNDEFINED_VERSION
        self.button_update.setEnabled(needs_updating)
        self.button_uninstall.setEnabled(current != UNDEFINED_VERSION)

    def _update_current_version(self):
        """Updates the current version of the device"""
        self.label_current.setVisible(False)

        current_version = get_current_version()
        self._update_log("Version refreshed")

        self.label_current.setVisible(True)
        self.label_current.setText(current_version)

    def _update_latest_version(self):
        """Updates the latest version of the device"""
        self.label_latest.setVisible(False)

        latest_version = get_latest_version()

        self.label_latest.setVisible(True)
        self.label_latest.setText(latest_version)

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
        tag = self.label_latest.text()
        if tag == UNDEFINED_VERSION:
            return
        cmd = (f"pip install {_PKG_NAME}=={tag} "
               f"--ignore-installed --index-url {_PYPI_INDEX} "
               "--disable-pip-version-check")
        self._start_process(cmd)

    def _start_uninstall_process(self):
        """Uninstalls the current GUIExtensions package"""
        cmd = f"pip uninstall --yes {_PKG_NAME}"
        self._start_process(cmd)

    def _update_log(self, text):
        self.text_log.append(text)

    def _clear_process(self):
        """Clears all running process resources"""
        if self._process is not None:
            self._process.kill()

        self._process = None

    # --------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def on_button_refresh_clicked(self):
        self.button_refresh.setEnabled(False)
        self.refresh_versions()
        self.button_refresh.setEnabled(True)

    @Slot()
    def on_button_stop_clicked(self):
        """Kills the running process"""
        self.button_stop.setEnabled(False)
        if (self._process is not None and
                self._process.state() == QProcess.Running):
            self._clear_process()
        self.button_stop.setEnabled(True)

    @Slot()
    def on_button_uninstall_clicked(self):
        self.button_uninstall.setEnabled(False)
        self.button_update.setEnabled(False)
        self.button_stop.setEnabled(True)
        self._start_uninstall_process()

    @Slot()
    def on_button_update_clicked(self):
        """Updates gui extensions to the latest tag"""
        self.button_refresh.setEnabled(False)
        self.button_update.setEnabled(False)
        self.button_stop.setEnabled(True)
        self._start_update_process()

    @Slot()
    def _on_output(self):
        while self._process.canReadLine():
            line = self._process.readLine()
            data = line.data().decode("utf-8")
            self._update_log(data)

    @Slot()
    def _on_finished(self):
        """Called when the uninstall finishes or crashes"""
        self.button_refresh.setEnabled(True)
        self.button_stop.setEnabled(False)
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
    ap.add_argument("-t", "--tag", nargs=1, type=str,
                    help="Desired package version (tag)")
    ap.add_argument("-u", "--uninstall", action="store_true", required=False,
                    help="Uninstalls any previous versions")
    ap.add_argument("-l", "--latest", action="store_true", required=False,
                    help="Upgrade to the latest tag")
    ap.add_argument("-V", "--version", action="store_true", required=False,
                    help="Print the installed version")
    args = ap.parse_args()

    if not any([args.uninstall, args.tag, args.latest, args.version]):
        print("At least one option must be given! Please use -h for help.")
        return

    if args.version:
        version = get_current_version()
        if version == UNDEFINED_VERSION:
            print(f"{_PKG_NAME} package not installed")
        else:
            print(f"{_PKG_NAME}: version {version} installed")

    elif args.uninstall:
        print(f"Uninstalling {_PKG_NAME} package")
        output = uninstall_package()
        print(output)

    elif args.tag:
        tag = args.tag[0]
        print(f"Installing {_PKG_NAME} version {tag}\n")
        output = install_specific_tag(tag)
        print(output)

    elif args.latest:
        tag = get_latest_version()
        print(f"Installing {_PKG_NAME} version {tag}\n")
        output = install_specific_tag(tag)
        print(output)


if __name__ == "__main__":
    main()
