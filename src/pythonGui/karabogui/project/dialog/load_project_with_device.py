#############################################################################
# Author: <raul.costa@xfel.eu>
# Created on August 26, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import (
    QAbstractItemView, QDialog, QDialogButtonBox, QTableWidgetItem)

from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_config, get_db_conn
from karabogui.util import SignalBlocker

# Minimum size for a device id substring to be considered searchable.
MIN_DEVICE_ID_SIZE = 4


class LoadProjectWithDeviceDialog(QDialog):

    def __init__(self, initial_domain=None, parent=None):
        super().__init__(parent=parent)
        ui_file = str(Path(__file__).parent / "load_project_with_device.ui")
        uic.loadUi(ui_file, self)
        self.setWindowTitle("Find and Load Project with Device")
        self.initial_domain = initial_domain
        self.txt_device_id.setPlaceholderText(
            f'Enter DeviceID part (at least {MIN_DEVICE_ID_SIZE} '
            'characters)')
        self.button_box.button(QDialogButtonBox.Ok).setText("Load")
        flags = Qt.WindowCloseButtonHint
        self.setWindowFlags(self.windowFlags() | flags)

        # Subscribes for a set of broadcast events.
        self.event_map = {
            KaraboEvent.ProjectDomainsList: self._event_domain_list,
            KaraboEvent.ProjectFindWithDevice: self._event_find_with_device
        }
        register_for_broadcasts(self.event_map)

        self.setup_table()
        self.connect_widgets_events()

        self.search_domain = ""     # domain used by last project search
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
        super(LoadProjectWithDeviceDialog, self).done(result)

    # -----------------------------------------------------------------------
    # Dialog values accessors

    def selected_project_id(self):
        prj_id = None
        if len(self.tbl_projects.selectedItems()) == 3:
            prj_id = self.tbl_projects.selectedItems()[2].text()
        return prj_id

    def selected_domain(self):
        domain = None
        if len(self.cmb_domain.currentText()) > 0:
            domain = self.cmb_domain.currentText()
        return domain

    # -----------------------------------------------------------------------
    # Karabo Event Handlers

    def _event_domain_list(self, data):
        self._domains_updated(data.get('items', []))

    def _event_find_with_device(self, data):
        self._projects_with_device_updated(data.get('projects', []))

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
                current_topic = get_config()['broker_topic']
                topic_idx = self.cmb_domain.findText(current_topic)
                if topic_idx >= 0:
                    # The topic is among the domains
                    self.cmb_domain.setCurrentIndex(topic_idx)
            else:
                # The previously selected domain has been found
                self.cmb_domain.setCurrentIndex(domain_idx)
        self.loading_domains = False
        self.update_dialog_state()

    def _projects_with_device_updated(self, projects):
        self.finding_projects = False
        self.populate_projects_table(projects)
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
        cols = ['Name', 'Last Modified', 'uuid']
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
            self.tbl_projects.setItem(row, 0, QTableWidgetItem(rec['name']))
            self.tbl_projects.setItem(
                row, 1, QTableWidgetItem(rec['last_modified']))
            self.tbl_projects.setItem(row, 2, QTableWidgetItem(rec['uuid']))
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
        self.btn_find_projects.setEnabled(
            not loading_data and
            len(self.txt_device_id.text()) >= MIN_DEVICE_ID_SIZE)
        self.button_box.button(QDialogButtonBox.Ok).setEnabled(
            len(self.tbl_projects.selectedItems()) > 0)

        if self.search_domain and self.search_device_id and not loading_data:
            if self.tbl_projects.rowCount() == 0:
                self.lbl_projects.setText(
                    f'No project with "{self.search_device_id}" in '
                    f'"{self.search_domain}"'
                )
            elif self.tbl_projects.rowCount() == 1:
                self.lbl_projects.setText(
                    f'1 project with "{self.search_device_id}" in '
                    f'"{self.search_domain}"'
                )
            else:
                self.lbl_projects.setText(
                    f'{self.tbl_projects.rowCount()} projects with '
                    f'"{self.search_device_id}" in "{self.search_domain}"'
                )
        else:
            self.lbl_projects.setText('')