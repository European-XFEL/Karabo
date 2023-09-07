#############################################################################
# Author: <dennis.goeries@xfel.eu>
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
import re
import weakref

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QHeaderView, QWidget

import karabogui.icons as icons
from karabo.common.api import State
from karabo.common.enums import Interfaces
from karabogui.indicators import STATE_COLORS
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


_QEDIT_STYLE = """
QLineEdit {{
    border: 1px solid gray;
    border-radius: 2px;
    background-color: rgba{};
}}
QLineEdit:focus
{{
    border: 1px solid rgb(48,140,198);
}}
"""


_QCOMBOX_STYLE = """
QComboBox {{
    border: 1px solid gray;
    border-radius: 2px;
    background-color: rgba{};
    selection-background-color: rgb(48,140,198);
}}
"""

_CHANGED_COLOR = STATE_COLORS[State.CHANGING] + (128,)


class BaseSearchBar(QWidget):
    """The `BaseSearchBar` for filtering a`QAbstractItemModel`

    Note: By default this class does not apply a `QValidator` for text
          filtering.
    """
    ui_file = None

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_panel_ui(self.ui_file), self)
        self.ui_filter.returnPressed.connect(self._search_clicked)
        self.ui_filter.textChanged.connect(self._filter_modified)
        self.ui_search.clicked.connect(self._search_clicked)
        self.ui_clear.clicked.connect(self._clear_clicked)

        # Set StyleSheets
        self._filter_normal = _QEDIT_STYLE.format((0, 0, 0, 0))
        self._filter_changed = _QEDIT_STYLE.format(_CHANGED_COLOR)
        self.ui_filter.setStyleSheet(self._filter_normal)
        self.tree_view = None
        self._enable_filter()

    def _enable_filter(self, enable=False):
        self.ui_filter.setText("")
        self.ui_filter.setPlaceholderText("")
        self.ui_filter.setEnabled(enable)
        self.ui_search.setEnabled(enable)
        self.ui_clear.setEnabled(enable)

    def _set_filter_modified(self, modified):
        sheet = self._filter_changed if modified else self._filter_normal
        self.ui_filter.setStyleSheet(sheet)

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
    def _filter_modified(self):
        self._set_filter_modified(True)

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
            self._set_filter_modified(False)
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
            self._set_filter_modified(False)
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


class BaseSelectionBar(BaseSearchBar):
    """The `BaseSelectionBar` class that is used in the topology filtering

    This class applies a `QValidator` for instance filtering
    """
    ui_file = "combo_filter.ui"

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.ui_filter.setValidator(SearchValidator())
        self.setup_combo()
        # Set StyleSheets
        self._combo_normal = _QCOMBOX_STYLE.format((0, 0, 0, 0))
        self._combo_changed = _QCOMBOX_STYLE.format(_CHANGED_COLOR)
        self.ui_combo.setStyleSheet(self._combo_normal)
        self.ui_combo.currentIndexChanged.connect(self._combo_modified)

    def setup_combo(self):
        """Subclass this method to setup the combo filter box"""

    # -----------------------------------------

    def on_search(self):
        model = self.tree_view().model()
        model.setFilterSelection(self.ui_combo.currentText())

    def on_clear(self):
        self.ui_combo.setCurrentIndex(0)
        model = self.tree_view().model()
        model.setFilterSelection(self.ui_combo.currentText())

    def post_search(self):
        self._set_combo_modified(False)

    def post_clear(self):
        self._set_combo_modified(False)

    # -----------------------------------------
    # Qt Slots

    @Slot()
    def _combo_modified(self):
        self._set_combo_modified(True)

    def _set_combo_modified(self, modified):
        sheet = self._combo_changed if modified else self._combo_normal
        self.ui_combo.setStyleSheet(sheet)


class SearchBar(BaseSelectionBar):

    def setup_combo(self):
        self.ui_combo.addItem("No Status Filtering")
        self.ui_combo.addItem("Health Status [OK]")
        self.ui_combo.addItem("Health Status [ERROR]")
        self.ui_combo.addItem("Health Status [UNKNOWN]")
        self.ui_combo.setItemIcon(1, icons.statusOk)
        self.ui_combo.setItemIcon(2, icons.statusError)
        self.ui_combo.setItemIcon(3, icons.statusUnknown)


class InterfaceBar(BaseSelectionBar):

    def setup_combo(self):
        self.ui_combo.addItem("All Devices")
        for interface in Interfaces:
            self.ui_combo.addItem(interface.name)


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

    # -----------------------------------------

    def on_search(self):
        self.tree_view().close_popup_widget()

    def on_clear(self):
        self.tree_view().close_popup_widget()

    def post_search(self):
        self.resize_contents()

    def post_clear(self):
        self.resize_contents()
