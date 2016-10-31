#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance

from karabo.common.project.api import DeviceConfigurationModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from .base import BaseProjectTreeItem


class DeviceConfigurationModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceConfigurationModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceConfigurationModel)

    def context_menu(self, parent):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        return menu

    def _get_qt_item(self):
        item = QStandardItem(self.model.class_id)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.file)
        item.setEditable(False)
        return item
