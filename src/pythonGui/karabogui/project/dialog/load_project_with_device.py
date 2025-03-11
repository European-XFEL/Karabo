#############################################################################
# Author: <raul.costa@xfel.eu>
# Created on August 26, 2021
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
#############################################################################

from qtpy import uic
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtWidgets import (
    QAbstractItemView, QDialog, QDialogButtonBox, QTableWidgetItem)

from karabogui import messagebox
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_config, get_db_conn
from karabogui.util import SignalBlocker, get_spin_widget, utc_to_local

from .utils import get_dialog_ui

# Minimum size for a device id substring to be considered searchable.
MIN_DEVICE_ID_SIZE = 5


class LoadProjectWithDeviceDialog(QDialog):

    def __init__(self, initial_domain=None, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("load_project_with_device.ui"), self)
        self.setWindowTitle("Find and Load Project with Device")
        self.initial_domain = initial_domain
        self.txt_device_id.setPlaceholderText(
            f"Enter DeviceID part (at least {MIN_DEVICE_ID_SIZE} "
            "characters)")
        self.button_box.button(QDialogButtonBox.Ok).setText("Load")
        flags = Qt.WindowCloseButtonHint
        self.setWindowFlags(self.windowFlags() | flags)

        self.spin_widget = get_spin_widget(icon="wait-black",
                                           scaled_size=QSize(16, 16))
        self.spin_widget.setVisible(False)
        self.bottom_horizontal_layout.insertWidget(0, self.spin_widget)

        # Subscribes for a set of broadcast events.
        self.event_map = {
            KaraboEvent.ProjectDomainsList: self._event_domain_list,
            KaraboEvent.ProjectFindWithDevice: self._event_find_with_device
        }
        register_for_broadcasts(self.event_map)

        self.setup_table()
        self.connect_widgets_events()

        self.search_domain = ""  # domain used by last project search
        self.search_device_id = ""  # device_id used by last project search
        self.loading_domains = False
        self.finding_projects = False

        self.db_conn = get_db_conn()
        self.db_conn.ignore_local_cache = True
        self.load_domains()

    def done(self, result):
        """ Override ``QDialog`` virtual slot.
        Unsubscribes from broadcast events.
        """
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    # -----------------------------------------------------------------------
    # Dialog values accessors

    def selected_project_id(self):
        prj_id = None
        if len(self.tbl_projects.selectedItems()) == 3:
            # The project name is the first item and the uuid is its
            # associated data.
            prj_name = self.tbl_projects.selectedItems()[0]
            prj_id = prj_name.data(Qt.UserRole)
        return prj_id

    def selected_domain(self):
        domain = None
        if len(self.cmb_domain.currentText()) > 0:
            domain = self.cmb_domain.currentText()
        return domain

    @property
    def simple_name(self):
        item = self.tbl_projects.selectedItems()[0]
        name = item.data(Qt.DisplayRole)
        return name

    # -----------------------------------------------------------------------
    # Karabo Event Handlers

    def _event_domain_list(self, data):
        self._domains_updated(data.get("items", []))

    def _event_find_with_device(self, data):
        if self.finding_projects:
            # The results are for the search currently being expected.
            # A long-running search could have been triggered from a
            # previous instance of the dialog.
            self._projects_with_device_updated(data)

    def _domains_updated(self, domains):
        domain_to_select = self.initial_domain
        with SignalBlocker(self.cmb_domain):
            self.cmb_domain.clear()
            self.cmb_domain.addItems(sorted(domains))
            # Sets the selected domain to the one passed to the dialog's
            # initializer (if there was one) or sets the selected domain to
            # the current topic (if it is among the returned domains).
            domain_idx = -1
            if domain_to_select:
                domain_idx = self.cmb_domain.findText(domain_to_select)
            if domain_idx == -1:
                current_topic = get_config()["broker_topic"]
                topic_idx = self.cmb_domain.findText(current_topic)
                if topic_idx >= 0:
                    # The topic is among the domains
                    self.cmb_domain.setCurrentIndex(topic_idx)
            else:
                # The previously selected domain has been found
                self.cmb_domain.setCurrentIndex(domain_idx)
        self.loading_domains = False
        self.update_dialog_state()

    def _projects_with_device_updated(self, data):
        self.finding_projects = False
        if data["error"] is not None:
            self.update_dialog_state()
            messagebox.show_error("Find projects with device failed!",
                                  details=data["error"])
        else:
            self.populate_projects_table(data["projects"])
            self.update_dialog_state()

    # -----------------------------------------------------------------------
    # Widgets Event Handlers

    def connect_widgets_events(self):
        self.btn_find_projects.clicked.connect(self.find_projects)
        self.cmb_domain.currentIndexChanged.connect(self.on_domain_changed)

        self.txt_device_id.textEdited.connect(self.update_dialog_state)
        self.txt_device_id.returnPressed.connect(self.on_device_id_enter)

        # Double-clicking a project with a device should load it.
        self.tbl_projects.doubleClicked.connect(self.accept)
        self.tbl_projects.itemSelectionChanged.connect(
            self.update_dialog_state)

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @Slot()
    def on_domain_changed(self):
        curr_device = self.txt_device_id.text()
        if curr_device and self.search_device_id == curr_device:
            # The user has changed the domain but kept the last device id
            # searched. Refresh the search automatically.
            self.find_projects()

    @Slot()
    def on_device_id_enter(self):
        if len(self.txt_device_id.text()) >= MIN_DEVICE_ID_SIZE:
            self.find_projects()

    # -----------------------------------------------------------------------
    # Data Fetching

    def load_domains(self):
        self.loading_domains = True
        self.db_conn.get_available_domains()
        self.update_dialog_state()

    def find_projects(self):
        self.finding_projects = True
        self.clear_table()
        self.update_dialog_state()
        self.search_domain = self.cmb_domain.currentText()
        self.search_device_id = self.txt_device_id.text()
        self.db_conn.get_projects_with_device(
            self.search_domain, self.search_device_id)

    # -----------------------------------------------------------------------
    # UI Setup and Updating

    def setup_table(self):
        cols = ["Project", "Last Modified", "Matching Devices"]
        self.tbl_projects.verticalHeader().setVisible(False)
        self.tbl_projects.setColumnCount(len(cols))
        self.tbl_projects.setHorizontalHeaderLabels(cols)
        self.tbl_projects.horizontalHeader().setStretchLastSection(True)
        self.tbl_projects.setAlternatingRowColors(True)
        self.tbl_projects.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.tbl_projects.setSelectionMode(QAbstractItemView.SingleSelection)
        self.tbl_projects.setEditTriggers(QAbstractItemView.NoEditTriggers)

    def clear_table(self):
        self.tbl_projects.clearContents()
        self.tbl_projects.setRowCount(0)

    def populate_projects_table(self, data):
        if len(data) == 0:
            return
        self.tbl_projects.setSortingEnabled(False)
        self.tbl_projects.setUpdatesEnabled(False)
        self.tbl_projects.setRowCount(len(data))
        for row, rec in enumerate(data):
            prj_name = QTableWidgetItem(rec["name"])
            prj_name.setData(Qt.UserRole, rec["uuid"])
            self.tbl_projects.setItem(row, 0, prj_name)
            self.tbl_projects.setItem(
                row, 1, QTableWidgetItem(utc_to_local(rec["last_modified"])))
            devices = ", ".join(sorted(rec["devices"]))
            device_item = QTableWidgetItem(devices)
            device_item.setToolTip(devices)
            self.tbl_projects.setItem(row, 2, device_item)
        self.tbl_projects.setUpdatesEnabled(True)
        self.tbl_projects.setSortingEnabled(True)
        # The last column is not resized to avoid conflict with its stretch
        # attribute. Resizing it may result in uneeded space for a vertical
        # scroll bar.
        self.tbl_projects.resizeColumnToContents(0)
        self.tbl_projects.resizeColumnToContents(1)

    def update_dialog_state(self):
        loading_data = self.loading_domains or self.finding_projects

        self.cmb_domain.setEnabled(not loading_data)
        enabled = (not loading_data and len(
            self.txt_device_id.text()) >= MIN_DEVICE_ID_SIZE)
        self.btn_find_projects.setEnabled(enabled)
        self.button_box.button(QDialogButtonBox.Ok).setEnabled(
            len(self.tbl_projects.selectedItems()) > 0)

        if self.search_domain and self.search_device_id and not loading_data:
            if self.tbl_projects.rowCount() == 0:
                self.lbl_status.setText(
                    f"No project with {self.search_device_id} in "
                    f"{self.search_domain}")
            elif self.tbl_projects.rowCount() == 1:
                self.lbl_status.setText(
                    f"1 project with {self.search_device_id} in "
                    f"{self.search_domain}")
            else:
                self.lbl_status.setText(
                    f"{self.tbl_projects.rowCount()} projects with "
                    f"{self.search_device_id} in {self.search_domain}")
        elif self.loading_domains:
            self.lbl_status.setText("Loading domains ...")
        elif self.finding_projects:
            self.lbl_status.setText("Finding projects ...")
        else:
            self.lbl_status.setText("")

        self.spin_widget.setVisible(loading_data)
