#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from types import MethodType
import weakref

from PyQt4.QtGui import QDialog, QMessageBox

from karabo.common.project.api import (
    BaseProjectObjectModel, walk_traits_object)
from karabo.middlelayer_api.project.api import recursive_save_object
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

    def set_alias(root, alias):
        def _visitor(model):
            if isinstance(model, BaseProjectObjectModel):
                model.alias = alias

        walk_traits_object(root, _visitor)

    dialog = ObjectSaveDialog(alias=obj.alias)
    if dialog.exec() == QDialog.Accepted:
        # XXX: Set same alias for each sub-item
        set_alias(obj, dialog.alias)
        recursive_save_object(obj, get_db_conn(), TEST_DOMAIN)


def show_save_project_message(project):
    """Check whether the given ``project`` is modified and show a messagebox to
    allow the user to confirm saving or cancel

    :return Whether the user wants to save
    """
    if project is not None and project.modified:
        ask = ('The project \"<b>{}</b>\" has be modified.<br />Do you want '
               'to save the project?').format(project.simple_name)
        options = (QMessageBox.Save | QMessageBox.No)
        reply = QMessageBox.question(None, 'Save project', ask, options,
                                     QMessageBox.Save)
        if reply == QMessageBox.No:
            return False

        if reply == QMessageBox.Save:
            return True
    return False


def maybe_save_modified_project(project):
    """Check modified flag of the ``project`` and offer saving via dialog
    """
    if project is None:
        return True

    if show_save_project_message(project):
        save_object(project)

    return True
