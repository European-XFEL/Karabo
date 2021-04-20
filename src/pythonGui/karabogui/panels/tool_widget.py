#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path as op
import weakref

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QWidget

from karabo.common.enums import Interfaces
from karabogui.util import wait_cursor


class BaseSearchBar(QWidget):
    ui_file = None

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(op.join(op.dirname(__file__), self.ui_file), self)
        self.ui_filter.returnPressed.connect(self._search_clicked)
        self.ui_search.clicked.connect(self._search_clicked)
        self.ui_clear.clicked.connect(self._clear_clicked)

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
            index = proxy_model.currentIndex()
            if index.isValid():
                tree_view.scrollTo(index)

    # -----------------------------------------
    # Subclass functions

    def on_search(self):
        """Subclass this method to act on searching of a pattern"""

    def on_clear(self):
        """Subclass this method to act on clearing of a pattern"""


class SearchBar(BaseSearchBar):
    ui_file = "filter_widget.ui"


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
