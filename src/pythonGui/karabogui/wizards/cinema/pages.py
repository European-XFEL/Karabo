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
from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Qt, Signal, Slot
from qtpy.QtGui import QClipboard, QIntValidator, QPixmap
from qtpy.QtWidgets import (
    QApplication, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QListWidget,
    QListWidgetItem, QPushButton, QRadioButton, QSizePolicy, QSpacerItem,
    QVBoxLayout, QWizard, QWizardPage)

from karabogui.util import SignalBlocker

from .const import LOGO_PATH, LOGO_WIDTH

LABEL_HEIGHT = 20


class PageContainer(QWizard):
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        pixmap = QPixmap(LOGO_PATH)
        pixmap = pixmap.scaledToWidth(LOGO_WIDTH)
        self.setPixmap(QWizard.LogoPixmap, pixmap)
        self.currentIdChanged[int].connect(self.fitContents)

    def setPage(self, index, page):
        page.container = self
        super().setPage(index, page)

    @Slot(int)
    def fitContents(self, index):
        self.adjustSize()


class Page(QWizardPage):
    header = None
    subheader = None
    container = None
    ui_file = None

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        if self.ui_file is not None:
            ui_path = Path(__file__).parent / self.ui_file
            uic.loadUi(str(ui_path), self)
        self.setAttribute(Qt.WA_DeleteOnClose)

        # Initialize variables
        self._is_complete = True
        if self.header is not None:
            self.setTitle(self.header)
        if self.subheader is not None:
            self.setSubTitle(self.subheader)

        self.setup_page()

    def set_complete(self, complete):
        if self._is_complete != complete:
            self._is_complete = complete
            self.completeChanged.emit()

    def set_header(self, text):
        self.header = text
        self.setTitle(text)

    def set_subheader(self, text):
        self.subheader = text
        self.setSubTitle(text)

    def isComplete(self):
        return self._is_complete

    # Abstract interface
    # -----------------------------------------------------------------------

    def setup_page(self):
        """Reimplement to populate the page with the desired contents"""


LEFT_ARROW = "\u2190"
RIGHT_ARROW = "\u2192"


class SelectScenesPage(Page):

    def sceneAdded(self, model):
        """An empty handler to be configured"""

    def sceneRemoved(self, model):
        """An empty handler to be configured"""

    def setup_page(self):
        # Available scenes
        available_label = QLabel("Available Scenes:")
        self.available_listwidget = avail_widget = QListWidget()
        avail_widget.itemSelectionChanged.connect(self._available_selected)
        self.add_button = QPushButton(f"Add {RIGHT_ARROW}")
        self.add_button.setEnabled(False)
        self.add_button.clicked.connect(self._add_scene)
        spacer = QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Fixed)
        add_hbox = QHBoxLayout()
        add_hbox.addWidget(self.add_button)
        add_hbox.addSpacerItem(spacer)
        available_vbox = QVBoxLayout()
        available_vbox.addWidget(available_label)
        available_vbox.addWidget(avail_widget)
        available_vbox.addLayout(add_hbox)

        # Selected scenes
        selected_label = QLabel("Selected Scenes:")
        self.selected_listwidget = select_widget = QListWidget()
        select_widget.itemSelectionChanged.connect(self._selected_selected)
        self.remove_button = QPushButton(f"{LEFT_ARROW} Remove")
        self.remove_button.setEnabled(False)
        self.remove_button.clicked.connect(self._remove_scene)
        spacer = QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Fixed)

        remove_hbox = QHBoxLayout()
        remove_hbox.addWidget(self.remove_button)
        remove_hbox.addSpacerItem(spacer)
        selected_vbox = QVBoxLayout()
        selected_vbox.addWidget(selected_label)
        selected_vbox.addWidget(select_widget)
        selected_vbox.addLayout(remove_hbox)

        # Combine everything
        content_hbox = QHBoxLayout(self)
        content_hbox.addLayout(available_vbox)
        content_hbox.addLayout(selected_vbox)

        self.setLayout(content_hbox)

    def set_available_scenes(self, scenes):
        self.available_listwidget.clear()
        for text, data in scenes:
            item = QListWidgetItem(text)
            item.setData(Qt.UserRole, data)
            self.available_listwidget.addItem(item)

    def set_selected_scenes(self, scenes):
        self.selected_listwidget.clear()
        for text, data in scenes:
            item = QListWidgetItem(text)
            item.setData(Qt.UserRole, data)
            self.selected_listwidget.addItem(item)

    @Slot()
    def _available_selected(self):
        selected = self.available_listwidget.selectedItems()
        self.add_button.setEnabled(bool(len(selected)))
        # Update opposite list widget
        with SignalBlocker(self.selected_listwidget):
            self.selected_listwidget.clearSelection()
        self.remove_button.setEnabled(False)

    @Slot()
    def _selected_selected(self):
        selected = self.selected_listwidget.selectedItems()
        self.remove_button.setEnabled(bool(len(selected)))
        # Update opposite list widget
        with SignalBlocker(self.available_listwidget):
            self.available_listwidget.clearSelection()
        self.add_button.setEnabled(False)

    @Slot()
    def _add_scene(self):
        list_widget = self.available_listwidget
        selected = list_widget.selectedItems()
        if not len(selected):
            return

        # We are sure that only one text is selected
        selected = selected[0]
        list_widget.takeItem(list_widget.row(selected))
        self.selected_listwidget.addItem(selected)
        self.sceneAdded(selected.data(Qt.UserRole))

    @Slot()
    def _remove_scene(self):
        list_widget = self.selected_listwidget
        selected = list_widget.selectedItems()
        if not len(selected):
            return

        # We are sure that only one text is selected
        selected = selected[0]
        list_widget.takeItem(list_widget.row(selected))
        self.available_listwidget.addItem(selected)
        self.sceneRemoved(selected.data(Qt.UserRole))


