#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
import re
import weakref

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QHeaderView, QWidget

import karabogui.icons as icons
from karabo.common.enums import Interfaces
from karabogui.util import wait_cursor

from .utils import get_panel_ui


class SearchValidator(QValidator):

    def __init__(self, parent=None):
        super().__init__(parent)
        self.pattern = re.compile(pattern=r"^[a-zA-Z0-9\-\/\_]+$")

    def validate(self, input, pos):
        if not input:
            return self.Acceptable, input, pos

        if not self.pattern.match(input):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


class BaseSearchBar(QWidget):
    ui_file = None

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_panel_ui(self.ui_file), self)
        self.ui_filter.returnPressed.connect(self._search_clicked)
        self.ui_search.clicked.connect(self._search_clicked)
        self.ui_clear.clicked.connect(self._clear_clicked)
        # Test typical instanceId, classId filtering
        self.ui_filter.setValidator(SearchValidator())

        self.tree_view = None
        self._enable_filter()

    def _enable_filter(self, enable=False):
        self.ui_filter.setText("")
        self.ui_filter.setPlaceholderText("")
        self.ui_filter.setEnabled(enable)
        self.ui_search.setEnabled(enable)
        self.ui_clear.setEnabled(enable)

    # -----------------------------------------
    # Public interface

    def setView(self, view):
        self.tree_view = weakref.ref(view)

    def reset(self, enable=False):
        """Reset the model search bar"""
        self._enable_filter(enable)
        self._clear_clicked()

    # -----------------------------------------
    # Qt Slots

    @Slot()
    def _search_clicked(self):
        with wait_cursor():
            self.on_search()
            pattern = str(self.ui_filter.text())
            self.ui_filter.setPlaceholderText(pattern)

            # View and activate
            tree_view = self.tree_view()
            proxy_model = tree_view.model()
            proxy_model.setFilterFixedString(pattern)
            tree_view.expandAll()
            # Expand whole tree and scroll try to keep the selection
            index = self.currentIndex()
            if index.isValid():
                tree_view.scrollTo(index)
            self.post_search()

    @Slot()
    def _clear_clicked(self):
        with wait_cursor():
            self.on_clear()
            pattern = ''
            self.ui_filter.setText(pattern)
            self.ui_filter.setPlaceholderText(pattern)

            # View and activate
            tree_view = self.tree_view()
            proxy_model = tree_view.model()
            proxy_model.setFilterFixedString(pattern)
            # Collapse whole tree and scroll / expand the selection
            tree_view.collapseAll()
            index = self.currentIndex()
            if index.isValid():
                tree_view.scrollTo(index)
            self.post_clear()

    # -----------------------------------------
    # Subclass functions

    def on_search(self):
        """Subclass this method to act on searching of a pattern"""

    def on_clear(self):
        """Subclass this method to act on clearing of a pattern"""

    def post_search(self):
        """Subclass this method to act after searching of a pattern"""

    def post_clear(self):
        """Subclass this method to act after clearing of a pattern"""

    def currentIndex(self):
        """Subclass this method to retrieve the current index"""
        view = self.tree_view()
        return view.model().currentIndex()


class SearchBar(BaseSearchBar):
    ui_file = "status_filter.ui"

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.ui_status.setItemIcon(1, icons.statusOk)
        self.ui_status.setItemIcon(2, icons.statusError)
        self.ui_status.setItemIcon(3, icons.statusUnknown)

    # -----------------------------------------
    # Qt Slots

    def on_search(self):
        model = self.tree_view().model()
        model.setFilterStatus(self.ui_status.currentText())

    def on_clear(self):
        self.ui_status.setCurrentIndex(0)
        model = self.tree_view().model()
        model.setFilterStatus(self.ui_status.currentText())


class InterfaceBar(BaseSearchBar):
    ui_file = "interface_filter.ui"

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        for interface in Interfaces:
            self.ui_interface.addItem(interface.name)

    # -----------------------------------------
    # Qt Slots

    def on_search(self):
        model = self.tree_view().model()
        model.setInterface(self.ui_interface.currentText())

    def on_clear(self):
        self.ui_interface.setCurrentIndex(0)
        model = self.tree_view().model()
        model.setInterface(self.ui_interface.currentText())


class ConfiguratorSearch(BaseSearchBar):
    ui_file = "configurator_filter.ui"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.active = False
        self.setVisible(False)

    def currentIndex(self):
        return self.tree_view().currentIndex()

    @Slot()
    def toggleActivate(self):
        """Toggle the activation and swap of the qt models"""
        self.tree_view().swap_models()
        active = not self.active
        if active:
            self.reset(True)
        self.setVisible(active)
        # Finally, resize when phasing out!
        if not active:
            self.resize_contents()
        self.active = active

    def resize_contents(self):
        header = self.tree_view().header()
        header.resizeSections(QHeaderView.ResizeToContents)

    def on_search(self):
        self.tree_view().close_popup_widget()

    def on_clear(self):
        self.tree_view().close_popup_widget()

    def post_search(self):
        self.resize_contents()

    def post_clear(self):
        self.resize_contents()
