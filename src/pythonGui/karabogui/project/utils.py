#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
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
from qtpy.QtWidgets import QDialog, QMessageBox

from karabo.common.api import (
    KARABO_PROJECT_MANAGER, set_modified_flag, walk_traits_object)
from karabo.common.project.api import (
    BaseProjectObjectModel, DeviceConfigurationModel, DeviceInstanceModel,
    DeviceServerModel, MacroModel, ProjectModel, device_config_exists,
    device_instance_exists, device_server_exists, macro_exists,
    read_lazy_object, recursive_save_object)
from karabo.common.sanity_check import validate_macro
from karabo.common.scenemodel.api import (
    BaseWidgetObjectData, SceneLinkModel, SceneModel)
from karabo.native import Hash, read_project_model
from karabogui import messagebox
from karabogui.access import AccessRole, access_role_allowed
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.request import onShutdown
from karabogui.singletons.api import (
    get_config, get_db_conn, get_network, get_project_model, get_topology)
from karabogui.topology.util import get_macro_servers
from karabogui.util import (
    create_list_string, move_to_cursor, version_compatible)


def add_device_to_server(server, class_id='', parent=None):
    """ Add a new device to a server

    :param server: A DeviceServerModel object, OR string containing a server id
    :param class_id: An optional parameter containing the class_id to use for
                     the newly created device.
    :return: The device id of the newly added device, or an empty string if
             no device was added.
    """
    from karabogui.project.dialog.device_handle import DeviceHandleDialog

    server_model = None
    if isinstance(server, str):
        server_model = get_device_server_model(server)
    elif isinstance(server, DeviceServerModel):
        server_model = server

    if server_model is None:
        msg = ('A device server with the ID "<b>{}</b>"<br>'
               'needs to be added to the current project before<br>'
               'a device can be added').format(server)
        messagebox.show_warning(msg, title='Server doesn\'t exist',
                                parent=parent)
        return ''

    dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                class_id=class_id,
                                parent=parent)
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
    """Check whether ``instance_id`` already exists in the current projects

    Shows a messagebox if the device already exists in the project!

    :returns: ``True`` if ``instance_id`` was NOT found else ``False``.
    """
    root_project = get_project_model().root_model
    if device_instance_exists(root_project, instance_id):
        msg = ('Another device with the same device ID \"<b>{}</b>\" '
               '<br>already exists! Therefore it will not be '
               'added!').format(instance_id)
        messagebox.show_warning(msg, title='Device already exists')
        return True
    return False


def validate_device_instance_exists(instance_id):
    """Validate whether ``instance_id`` was found in the root project

        Shows a messagebox if the device was not found in the project!

        :returns: ``True`` if ``instance_id`` was found else ``False``.
    """
    root_project = get_project_model().root_model
    if not device_instance_exists(root_project, instance_id):
        msg = ('The device with the device ID \"<b>{}</b>\" '
               '<br> was not found in the project!').format(instance_id)
        messagebox.show_warning(msg, title='Device does not exist')
        return False
    return True


def check_device_config_exists(instance_id, config_name):
    """Check whether the configuration already exists for the device

    :param instance_id: deviceId to be searched for
    :param config_name: device configuration to be searched for

    :return ``True`` if config name is found else ``False``.
    """
    root_project = get_project_model().root_model
    if device_config_exists(root_project, instance_id, config_name):
        msg = ('Another device configuration with the same name \"<b>{}</b>\" '
               '<br>already exists! Therefore it will not be '
               'added!').format(config_name)
        messagebox.show_warning(msg,
                                title='Device configuration already exists')
        return True
    return False


def check_device_server_exists(instance_id):
    """Check whether the device server already exists

    :param instance_id: serverId to be searched for

    :return ``True`` if server name is found else ``False``.
    """
    root_project = get_project_model().root_model
    if device_server_exists(root_project, instance_id):
        msg = ('The server with the server ID \"<b>{}</b>\" '
               '<br> already exists. Therefore it will not be '
               'added!').format(instance_id)
        messagebox.show_warning(msg, title='Server already exists')
        return True
    return False


