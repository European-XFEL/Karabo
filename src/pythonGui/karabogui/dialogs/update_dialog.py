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
from collections import defaultdict
from subprocess import STDOUT, CalledProcessError, check_output

import requests
from packaging.version import Version
from qtpy import uic
from qtpy.QtCore import (
    QAbstractAnimation, QEasingCurve, QProcess, QPropertyAnimation, Qt, Slot)
from qtpy.QtWidgets import (
    QDialog, QHeaderView, QProgressBar, QPushButton, QTableWidgetItem)

from karabogui import icons
from karabogui.controllers.util import load_extensions
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.singletons.api import get_config

_TIMEOUT = 0.5
_TAG_REGEX = r"^\d+\.\d+\.\d+(\.\d+)?$"
_PKG_NAME = "GUIExtensions"
_PKG_NAME_ALL = "GUIExtensions[all]"
_PACKAGE_URL = "https://git.xfel.eu/api/v4/projects/5187/packages"
_PYPI_INDEX = f"{_PACKAGE_URL}/pypi/simple"
UNDEFINED_VERSION = "Undefined"

DURATION = 500  # [ms]


class LoadingBar(QProgressBar):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setTextVisible(False)
        self.animation = QPropertyAnimation(
            targetObject=self, propertyName=b"value")
        self.animation.setStartValue(self.minimum())
        self.animation.setEndValue(self.maximum())
        self.animation.finished.connect(self._update_bar)
        self.animation.setDuration(DURATION)
        self.animation.setEasingCurve(QEasingCurve.InOutQuad)
        self.animation.start()

    @Slot()
    def _update_bar(self):
        if self.animation.currentValue() == self.maximum():
            self.animation.setDirection(QAbstractAnimation.Backward)
            self.setInvertedAppearance(True)
            self.animation.start()
        elif self.animation.currentValue() == self.minimum():
            self.animation.setDirection(QAbstractAnimation.Forward)
            self.setInvertedAppearance(False)
            self.animation.start()


def create_item(name: str) -> QTableWidgetItem:
    """Create a read only standard table widget item"""
    item = QTableWidgetItem(name)
    flags = item.flags()
    flags &= ~ Qt.ItemIsEditable
    item.setFlags(flags)
    return item


def get_pkg_version(name: str) -> str:
    """Gets the current version of the package"""
    try:
        return importlib.metadata.version(name)
    except Exception:
        return UNDEFINED_VERSION


def get_index_list() -> dict:
    try:
        result = requests.get(_PACKAGE_URL, timeout=_TIMEOUT)
    except requests.Timeout:
        return {}
    package_list = result.json()

    tag_regex = re.compile(_TAG_REGEX)
    uniques = defaultdict(list)
    for p in package_list:
        name = p["name"]
        version = p["version"]
        if not tag_regex.match(version):
            continue
        uniques[name].append(version)

    def safe_version(semver: str):
        try:
            return Version(semver)
        except Exception:
            return Version('0.0.0')

    return {name: max(versions, key=safe_version)
            for name, versions in uniques.items()}


def uninstall_package(package: str) -> str:
    try:
        output = check_output([sys.executable, "-m", "pip",
                               "uninstall", "--yes", package],
                              stderr=STDOUT)
        return output.decode()
    except CalledProcessError as e:
        return f"Error uninstalling package. Is it installed? {e}"


def install_package(package: str, version: str) -> str:
    """Installs a given wheel file"""
    try:
        output = check_output(
            [sys.executable, "-m", "pip", "install", f"{package}=={version}",
             "--ignore-installed", "--disable-pip-version-check",
             "--index-url", _PYPI_INDEX],
            stderr=STDOUT)
        # Reload the entry points
        load_extensions()
        return output.decode()
    except CalledProcessError as e:
        return f"Error installing {_PKG_NAME} package: {str(e)}"


def install_specific_tag(package: str, tag: str) -> str:
    """Updates the package to the given tag version.

    :returns str:
        The output given by the process.
    """
    assert isinstance(tag, str)
    # Install it using a system call
    return install_package(package, tag)


