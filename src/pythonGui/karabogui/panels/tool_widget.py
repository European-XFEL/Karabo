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


class SearchBar(QWidget):
    def __init__(self, parent=None):
        super(SearchBar, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "filter_widget.ui"), self)

        self.tree_view = None
        self.ui_filter.returnPressed.connect(self._search_clicked)
        self.ui_search.clicked.connect(self._search_clicked)
        self.ui_clear.clicked.connect(self._clear_clicked)
        self.reset()

    def setView(self, view):
        self.tree_view = weakref.ref(view)

    # -----------------------------------------
    # Public interface

    def reset(self, enable=False):
        """Reset the model search bar"""
        self.ui_filter.setText("")
        self.ui_filter.setEnabled(enable)
        self.ui_search.setEnabled(enable)
        self.ui_clear.setEnabled(enable)

    # -----------------------------------------
    # Qt Slots

    @Slot()
    def _search_clicked(self):
        with wait_cursor():
            pattern = str(self.ui_filter.text())
            tree_view = self.tree_view()
            proxy_model = tree_view.model()
            proxy_model.setFilterFixedString(pattern)
            tree_view.expandAll()

    @Slot()
    def _clear_clicked(self):
        with wait_cursor():
            pattern = ''
            self.ui_filter.setText(pattern)
            tree_view = self.tree_view()
            proxy_model = tree_view.model()
            proxy_model.setFilterFixedString(pattern)
            # After search, the operator can clear and maintain his selection!
            # Note: Maybe expand tree view
            index = proxy_model.currentIndex()
            if index.isValid():
                tree_view.scrollTo(index)


class InterfaceBar(QWidget):
    def __init__(self, parent=None):
        super(InterfaceBar, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "interface_filter.ui"), self)

        self.ui_filter.returnPressed.connect(self._search_clicked)
        self.ui_search.clicked.connect(self._search_clicked)
        self.ui_clear.clicked.connect(self._clear_clicked)
        for interface in Interfaces:
            self.ui_interface.addItem(interface.name)

        self.reset()
        self.tree_view = None

    def setView(self, view):
        self.tree_view = weakref.ref(view)

    # -----------------------------------------
    # Public interface

    def reset(self, enable=False):
        """Reset the model search bar"""
        self.ui_filter.setText("")
        self.ui_filter.setEnabled(enable)
        self.ui_search.setEnabled(enable)
        self.ui_clear.setEnabled(enable)
        self.ui_interface.setEnabled(enable)

    # -----------------------------------------
    # Qt Slots

    @Slot()
    def _search_clicked(self):
        with wait_cursor():
            pattern = str(self.ui_filter.text())
            tree_view = self.tree_view()
            proxy_model = tree_view.model()
            proxy_model.setInterface(self.ui_interface.currentText())
            proxy_model.setFilterFixedString(pattern)
            tree_view.expandAll()

    @Slot()
    def _clear_clicked(self):
        with wait_cursor():
            pattern = ''
            self.ui_interface.setCurrentIndex(0)
            self.ui_filter.setText(pattern)
            tree_view = self.tree_view()
            proxy_model = tree_view.model()
            proxy_model.setInterface(self.ui_interface.currentText())
            proxy_model.setFilterFixedString(pattern)
            # After search, the operator can clear and maintain his selection!
            # Note: Maybe expand tree view
            index = proxy_model.currentIndex()
            if index.isValid():
                tree_view.scrollTo(index)