def check_macro_exists(instance_id):
    """Check whether the macro name already exists

    :param instance_id: serverId to be searched for

    :return ``True`` if server name is found else ``False``.
    """
    root_project = get_project_model().root_model
    if macro_exists(root_project, instance_id):
        msg = ('The macro with the name \"<b>{}</b>\" '
               '<br> already exists. Therefore it will not be '
               'added!').format(instance_id)
        messagebox.show_warning(msg, title='Macro already exists')
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
            return True

    root_project = get_project_model().root_model
    walk_traits_object(root_project, visitor, fast_exit=True)
    return server_model


def load_project(is_subproject=False, parent=None):
    """Load a project from the project database.

    :param is_subproject States whether a master or a subproject is about to be
                         loaded
    """
    from karabogui.project.dialog.project_handle import LoadProjectDialog
    dialog = LoadProjectDialog(is_subproject=is_subproject, parent=parent)
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
            if not is_subproject:
                broadcast_event(KaraboEvent.ProjectName,
                                {"simple_name": dialog.simple_name})
            return model
    return None


def load_project_with_device(parent=None):
    """Load a project with a device specified by the user from the project
    database.
    """
    from karabogui.project.dialog.load_project_with_device import (
        LoadProjectWithDeviceDialog)
    device_domain = get_config()["device_domain"]
    dialog = LoadProjectWithDeviceDialog(
        initial_domain=device_domain, parent=parent)
    result = dialog.exec()
    if result == QDialog.Accepted:
        domain = dialog.selected_domain()
        uuid = dialog.selected_project_id()
        get_config()["device_domain"] = domain
        if domain is not None and uuid is not None:
            db_conn = get_db_conn()
            model = ProjectModel(uuid=uuid)
            read_lazy_object(domain, uuid, db_conn, read_project_model,
                             existing=model)
            db_conn.default_domain = domain
            db_conn.flush()
            # Project name will be updated
            broadcast_event(KaraboEvent.ProjectName,
                            {"simple_name": dialog.simple_name})
            return model
    return None


def reload_project(model):
    """Reload a project from the project database."""
    uuid = model.uuid
    model = ProjectModel(uuid=uuid)
    # NOTE: We keep the default domain of the db_conn here!
    db_conn = get_db_conn()
    domain = db_conn.default_domain
    read_lazy_object(domain, uuid, db_conn, read_project_model,
                     existing=model)
    # NOTE: Cache objects are loaded without modified flag!
    db_conn.flush()

    return model


def maybe_save_modified_project(project, parent=None):
    """Check modified flag of the ``project`` and offer saving via dialog
    """
    if project is None:
        return True

    if show_save_project_message(project, parent):
        save_object(project)

    return True


