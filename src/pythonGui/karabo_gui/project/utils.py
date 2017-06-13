#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import StringIO
from types import MethodType
import weakref

from PyQt4.QtGui import QDialog, QMessageBox

from karabo.common.api import set_modified_flag, walk_traits_object
from karabo.common.project.api import (
    BaseProjectObjectModel, DeviceConfigurationModel, DeviceInstanceModel,
    DeviceServerModel, ProjectModel, device_instance_exists,
    recursive_save_object, read_lazy_object
)
from karabo.common.scenemodel.api import read_scene
from karabo.middlelayer import Hash, read_project_model
from karabo_gui.events import broadcast_event, KaraboEventSender
import karabo_gui.globals as krb_globals
from karabo_gui import messagebox
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
    from karabo_gui.project.dialog.device_handle import DeviceHandleDialog

    server_model = None
    if isinstance(server, str):
        server_model = get_device_server_model(server)
    elif isinstance(server, DeviceServerModel):
        server_model = server

    if server_model is None:
        msg = ('A device server with the ID "<b>{}</b>"<br>'
               'needs to be added to the current project before<br>'
               'a device can be added').format(server)
        messagebox.show_warning(msg, title='Server doesn\'t exist')
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
            description=dialog.description
        )
        # Set initialized and modified last
        config_model.initialized = config_model.modified = True

        device = DeviceInstanceModel(
            class_id=dialog.class_id,
            instance_id=dialog.instance_id,
            if_exists=dialog.if_exists,
            configs=[config_model],
            active_config_ref=config_model.uuid,
        )
        # Set initialized and modified last
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
        messagebox.show_warning(msg, title='Device already exists')
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


def handle_scene_from_server(dev_id, name, project, success, reply):
    """Callback handler for a request to a device to load one of its scenes.
    """
    if not (success and reply.get('payload.success', False)):
        msg = 'Scene "{}" from device "{}" was not retreived!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Scene from Device Failed')
        return

    data = reply.get('payload.data', '')
    if not data:
        msg = 'Scene "{}" from device "{}" contains no data!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Scene from Device Failed')
        return

    with StringIO(data) as fp:
        scene = read_scene(fp)
        scene.modified = True
        scene.simple_name = name
        scene.reset_uuid()

    # Add to the project AND open it
    project.scenes.append(scene)
    broadcast_event(KaraboEventSender.ShowSceneView, {'model': scene})


def load_project(is_subproject=False):
    """Load a project from the project database.

    :param is_subproject States whether a master or a subproject is about to be
                         loaded
    """
    from karabo_gui.project.dialog.project_handle import LoadProjectDialog
    dialog = LoadProjectDialog(is_subproject=is_subproject)
    result = dialog.exec()
    if result == QDialog.Accepted:
        domain, uuid = dialog.selected_item()
        if domain is not None and uuid is not None:
            db_conn = get_db_conn()
            model = ProjectModel(uuid=uuid)
            read_lazy_object(domain, uuid, db_conn, read_project_model,
                             existing=model)
            db_conn.default_domain = domain
            db_conn.flush()
            if not dialog.ignore_cache:
                # Set modified flag recursively to True to make sure that
                # EVERYTHING gets saved to the database in case the project was
                # loaded from cache
                set_modified_flag(model, value=True)
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


def save_as_object(obj):
    """ Save copy of individual `obj` recursively into the project database

    :param obj A project model object
    """
    from karabo_gui.project.dialog.project_handle import NewProjectDialog

    def _visitor(model):
        if isinstance(model, BaseProjectObjectModel):
            model.reset_uuid()

    assert isinstance(obj, ProjectModel)
    dialog = NewProjectDialog(model=obj)
    if dialog.exec() == QDialog.Accepted:
        obj.simple_name = dialog.simple_name
        walk_traits_object(obj, _visitor)
        # Set all child object of the given ``obj`` to modified to actually
        # save the complete tree to the new domain
        set_modified_flag(obj, value=True)
        save_object(obj, dialog.domain)


def save_object(obj, domain=None):
    """ Save individual `obj` recursively into the project database

    :param obj A project model object
    """
    db_conn = get_db_conn()
    if domain is None:
        domain = db_conn.default_domain

    recursive_save_object(obj, db_conn, domain)
    db_conn.default_domain = domain
    db_conn.flush()


def show_save_project_message(project):
    """Check whether the given ``project`` is modified and show a messagebox to
    allow the user to confirm saving or cancel

    :return Whether the user wants to save
    """
    if project is not None and project.modified:
        ask = ('The project \"<b>{}</b>\" has been modified.<br />Do you want '
               'to save the project?').format(project.simple_name)
        options = (QMessageBox.Save | QMessageBox.No)
        reply = QMessageBox.question(None, 'Save project', ask, options,
                                     QMessageBox.Save)
        if reply == QMessageBox.No:
            return False

        if reply == QMessageBox.Save:
            return True
    return False


def show_trash_project_message(is_trashed, simple_name=''):
    """Check whether the given ``is_trashed`` should be toggled and show a
    messagebox to allow the user to confirm moving to trash or restoring from
    trash or cancel

    :return Whether the user wants to change the ``is_trashed`` attribute
    """
    if is_trashed:
        title = 'Restore from trash'
        text = ('Do you really want to <b>restore</b> this project <br><b>{}'
                '</b> from trash?').format(simple_name)
    else:
        title = 'Move to trash'
        text = ('Do you really want to <b>move</b> this project <br><b>{}</b>'
                ' to trash?').format(simple_name)

    result = QMessageBox.question(None, title, text,
                                  QMessageBox.Yes | QMessageBox.No)
    if result == QMessageBox.Yes:
        return True
    return False


def show_no_configuration():
    """Broadcast event to show no configuration in configuration panel
    """
    broadcast_event(KaraboEventSender.ShowConfiguration,
                    {'configuration': None})


def run_macro(macro_model):
    instance_id = macro_model.instance_id
    h = Hash("code", macro_model.code,
             "module", macro_model.simple_name,
             "uuid", macro_model.uuid)
    get_network().onInitDevice(krb_globals.MACRO_SERVER, "MetaMacro",
                               instance_id, h)
