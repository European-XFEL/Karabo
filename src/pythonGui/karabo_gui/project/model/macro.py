#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance

from karabo.common.project.api import MacroModel, write_macro
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.macro_handle import MacroHandleDialog
from karabo_gui.project.utils import save_object
from karabo_gui.util import getSaveFileName
from .bases import BaseProjectTreeItem


class MacroModelItem(BaseProjectTreeItem):
    """ A wrapper for MacroModel objects
    """
    # Redefine model with the correct type
    model = Instance(MacroModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_macro)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_macro,
                                                parent_project))
        save_action = QAction('Save', menu)
        save_action.triggered.connect(self._save_macro)
        save_as_action = QAction('Save As...', menu)
        save_as_action.triggered.connect(self._save_macro_to_file)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_action)
        menu.addAction(save_as_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.simple_name)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.file)
        item.setEditable(False)
        return item

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_macro(self, project):
        """ Remove the macro associated with this item from its project
        """
        macro = self.model
        if macro in project.macros:
            project.macros.remove(macro)

    def _edit_macro(self):
        dialog = MacroHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _save_macro(self):
        save_object(self.model)

    def _save_macro_to_file(self):
        macro = self.model
        fn = getSaveFileName(caption='Save macro to file',
                             filter='Python Macro (*.py)',
                             suffix='py',
                             selectFile=macro.simple_name)
        if not fn:
            return

        if not fn.endswith('.py'):
            fn = '{}.py'.format(fn)

        with open(fn, 'w') as fout:
            fout.write(write_macro(macro))