def save_as_object(obj):
    """ Save copy of individual `obj` recursively into the project database

    :param obj A project model object
    """
    from karabogui.project.dialog.project_handle import NewProjectDialog

    # Map old scene UUIDs to new UUIDs
    scene_uuids = {}
    # Map old macro UUIDs to new UUIDs
    macro_uuids = {}
    # Store all scene link references
    scene_links = []
    # Store all models that link to macros
    macro_models = []

    def _visitor(model):
        if isinstance(model, BaseProjectObjectModel):
            old_uuid = model.uuid
            model.reset_uuid()
            if isinstance(model, SceneModel):
                # Keep track of new UUIDs
                scene_uuids[old_uuid] = model.uuid
            if isinstance(model, MacroModel):
                # Keep track of new UUIDs
                macro_uuids[old_uuid] = model.uuid
        elif isinstance(model, SceneLinkModel):
            scene_links.append(model)
        elif isinstance(model, BaseWidgetObjectData):
            for key in model.keys:
                # optimize out non macros
                if key.startswith("Macro-"):
                    # key will be => Macro-module-UUID-Classname.keyname
                    uuid = '-'.join(key.split('-')[2:-1])
                    macro_models.append((model, uuid))

    def _replace_scene_link_uuids():
        for link in scene_links:
            parts = link.target.split(':')
            if len(parts) != 2:
                break
            # target format => "simple_name:UUID"
            simple_name = parts[0]
            old_uuid = parts[1]
            if old_uuid in scene_uuids:
                new_uuid = scene_uuids[old_uuid]
                target = f"{simple_name}:{new_uuid}"
            else:
                target = f"{simple_name}:{old_uuid}"
            link.target = target

    def _replace_macro_uuids():
        for widget, old_uuid in macro_models:
            new_keys = []
            for key in widget.keys:
                if old_uuid in key:
                    # replace old uuid with new uuid
                    new_uuid = macro_uuids[old_uuid]
                    key = key.replace(old_uuid, new_uuid)
                new_keys.append(key)
            widget.keys = new_keys

    assert isinstance(obj, ProjectModel)
    dialog = NewProjectDialog(model=obj)
    if dialog.exec() == QDialog.Accepted:
        if obj.simple_name == dialog.simple_name:
            # Throw a tantrum
            messagebox.show_error(
                "Please choose a different name. "
                "Projects with the same name should be avoided.")
            return
        obj.simple_name = dialog.simple_name
        # Reset UUIDs and map scene and macro models
        walk_traits_object(obj, _visitor)
        # Replace old scene link UUIDs with new ones
        _replace_scene_link_uuids()
        # Replace old macro UUIDs keys with new ones
        _replace_macro_uuids()
        # Set all child object of the given ``obj`` to modified to actually
        # save the complete tree to the new domain
        set_modified_flag(obj, value=True)
        save_object(obj, dialog.domain)


def save_object(obj, domain=None):
    """ Save individual `obj` recursively into the project database

    :param obj A project model object
    """
    if not _is_karabo3():
        dialog = QMessageBox(QMessageBox.Critical, "Error",
                             "Can not Save project to Karabo2")
        move_to_cursor(dialog)
        dialog.exec()
        return
    db_conn = get_db_conn()
    if domain is None:
        domain = db_conn.default_domain

    recursive_save_object(obj, db_conn, domain)
    db_conn.default_domain = domain
    db_conn.flush()