def is_package_updated(package_name: str) -> bool:
    """Checks if there are new extensions available"""

    latest_version = get_index_list().get(package_name, "0.0.0")
    current_version = get_pkg_version(package_name)
    if current_version == UNDEFINED_VERSION:
        return True
    return Version(latest_version) > Version(current_version)


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
        self.loadingbar.setVisible(False)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.label_current.setText(UNDEFINED_VERSION)
        self.label_latest.setText(UNDEFINED_VERSION)
        self.button_refresh.setIcon(icons.refresh)

        self.button_update.setEnabled(False)
        self.button_close.clicked.connect(self.accept)

        self._package_index = None
        # Store the current running process
        self._process = None
        self.extension_url.setText(
            "The extensions can be installed via PyPi Index: "
            f"<b>{_PYPI_INDEX}</b>")
        self.refresh_versions()
        self.look_for_update.setChecked(get_config()["check_updates"])
        self.look_for_update.clicked.connect(self.check_update_on_startup)

    @property
    def package_index(self):
        """Cache the package index to only load once"""
        if self._package_index is None:
            self._package_index = get_index_list()
        return self._package_index

    def _fill_package_overview(self):
        self.tableWidget.clearContents()
        self.tableWidget.setRowCount(0)

        packages = get_index_list()
        labels = ["Name", "Current Version", "Available Version", "Install",
                  "Uninstall"]
        self.tableWidget.setColumnCount(len(labels))
        self.tableWidget.setSelectionBehavior(self.tableWidget.SelectRows)
        self.tableWidget.setHorizontalHeaderLabels(labels)

        header = self.tableWidget.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.Stretch)
        header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, QHeaderView.ResizeToContents)
        row = 0
        for name, version in packages.items():
            if name == _PKG_NAME:
                continue
            self.tableWidget.insertRow(row)
            self.tableWidget.setItem(row, 0, create_item(name))
            current_version = get_pkg_version(name)
            self.tableWidget.setItem(row, 1, create_item(current_version))
            self.tableWidget.setItem(row, 2, create_item(version))

            install = QPushButton(icons.mediaStart, "Install")
            install.clicked.connect(self._table_button_clicked)
            info = {"package": name,
                    "mode": "install",
                    "version": version}
            install.setProperty("items", info)
            install.setEnabled(version != current_version)
            self.tableWidget.setCellWidget(row, 3, install)

            uninstall = QPushButton(icons.mediaBackward, "Uninstall")
            uninstall.setEnabled(current_version != UNDEFINED_VERSION)
            info = {"package": name,
                    "mode": "uninstall"}
            uninstall.setProperty("items", info)
            uninstall.clicked.connect(self._table_button_clicked)
            self.tableWidget.setCellWidget(row, 4, uninstall)
            row += 1

    @Slot()
    def _table_button_clicked(self):
        """An install or uninstall button was clicked in the tablewidget"""
        sender = self.sender()
        info = sender.property("items")
        mode = info["mode"]
        name = info["package"]
        self.tableWidget.setEnabled(False)
        if mode == "install":
            self._set_buttons_install()
            self._start_package_install(name, info["version"])
        if mode == "uninstall":
            self._set_buttons_uninstall()
            self._start_package_uninstall(name)

    def refresh_versions(self):
        """Refreshing the current and latest package versions"""
        self._update_current_version()
        self._update_latest_version()
        current = self.label_current.text()
        latest = self.label_latest.text()
        needs_updating = current != latest != UNDEFINED_VERSION
        self.button_update.setEnabled(needs_updating)
        self.button_uninstall.setEnabled(current != UNDEFINED_VERSION)
        self._fill_package_overview()

    def _update_current_version(self):
        """Updates the current version of the device"""
        self.label_current.setVisible(False)

        current_version = get_pkg_version(_PKG_NAME)
        self._update_log("Version refreshed")

        self.label_current.setVisible(True)
        self.label_current.setText(current_version)

    def _update_latest_version(self):
        """Updates the latest version of the device"""
        self.label_latest.setVisible(False)
        version = self.package_index.get(_PKG_NAME, UNDEFINED_VERSION)
        self.label_latest.setVisible(True)
        self.label_latest.setText(version)

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
            self.loadingbar.setVisible(True)
            self._process.start(cmd)

    def _start_package_install(self, package, tag):
        """Create a QProcess to update to the latest tag.

        This process' signals are connected to the given callbacks."""
        cmd = (f"pip install {package}=={tag} "
               f"--ignore-installed --index-url {_PYPI_INDEX} "
               "--disable-pip-version-check")
        self._start_process(cmd)

    def _start_package_uninstall(self, package):
        """Uninstalls the current GUIExtensions package"""
        cmd = f"pip uninstall --yes {package}"
        self._start_process(cmd)

    def _update_log(self, text):
        self.text_log.append(f"> {text}")

    def _clear_process(self):
        """Clears all running process resources"""
        if self._process is not None:
            self._process.kill()

        self._process = None

    def done(self, result):
        self._clear_process()
        return super().done(result)

    # --------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def on_button_refresh_clicked(self):
        self.button_refresh.setEnabled(False)
        self.refresh_versions()
        self.button_refresh.setEnabled(True)

    @Slot()
    def on_button_uninstall_clicked(self):
        self._set_buttons_uninstall()
        self._start_package_uninstall(_PKG_NAME)

    @Slot()
    def on_button_update_clicked(self):
        """Updates gui extensions to the latest tag"""
        self._set_buttons_install()
        tag = self.label_latest.text()
        self._start_package_install(_PKG_NAME, tag)

    @Slot()
    def _on_output(self):
        while self._process.canReadLine():
            line = self._process.readLine()
            data = line.data().decode("utf-8").strip()
            self._update_log(data)

    @Slot()
    def _on_finished(self):
        """Called when the uninstall finishes or crashes"""
        self._clear_process()
        self.refresh_versions()
        load_extensions()
        self._set_buttons_finish()
        self.loadingbar.setVisible(False)

    # Buttons
    # -----------------------------------------------------------------------

    def _set_buttons_uninstall(self):
        self.button_uninstall.setEnabled(False)
        self.button_update.setEnabled(False)
        self.tableWidget.setEnabled(False)

    def _set_buttons_install(self):
        self.tableWidget.setEnabled(False)
        self.button_refresh.setEnabled(False)
        self.button_update.setEnabled(False)
        self.button_uninstall.setEnabled(False)

    def _set_buttons_finish(self):
        self.tableWidget.setEnabled(True)
        self.button_refresh.setEnabled(True)

    @Slot(bool)
    def check_update_on_startup(self, toggled):
        """Check if the user wants to check for updates on startup."""
        get_config()['check_updates'] = toggled


