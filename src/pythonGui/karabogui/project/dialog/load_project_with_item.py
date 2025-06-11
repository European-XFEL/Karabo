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
MIN_NAME_SIZE = 5


class LoadProjectWithDialog(QDialog):

    def __init__(self, domain=None, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("load_project_with_item.ui"), self)
        self.setWindowTitle("Find and Load Project")
        self.domain = domain
        self.text_name_part.setPlaceholderText(
            f"Enter name part (at least {MIN_NAME_SIZE} "
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
            KaraboEvent.ProjectFindWithItem: self._event_find_with_item
        }
        register_for_broadcasts(self.event_map)

        self.setup_table()
        self.connect_widgets_events()

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
        project_id = None
        if len(self.table_projects.selectedItems()) == 3:
            # The project name is the first item and the uuid is its
            # associated data.
            project_name = self.table_projects.selectedItems()[0]
            project_id = project_name.data(Qt.UserRole)
        return project_id

    def selected_domain(self):
        domain = None
        if len(self.combo_domain.currentText()) > 0:
            domain = self.combo_domain.currentText()
        return domain

    @property
    def simple_name(self):
        item = self.table_projects.selectedItems()[0]
        name = item.data(Qt.DisplayRole)
        return name

    # -----------------------------------------------------------------------
    # Karabo Event Handlers

    def _event_domain_list(self, data):
        self._domains_updated(data.get("items", []))

    def _event_find_with_item(self, data):
        if self.finding_projects:
            # The results are for the search currently being expected.
            # A long-running search could have been triggered from a
            # previous instance of the dialog.
            self._projects_updated(data)

    def _domains_updated(self, domains):
        domain_to_select = self.domain
        with SignalBlocker(self.combo_domain):
            self.combo_domain.clear()
            self.combo_domain.addItems(sorted(domains))
            # Sets the selected domain to the one passed to the dialog's
            # initializer (if there was one) or sets the selected domain to
            # the current topic (if it is among the returned domains).
            domain_index = -1
            if domain_to_select:
                domain_index = self.combo_domain.findText(domain_to_select)
            if domain_index == -1:
                current_topic = get_config()["broker_topic"]
                topic_index = self.combo_domain.findText(current_topic)
                if topic_index >= 0:
                    # The topic is among the domains
                    self.combo_domain.setCurrentIndex(topic_index)
            else:
                # The previously selected domain has been found
                self.combo_domain.setCurrentIndex(domain_index)
        self.loading_domains = False
        self.update_dialog_state()

    def _projects_updated(self, data):
        self.finding_projects = False
        if data["error"] is not None:
            self.update_dialog_state()
            messagebox.show_error("Find projects failed!",
                                  details=data["error"])
        else:
            self.populate_projects_table(data["items"])
            self.update_dialog_state()

    # -----------------------------------------------------------------------
    # Widgets Event Handlers

    def connect_widgets_events(self):
        self.button_find_projects.clicked.connect(self.find_projects)
        self.combo_domain.currentIndexChanged.connect(self.refresh_search)

        self.text_name_part.textEdited.connect(self.update_dialog_state)
        self.text_name_part.returnPressed.connect(self.refresh_search)
        self.combo_item_type.currentIndexChanged.connect(self.refresh_search)

        # Double-clicking a project with a device should load it.
        self.table_projects.doubleClicked.connect(self.accept)
        self.table_projects.itemSelectionChanged.connect(
            self.update_dialog_state)

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @Slot()
    def refresh_search(self):
        if len(self.text_name_part.text()) >= MIN_NAME_SIZE:
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
        search_domain = self.combo_domain.currentText()
        name_part = self.text_name_part.text()

        if self.combo_item_type.currentText() == "Device":
            self.db_conn.get_projects_with_device(
                search_domain, name_part)
        elif self.combo_item_type.currentText() == "Macro":
            self.db_conn.get_projects_with_macro(
                search_domain, name_part)
        elif self.combo_item_type.currentText() == "Server":
            self.db_conn.get_projects_with_server(
                search_domain, name_part)

    # -----------------------------------------------------------------------
    # UI Setup and Updating

    def setup_table(self):
        cols = ["Project", "Last Modified", "Matching"]
        self.table_projects.verticalHeader().setVisible(False)
        self.table_projects.setColumnCount(len(cols))
        self.table_projects.setHorizontalHeaderLabels(cols)
        self.table_projects.horizontalHeader().setStretchLastSection(True)
        self.table_projects.setAlternatingRowColors(True)
        self.table_projects.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.table_projects.setSelectionMode(QAbstractItemView.SingleSelection)
        self.table_projects.setEditTriggers(QAbstractItemView.NoEditTriggers)

    def clear_table(self):
        self.table_projects.clearContents()
        self.table_projects.setRowCount(0)

    def populate_projects_table(self, data):
        if len(data) == 0:
            return
        self.table_projects.setSortingEnabled(False)
        self.table_projects.setUpdatesEnabled(False)
        self.table_projects.setRowCount(len(data))
        for row, rec in enumerate(data):
            project_name = QTableWidgetItem(rec["project_name"])
            project_name.setData(Qt.UserRole, rec["uuid"])
            self.table_projects.setItem(row, 0, project_name)
            self.table_projects.setItem(
                row, 1, QTableWidgetItem(utc_to_local(rec["date"])))
            items = rec.get("items", [])
            items = ", ".join(sorted(items))
            table_item = QTableWidgetItem(items)
            table_item.setToolTip(items)
            self.table_projects.setItem(row, 2, table_item)

        self.table_projects.setUpdatesEnabled(True)
        self.table_projects.setSortingEnabled(True)
        # The last column is not resized to avoid conflict with its stretch
        # attribute. Resizing it may result in uneeded space for a vertical
        # scroll bar.
        self.table_projects.resizeColumnToContents(0)
        self.table_projects.resizeColumnToContents(1)

    def update_dialog_state(self):
        loading_data = self.loading_domains or self.finding_projects
        name_part = self.text_name_part.text()

        self.combo_domain.setEnabled(not loading_data)
        self.combo_item_type.setEnabled(not loading_data)
        enabled = (not loading_data and len(name_part) >= MIN_NAME_SIZE)
        self.button_find_projects.setEnabled(enabled)
        self.button_box.button(QDialogButtonBox.Ok).setEnabled(
            len(self.table_projects.selectedItems()) > 0)

        search_domain = self.combo_domain.currentText()
        if search_domain and name_part and not loading_data:
            if self.table_projects.rowCount() == 0:
                self.label_status.setText(
                    f"No project with {name_part} in "
                    f"{search_domain}")
            elif self.table_projects.rowCount() == 1:
                self.label_status.setText(
                    f"1 project with {name_part} in "
                    f"{search_domain}")
            else:
                self.label_status.setText(
                    f"{self.table_projects.rowCount()} projects with "
                    f"{name_part} in {search_domain}")
        elif self.loading_domains:
            self.label_status.setText("Loading domains ...")
        elif self.finding_projects:
            self.label_status.setText("Finding projects ...")
        else:
            self.label_status.setText("")

        self.spin_widget.setVisible(loading_data)
