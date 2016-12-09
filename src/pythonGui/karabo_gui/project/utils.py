#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from types import MethodType
import weakref

from PyQt4.QtGui import QDialog

from karabo.common.savable import BaseSavableModel
from karabo.common.project.api import walk_traits_object
from karabo_gui.messagebox import MessageBox
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


def save_project(project):
    """Before saving a project make sure that all children are already
    saved"""
    model = has_modified_children(project)
    if model is not None and model is not project:
        text = 'Please save individual project sub items first'
        MessageBox.showError(text=text, title='Modified project objects')
        return
    save_object(project)


def has_modified_children(model):
    """ Check whether there are still children which ``modified`` flag is still
    True

    This recurses into all child models and checks the modified flag.

    :return The model which modified is True, else None
    """
    class _Visitor(object):
        modified_model = None

        def __call__(self, obj):
            if isinstance(obj, BaseSavableModel) and obj.modified:
                self.modified_model = obj

    visitor = _Visitor()
    walk_traits_object(model, visitor)
    return visitor.modified_model