class UpdateNoticeDialog(QDialog):
    """Dialog to check for updates.

    This dialog is shown when the user clicks on the "Check for updates"
    button in the settings dialog. It shows the current version and the
    latest version available. If the latest version is newer than the
    current version, it shows a button to download the update.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Update Notice")
        self.setObjectName("UpdateNoticeDialog")
        self.setModal(True)
        uic.loadUi(get_dialog_ui("update_notice_dialog.ui"), self)

        self.do_not_show.toggled.connect(self.check_update_on_startup)

        latest_version = get_index_list().get(_PKG_NAME, "0.0.0")
        current_version = get_pkg_version(_PKG_NAME)
        info_text = (
            f"A newer version of {_PKG_NAME} available: <b>{latest_version}"
            f"</b>.<br>Current version is <b>{current_version}</b>.<br> "
            f"<br>Would you like to install using the Update Dialog? <br> "
            f"<br> You can also update using the command-line utility: "
            f"<br><i>karabo-update-extensions -l </i>")
        self.info_label.setText(info_text)
        pixmap = icons.logo.pixmap(50)
        self.icon.setPixmap(pixmap)

    def accept(self):
        dialog = UpdateDialog(parent=self.parent())
        dialog.exec()
        super().accept()

    @Slot(bool)
    def check_update_on_startup(self, toggled):
        """Check if the user wants to check for updates on startup."""
        check_update = not bool(toggled)
        get_config()['check_updates'] = check_update


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
    ap.add_argument("-a", "--all", action="store_true", required=False,
                    help="Chose if all dependencies should be installed")
    args = ap.parse_args()

    if not any([args.uninstall, args.tag, args.latest, args.version]):
        print("At least one option must be given! Please use -h for help.")
        return

    if args.version:
        version = get_pkg_version(_PKG_NAME)
        if version == UNDEFINED_VERSION:
            print(f"{_PKG_NAME} package not installed")
        else:
            print(f"{_PKG_NAME}: version {version} installed")

    elif args.uninstall:
        print(f"Uninstalling {_PKG_NAME} package")
        output = uninstall_package(_PKG_NAME)
        print(output)

    elif args.tag:
        tag = args.tag[0]
        package = _PKG_NAME if not args.all else _PKG_NAME_ALL
        print(f"Installing {package} version {tag}\n")
        output = install_specific_tag(package, tag)
        print(output)

    elif args.latest:
        package_list = get_index_list()
        tag = package_list.get(_PKG_NAME, UNDEFINED_VERSION)
        package = _PKG_NAME if not args.all else _PKG_NAME_ALL
        print(f"Installing {package} version {tag}\n")
        output = install_specific_tag(package, tag)
        print(output)


if __name__ == "__main__":
    main()
