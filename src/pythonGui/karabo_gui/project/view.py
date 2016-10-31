#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCursor, QTreeView

from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from .item_model import ProjectItemModel


class ProjectView(QTreeView):
    """ An object representing the view for a Karabo project
    """
    def __init__(self, parent=None):
        super(ProjectView, self).__init__(parent)

        item_model = ProjectItemModel(self)
        self.setModel(item_model)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)

    # ----------------------------
    # Qt methods

    def closeEvent(self, event):
        self.destroy()
        event.accept()

    # ----------------------------
    # Public methods

    def destroy(self):
        """ Do some cleanup of the project's objects before death.
        """
        self.model().set_traits_model(None)

    # ----------------------------
    # Private methods

    def _show_context_menu(self):
        indices = self.selectionModel().selectedIndexes()
        if not indices:
            return

        first_index = indices[0]
        model_ref = first_index.data(PROJECT_ITEM_MODEL_REF)
        model = model_ref()
        if model is not None:
            menu = model.context_menu(self)
            menu.exec(QCursor.pos())