def show_save_project_message(project, parent=None):
    """Check whether the given ``project`` is modified and show a messagebox to
    allow the user to confirm saving or cancel

    :return Whether the user wants to save
    """

    if (project is not None and project.modified
            and access_role_allowed(AccessRole.PROJECT_EDIT)):
        ask = ('The project \"<b>{}</b>\" has been modified.<br />Do you want '
               'to save the project?').format(project.simple_name)
        options = (QMessageBox.Save | QMessageBox.No)
        reply = QMessageBox.question(parent, 'Save project', ask, options,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return False

        if reply == QMessageBox.Save:
            return True
    return False


def show_modified_project_message(project, parent=None):
    """Check whether the given ``project`` is modified and show a messagebox to
    allow the user to confirm saving or cancel

    :return Whether the user wants to save
    """
    if project is None:
        return False

    if project.modified:
        ask = ('The project \"<b>{}</b>\" has been modified.<br />Are you '
               'sure you want to continue? <b>Your project changes will be '
               'lost</b>!').format(project.simple_name)
        options = (QMessageBox.Yes | QMessageBox.No)
        reply = QMessageBox.question(parent, 'Modified project', ask, options,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return False

        if reply == QMessageBox.Yes:
            return True
    return True


def show_reload_scene_project_message(data, parent=None):
    """Check whether the given ``project`` has opened scenes

    :return Whether the user wants to restore scenes
    """
    if not data:
        return False

    ask = (f'The project currently has <b>{len(data)}</b> active scene(s). '
           'Do you want to open the scenes after project has been reloaded?')
    options = (QMessageBox.Yes | QMessageBox.No)
    reply = QMessageBox.question(parent, 'Scenes reload', ask,
                                 options, QMessageBox.No)
    if reply == QMessageBox.No:
        return False

    if reply == QMessageBox.Yes:
        return True


def show_trash_project_message(is_trashed, simple_name='', parent=None):
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

    result = QMessageBox.question(parent, title, text,
                                  QMessageBox.Yes | QMessageBox.No)
    if result == QMessageBox.Yes:
        return True
    return False


def show_no_configuration():
    """Broadcast event to show no configuration in configuration panel
    """
    broadcast_event(KaraboEvent.ShowConfiguration, {'proxy': None})


def run_macro(macro_model, parent=None):
    """
    Request to run a macro by validating and sending an instantiate.

    :param macro_model:  Model of the Macro.
    :type macro_model: karabo.common.project.macro.MacroModel
    :param parent: Parent for the QMessageBox.
    :type parent: QObject
    """
    macro_servers = get_macro_servers()
    if not macro_servers:
        messagebox.show_error("No (stable) Macro server found in system "
                              "topology. Macro cannot be started.")
        return

    if macro_model.modified:
        options = (QMessageBox.Yes | QMessageBox.No)
        title = "Run Unsaved Macro?"
        text = (
            "The Macro is not saved in the Project. Do you really want to "
            "run the Macro?")
        reply = QMessageBox.question(parent, title, text, options,
                                     QMessageBox.Yes)
        if reply == QMessageBox.No:
            return

    serverId = macro_servers[0]
    _run_macro(macro_model, serverId, parent)


def restart_macro(instance_id, handler, parent=None):
    """Restart macro after confirmation that macro should be restarted.

    Function will do nothing if user declined marco restart
    NOTE: macro should be instantiated"""
    ask = (f"The macro <b>{instance_id}</b> is already online. "
           "Do you want to restart the macro?")
    options = QMessageBox.Yes | QMessageBox.No
    reply = QMessageBox.question(
        parent, "Macro is already online", ask, options, QMessageBox.No)
    if reply != QMessageBox.Yes:
        return
    proxy = get_topology().get_device(instance_id, request=False)
    onShutdown(proxy, handler)


def run_macro_debug(macro_model, serverId, parent=None):
    """Request to run a macro by validating and sending an instantiate.

    This method doesn't ask for modification of the macro

    :param macro_model: Model of the Macro.
    :type macro_model: karabo.common.project.macro.MacroModel
    :param serverId: serverId of a development macro server.
    :type serverId: Str
    :param parent: Parent for the QMessageBox.
    :type parent: QObject
    """
    _run_macro(macro_model, serverId, parent)


def _run_macro(macro_model, serverId, parent):
    """Run a macro implementation to instantiate a macro"""
    report = validate_macro(macro_model.code)
    if report:
        fails = create_list_string(report)
        html = (f"Please check the macro validation report and rename "
                f"function names or remove forbidden imports:{fails}")
        messagebox.show_error(html, title="The Macro cannot be started",
                              parent=parent)
        return

    instance_id = macro_model.instance_id
    h = Hash("code", macro_model.code,
             "module", macro_model.simple_name,
             "uuid", macro_model.uuid)

    # Backward compatibility, only servers with `MetaMacro` class of 2.16.X
    # have the project property
    attrs = get_topology().get_attributes(f"server.{serverId}")
    if attrs is None:
        messagebox.show_error(
            f"No Macro server found with serverId '{serverId}' in system "
            f"topology. Macro cannot be started.", parent=parent)
        return
    version = attrs.get("karaboVersion", "0.0.0")
    if version_compatible(version, 2, 16):
        h["project"] = macro_model.project_name
    get_network().onInitDevice(serverId, "MetaMacro", instance_id, h)


def _is_karabo3() -> bool:
    path = f"device.{KARABO_PROJECT_MANAGER}"
    attributes = get_topology().get_attributes(path)
    is_karabo3 = False
    if attributes is not None:
        version = attributes.get("karaboVersion", "0.0.0")
        is_karabo3 = version_compatible(version, 3, 0)
    return is_karabo3
