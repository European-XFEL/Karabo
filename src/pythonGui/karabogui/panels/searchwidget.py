#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2019
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

import weakref
from collections import deque

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QWidget

from karabogui import icons

from .utils import get_panel_ui


class SearchBar(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_panel_ui("project_filter.ui"), self)

        self.label_filter.textChanged.connect(self._filter_changed)
        self.label_filter.returnPressed.connect(self._arrow_right_clicked)
        self.arrow_left.setIcon(icons.arrowLeft)
        self.arrow_left.clicked.connect(self._arrow_left_clicked)
        self.arrow_right.setIcon(icons.arrowRight)
        self.arrow_right.clicked.connect(self._arrow_right_clicked)
        self.case_sensitive.clicked.connect(self._update_filter)
        self.regexp.clicked.connect(self._update_filter)

        # The list of found nodes
        self.found = []
        # A deque array indicates the current selection in `self.found`
        self.index_array = deque([])
        self.reset()

        self.model = None

    def setModel(self, model):
        handler_method = getattr(model, 'findNodes', None)
        if handler_method is None or not callable(handler_method):
            msg = ('A model must implement a "findNodes" method '
                   'if it is attached for the search bar!')
            raise RuntimeError(msg)

        handler_method = getattr(model, 'selectNode', None)
        if handler_method is None or not callable(handler_method):
            msg = ('A model must implement a "selectNode" method '
                   'if it is attached for the search bar!')
            raise RuntimeError(msg)

        self.model = weakref.ref(model)

    # -----------------------------------------
    # Public interface

    def reset(self, enable=False):
        """Reset the model search bar"""
        self.found = []
        self.index_array = deque([])
        self.label_filter.setText("")
        self.label_filter.setEnabled(enable)
        self.case_sensitive.setEnabled(enable)
        self.regexp.setEnabled(enable)
        self.arrow_left.setEnabled(False)
        self.arrow_right.setEnabled(False)

    # -----------------------------------------

    def _select_node(self):
        """Pick the first number in index array, select the corresponding node
        """
        if self.model is None:
            raise RuntimeError("Model is not set!")

        idx = next(iter(self.index_array))
        node = self.found[idx]
        parent = node.parent
        # NOTE: The node might have left already!
        if parent is not None and node in parent.children:
            self.model().selectNode(node)

    # -----------------------------------------
    # Qt Slots

    @Slot(str)
    def _filter_changed(self, text):
        """ Slot is called whenever the search filter text was changed"""
        if self.model is None:
            raise RuntimeError("Model is not set!")

        if text:
            model = self.model()
            kwargs = {'case_sensitive': self.case_sensitive.isChecked(),
                      'use_reg_ex': self.regexp.isChecked()}
            self.found = model.findNodes(text, **kwargs)
        else:
            self.found = []

        n_found = len(self.found)
        self.index_array = deque(range(n_found))
        if self.index_array:
            result = f"{n_found} Results"
            self._select_node()
        else:
            result = "No results"
        self.result.setText(result)

        enable = n_found > 0
        self.arrow_left.setEnabled(enable)
        self.arrow_right.setEnabled(enable)

    @Slot()
    def _update_filter(self):
        text = self.label_filter.text()
        self._filter_changed(text)

    @Slot()
    def _arrow_left_clicked(self):
        """rotate index array to the right, so the first index point to the
        previous found node
        """
        self.index_array.rotate(1)
        self._select_node()

    @Slot()
    def _arrow_right_clicked(self):
        """rotate index array to the left, so the first index point to the
        next found node
        """
        # Enter key press is linked to arrow right, we have to validate if
        # there are results to show
        if self.index_array:
            self.index_array.rotate(-1)
            self._select_node()
