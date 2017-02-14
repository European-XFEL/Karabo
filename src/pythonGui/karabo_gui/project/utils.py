#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from types import MethodType
import weakref

from PyQt4.QtGui import QDialog, QMessageBox

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    ProjectModel, device_instance_exists, recursive_save_object,
    read_lazy_object, walk_traits_object
)
from karabo.middlelayer import Hash, read_project_model
from karabo_gui.events import (broadcast_event, KaraboBroadcastEvent,
                               KaraboEventSender)
import karabo_gui.globals as krb_globals 
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.project_handle import LoadProjectDialog
from karabo_gui.singletons.api import (get_db_conn, get_project_model,
                                       get_network)


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


def add_device_to_server(server, class_id=''):
    """ Add a new device to a server

    :param server: A DeviceServerModel object, OR string containing a server id
    :param class_id: An optional parameter containing the class_id to use for
                     the newly created device.
    :return: The device id of the newly added device, or an empty string if
             no device was added.
    """
    server_model = None
    if isinstance(server, str):
        server_model = get_device_server_model(server)
    elif isinstance(server, DeviceServerModel):
        server_model = server

    if server_model is None:
        msg = ('A device server with the ID "<b>{}</b>"<br>'
               'needs to be added to the current project before<br>'
               'a device can be added').format(server)
        QMessageBox.warning(None, 'Server doesn\'t exist', msg)
        return ''

    dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                class_id=class_id)
    result = dialog.exec()
    if result == QDialog.Accepted:
        # Check for existing device
        if check_device_instance_exists(dialog.instance_id):
            return ''

        config_model = DeviceConfigurationModel(
            class_id=dialog.class_id, configuration=Hash(),
            simple_name=dialog.configuration_name,
            alias=dialog.configuration_name,
            description=dialog.description
        )
        # Set initialized and modified last to avoid bumping revision
        config_model.initialized = config_model.modified = True

        device = DeviceInstanceModel(
            class_id=dialog.class_id,
            instance_id=dialog.instance_id,
            if_exists=dialog.if_exists,
            configs=[config_model],
            active_config_ref=(config_model.uuid, config_model.revision),
        )
        # Set initialized and modified last to avoid bumping revision
        device.initialized = device.modified = True

        server_model.devices.append(device)
        return device.instance_id

    # No device was added!
    return ''


def check_device_instance_exists(instance_id):
    """Check whether the incoming ``instance_id`` already exists in the current
    projects and return ``True`` if that is the case else ``False``.
    """
    root_project = get_project_model().traits_data_model
    if device_instance_exists(root_project, instance_id):
        msg = ('Another device with the same device ID \"<b>{}</b>\" '
               '<br>already exists! Therefore it will not be '
               'added!').format(instance_id)
        QMessageBox.warning(None, 'Device already exists', msg)
        return True
    return False


def get_device_server_model(server_id):
    """Given a string containing a server id, return the corresponding
    ``DeviceServerModel`` object in the current project, or None if one does
    not exist.
    """
    server_model = None

    def visitor(model):
        nonlocal server_model
        if (isinstance(model, DeviceServerModel) and
                model.server_id == server_id):
            server_model = model

    root_project = get_project_model().traits_data_model
    walk_traits_object(root_project, visitor)
    return server_model


def load_project(domain):
    """Load a project from the project database.
    """
    dialog = LoadProjectDialog()
    result = dialog.exec()
    if result == QDialog.Accepted:
        uuid, revision = dialog.selected_item()
        if uuid is not None and revision is not None:
            db_conn = get_db_conn()
            model = ProjectModel(uuid=uuid, revision=revision)
            read_lazy_object(domain, uuid, revision, db_conn,
                             read_project_model, existing=model)
            db_conn.flush()
            return model
    return None


def maybe_save_modified_project(project):
    """Check modified flag of the ``project`` and offer saving via dialog
    """
    if project is None:
        return True

    if show_save_project_message(project):
        save_object(project)

    return True


def save_object(obj):
    """ Save individual `obj` in project database

    :param obj A project model object
    """
    from karabo_gui.project.api import TEST_DOMAIN

    ask = 'Do you really want to save \"<b>{}</b>\"?'.format(obj.simple_name)
    reply = QMessageBox.question(None, 'Save object', ask,
                                 QMessageBox.Yes | QMessageBox.No,
                                 QMessageBox.No)
    if reply == QMessageBox.No:
        return

    db_conn = get_db_conn()
    recursive_save_object(obj, db_conn, TEST_DOMAIN)
    db_conn.flush()


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


def show_no_configuration():
    """Broadcast event to show no configuration in configuration panel
    """
    data = {'configuration': None}
    broadcast_event(KaraboBroadcastEvent(
        KaraboEventSender.ShowConfiguration, data))


def run_macro(macro_model):
    instance_id = macro_model.instance_id
    h = Hash("code", macro_model.code,
             "module", macro_model.simple_name,
             "uuid", macro_model.uuid)
    get_network().onInitDevice(krb_globals.MACRO_SERVER, "MetaMacro",
                               instance_id, h)
