#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 12, 2017
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
from qtpy.QtCore import QItemSelectionModel, QObject


class SelectionTracker(QObject):
    """An object which keeps track of what is selected.
    """

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self._model = None

    def grab_selection(self, selection_model):
        """'Grabs' the current selection.

        In this way, multiple selection models can maintain mutually exclusive
        selection with each other. Whenever one model gets a selection, it
        calls this method and the previously selected model is cleared. Then
        it becomes the currently selected model until this method is called
        with a different selection model.
        """
        assert isinstance(selection_model, QItemSelectionModel)

        if self._model is not None and self._model is not selection_model:
            self._model.clear()
        self._model = selection_model
