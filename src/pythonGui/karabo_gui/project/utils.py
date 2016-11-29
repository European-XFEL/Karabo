#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QDialog

from karabo_gui.project.dialog.object_handle import ObjectSaveDialog
from karabo_gui.singletons.api import get_db_conn


def save_object(obj):
    """ Save individual `obj` in project database

    :param obj A project model object
    """
    from karabo_gui.project.api import TEST_DOMAIN

    dialog = ObjectSaveDialog(obj.alias)
    if dialog.exec() == QDialog.Accepted:
        obj.alias = dialog.alias
        db_conn = get_db_conn()
        db_conn.store(TEST_DOMAIN, obj.uuid, obj.revision, obj)