class ConfigurePage(Page):
    header = "Create Cinema Link"
    subheader = "..for scenes in this project with the meta data"

    ui_file = "link.ui"

    configChanged = Signal(object)

    edits = None

    def setup_page(self):
        self.host_info_widget.setVisible(False)
        self.splash_checkbox.toggled.connect(self._show_splash_changed)
        self.include_host_checkbox.toggled.connect(self._show_host_info)
        self.host_lineedit.textChanged.connect(self._host_changed)
        self.port_lineedit.setValidator(QIntValidator())
        self.port_lineedit.textChanged.connect(self._port_changed)

        # Register fields
        self.edits = [self.host_lineedit, self.port_lineedit]
        for field in self.edits:
            field.textChanged.connect(self._check_empty_fields)

    @Slot()
    def _check_empty_fields(self):
        """validation of empty input fields"""
        complete = all([field.text() for field in self.edits])
        self.set_complete(complete)

    def set_config(self, config):
        self.splash_checkbox.setChecked(config["show_splash"])
        self.include_host_checkbox.setChecked(config["include_host"])
        self.host_lineedit.setText(config["host"])
        self.port_lineedit.setText(str(config["port"]))

    @Slot(bool)
    def _show_host_info(self, enabled):
        self.host_info_widget.setVisible(enabled)
        self.configChanged.emit({"include_host": enabled})

    @Slot(bool)
    def _show_splash_changed(self, enabled):
        self.configChanged.emit({"show_splash": enabled})

    @Slot()
    def _host_changed(self):
        self.configChanged.emit({"host": self.host_lineedit.text()})

    @Slot()
    def _port_changed(self):
        self.configChanged.emit({"port": self.port_lineedit.text()})


LINK_TEXT = [
    "karabo://cinema?domain=DOMAIN&scene_uuid= ...",
    "<a href='karabo://cinema?domain=DOMAIN ...'>Cinema Name</a>",
]


class LinkPage(Page):
    header = "Cinema Link Overview"
    subheader = "The following parameters have been configured ..."
    url = None

    def setup_page(self):
        vbox = QVBoxLayout(self)
        self.label = QLabel(self)
        self.label.setWordWrap(True)
        self.uuid_widget = QListWidget(self)
        description = QLabel("The link will be copied to the "
                             "clipboard on <b>Finish</b>.", self)
        vbox.addWidget(self.label)
        vbox.addWidget(self.uuid_widget)

        group = QGroupBox()
        self.classic = QRadioButton("Karabo URL", self)
        self.weblink = QRadioButton("HTML Link", self)
        self.classic.clicked.connect(self.update_url_info)
        self.weblink.clicked.connect(self.update_url_info)
        self.link_description = QLabel(self)
        self.link_description.setTextFormat(Qt.PlainText)

        name_layout = QHBoxLayout()
        self.name_label = QLabel("<b>Cinema Name</b>", self)
        self.cinema_name = QLineEdit("link text", self)

        name_layout.addWidget(self.name_label)
        name_layout.addWidget(self.cinema_name)

        # Set defaults
        self.link_description.setText(LINK_TEXT[0])
        self.classic.setChecked(True)
        self.cinema_name.setVisible(False)
        self.cinema_name.setMinimumHeight(25)
        self.name_label.setVisible(False)

        radio_layout = QHBoxLayout()
        radio_layout.addWidget(self.classic)
        radio_layout.addWidget(self.weblink)

        group_layout = QVBoxLayout()
        group_layout.addLayout(radio_layout)
        group_layout.addWidget(self.link_description)
        group_layout.addLayout(name_layout)

        group.setLayout(group_layout)
        group.adjustSize()
        vbox.addWidget(group)
        vbox.addWidget(description)

    @Slot(bool)
    def update_url_info(self, checked):
        if self.classic.isChecked():
            self.link_description.setText(LINK_TEXT[0])
            self.cinema_name.setVisible(False)
            self.name_label.setVisible(False)
        elif self.weblink.isChecked():
            self.link_description.setText(LINK_TEXT[1])
            self.cinema_name.setVisible(True)
            self.name_label.setVisible(True)

    def set_data(self, data):
        blacklist = (["uuids"] if data["include_host"]
                     else ["uuids", "host", "port"])
        meta = ("<table>" +
                "".join("<tr><td><b>{}</b>:   </td><td>{}</td></tr>".
                        format(key, str(value))
                        for key, value in data.items() if key not in blacklist)
                + "</table>")
        self.label.setText(meta)

        domain = data["domain"]
        include_host = data["include_host"]
        host = data["host"]
        port = data["port"]
        show_splash = data["show_splash"]
        uuids = data["uuids"]

        # Show the uuids in a list
        for data in uuids:
            item = QListWidgetItem(data)
            item.setData(Qt.UserRole, data)
            self.uuid_widget.addItem(item)

        # Mandatory args: domain and uuids
        args = ([f"domain={domain}"]
                + [f"scene_uuid={uuid}" for uuid in uuids])

        # Optional args: login details
        if include_host:
            args.extend([
                         f"host={host}",
                         f"port={port}",
                         ])
        # Optional args: no splash
        if not show_splash:
            args.append("nosplash")

        self.url = "karabo://cinema?" + "&".join(args)

    @Slot()
    def finished(self):
        if self.weblink.isChecked():
            self.url = f'<a href="{self.url}">{self.cinema_name.text()}</a>'
        clipboard = QApplication.clipboard()
        # Erase clipboard first!
        clipboard.clear(mode=QClipboard.Clipboard)
        clipboard.setText(self.url, mode=QClipboard.Clipboard)
