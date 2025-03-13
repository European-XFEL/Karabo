# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

# XML emitters for Models

import datetime

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, Scene)


def emit_project_xml(project: Project,
                     scenes: list[Scene],
                     macros: list[Macro],
                     servers: list[DeviceServer],
                     subprojects: list[Project]) -> str:
    def _emit_project_macros() -> str:
        xml = '<macros KRB_Type="VECTOR_HASH">'
        for macro in macros:
            xml += (
                '<KRB_Item>'
                f'<uuid KRB_Type="STRING">{macro.uuid}</uuid>'
                '<revision KRB_Type="INT32">0</revision>'
                '</KRB_Item>')
        xml += '</macros>'
        return xml

    def _emit_project_scenes() -> str:
        xml = '<scenes KRB_Type="VECTOR_HASH">'
        for scene in scenes:
            xml += (
                '<KRB_Item>'
                f'<uuid KRB_Type="STRING">{scene.uuid}</uuid>'
                '<revision KRB_Type="INT32">0</revision>'
                '</KRB_Item>')
        xml += '</scenes>'
        return xml

    def _emit_project_servers() -> str:
        xml = '<servers KRB_Type="VECTOR_HASH">'
        for server in servers:
            xml += (
                '<KRB_Item>'
                f'<uuid KRB_Type="STRING">{server.uuid}</uuid>'
                '<revision KRB_Type="INT32">0</revision>'
                '</KRB_Item>')
        xml += '</servers>'
        return xml

    def _emit_project_subprojects() -> str:
        xml = '<subprojects KRB_Type="VECTOR_HASH">'
        for subproject in subprojects:
            xml += (
                '<KRB_Item>'
                f'<uuid KRB_Type="STRING">{subproject.uuid}</uuid>'
                '<revision KRB_Type="INT32">0</revision>'
                '</KRB_Item>')
        xml += '</subprojects>'
        return xml

    xml = (
        '<xml xmlns:exist="http://exist.sourceforge.net/NS/exist" '
        f'uuid="{project.uuid}" simple_name="{project.name}" '
        f'description="{project.description}" '
        f'date="{(
            project.date.replace(tzinfo=datetime.UTC)
            if project.date is not None else '')}" '
        # NOTE: The GUI Client compares the is_trashed value
        #       with the constants 'true' and 'false' (all lower)
        f'is_trashed="{'true' if project.is_trashed else 'false'}" '
        f'item_type="project" user="{project.last_modified_user}" '
        'revision="0" alias="default">'
        '<root KRB_Artificial="">'
        '<project KRB_Type="HASH">'
        f'  {_emit_project_macros()} '
        f'  {_emit_project_scenes()} '
        f'  {_emit_project_servers()} '
        f'  {_emit_project_subprojects()} '
        '</project>'
        '</root>'
        '</xml>')

    return xml


def emit_scene_xml(scene: Scene) -> str:
    xml = (
        '<xml xmlns:exist="http://exist.sourceforge.net/NS/exist" '
        f'uuid="{scene.uuid}" simple_name="{scene.name}" '
        f'date="{(scene.date.replace(tzinfo=datetime.UTC)
                  if scene.date is not None else '')}" item_type="scene" '
        f'user="{scene.last_modified_user}" revision="0" alias="default">'
        f'{scene.svg_data}'
        '</xml>')

    return xml


def emit_macro_xml(macro: Macro) -> str:
    import base64
    xml = (
        '<xml xmlns:exist="http://exist.sourceforge.net/NS/exist" '
        f'uuid="{macro.uuid}" simple_name="{macro.name}" description="" '
        f'date="{(macro.date.replace(tzinfo=datetime.UTC)
                  if macro.date is not None else '')}" item_type="macro" '
        f'user="{macro.last_modified_user}" revision="0" alias="default">'
        '  <macro>'
        f'{base64.b64encode(macro.body.encode('utf-8')).decode()}'
        '  </macro>'
        '</xml>')

    return xml


def emit_device_server_xml(server: DeviceServer,
                           device_instances: list[DeviceInstance]) -> str:
    xml = (
        '<xml xmlns:exist="http://exist.sourceforge.net/NS/exist" '
        f'uuid="{server.uuid}" simple_name="{server.name}" '
        'item_type="device_server" '
        f'date="{(server.date.replace(tzinfo=datetime.UTC)
                  if server.date is not None else '')}" '
        f'user="{server.last_modified_user}" revision="0" alias="default">'
        f'<device_server server_id="{server.name}" host="">')
    for device_instance in device_instances:
        xml += (
            f'<device_instance uuid="{device_instance.uuid}" '
            'revision="0"/>')
    xml += (
        '</device_server>'
        '</xml>')

    return xml


def emit_device_instance_xml(instance: DeviceInstance,
                             configs: list[DeviceConfig]) -> str:
    active_uuid = configs[0].uuid if len(configs) > 0 else ''
    for config in configs:
        if config.is_active:
            active_uuid = config.uuid

    xml = (
        '<xml xmlns:exist="http://exist.sourceforge.net/NS/exist" '
        f'uuid="{instance.uuid}" simple_name="{instance.name}" '
        'description="" item_type="device_instance" '
        f'date="{(instance.date.replace(tzinfo=datetime.UTC)
                  if instance.date is not None else '')}" '
        f'user="{instance.last_modified_user}"'
        ' revision="0" alias="default">')
    xml += (
        f'<device_instance class_id="{instance.class_id}" '
        f'instance_id="{instance.name}" '
        f'active_uuid="{active_uuid}" active_rev="0">')
    for config in configs:
        xml += (
            f'<device_config class_id="{instance.class_id}" '
            f'uuid="{config.uuid}" revision="0"/>')
    xml += '</device_instance></xml>'

    return xml


def emit_device_config_xml(config: DeviceConfig) -> str:
    xml = (
        '<xml xmlns:exist="http://exist.sourceforge.net/NS/exist" '
        f'uuid="{config.uuid}" simple_name="{config.name}" '
        f'description="{config.description}" '
        f'date="{(config.date.replace(tzinfo=datetime.UTC)
                  if config.date is not None else '')}" '
        'item_type="device_config" '
        f'user="{config.last_modified_user}" revision="0" alias="default">'
        f'{config.config_data}'
        '</xml>')

    return xml
