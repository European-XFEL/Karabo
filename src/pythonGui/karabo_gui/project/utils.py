#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from types import MethodType
import weakref

from PyQt4.QtGui import QDialog

from karabo_gui.project.dialog.object_handle import ObjectSaveDialog
from karabo_gui.singletons.api import get_db_conn


class WeakMethodRef(object):
    """ A weakref.ref() for bound methods
    """
    def __init__(self, bound_method, num_args=-1):
        # Preconditions...
        # bound_method MUST be a bound method
        assert type(bound_method) is MethodType
        if num_args > -1:
            # bound_method MUST take N args (- 1 because of self)!
            needed_args = bound_method.__func__.__code__.co_argcount - 1
            assert needed_args == num_args

        obj = bound_method.__self__
        if obj is not None:
            self.obj = weakref.ref(obj, self._owner_deleted)
            self.name = bound_method.__name__

    def __call__(self, *args, **kwargs):
        if self.obj is not None:
            obj = self.obj()
            if obj is not None:
                method = getattr(obj, self.name)
                method(*args, **kwargs)

    def _owner_deleted(self, ref):
        self.obj = None


def save_object(obj):
    """ Save individual `obj` in project database

    :param obj A project model object
    """
    from karabo_gui.project.api import TEST_DOMAIN

    dialog = ObjectSaveDialog(alias=obj.alias)
    if dialog.exec() == QDialog.Accepted:
        obj.alias = dialog.alias
        db_conn = get_db_conn()
        db_conn.store(TEST_DOMAIN, obj.uuid, obj.revision, obj)
