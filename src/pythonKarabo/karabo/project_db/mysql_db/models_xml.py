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

import base64
import datetime

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, Scene)

_XML_NS = 'xmlns:exist="http://exist.sourceforge.net/NS/exist"'
_REVISION_ALIAS = 'revision="0" alias="default"'
_ITEM_TEMPLATE = ('<KRB_Item><uuid KRB_Type="STRING">{uuid}</uuid>'
                  '<revision KRB_Type="INT32">0</revision></KRB_Item>')


def _format_date(d):
    return d.replace(tzinfo=datetime.UTC) if d else ''


def _wrap_vector_hash(tag, items):
    return f'<{tag} KRB_Type="VECTOR_HASH">{"".join(items)}</{tag}>'


def _wrap_xml(uuid, name, item_type, user, date='', description='', extra=''):
    return (
        f'<xml {_XML_NS} uuid="{uuid}" simple_name="{name}" '
        f'description="{description}" date="{date}" '
        f'item_type="{item_type}" user="{user}" '
        f'{_REVISION_ALIAS}>{extra}</xml>'
    )


def emit_project_xml(project: Project,
                     scenes: list[Scene],
                     macros: list[Macro],
                     servers: list[DeviceServer],
                     subprojects: list[Project]) -> str:
    def _emit_refs(objects):
        return [_ITEM_TEMPLATE.format(uuid=o.uuid) for o in objects]

    date = _format_date(project.date)
    trashed = 'true' if project.is_trashed else 'false'

    content = (
        '<root KRB_Artificial="">'
        '<project KRB_Type="HASH">'
        f'{_wrap_vector_hash("macros", _emit_refs(macros))}'
        f'{_wrap_vector_hash("scenes", _emit_refs(scenes))}'
        f'{_wrap_vector_hash("servers", _emit_refs(servers))}'
        f'{_wrap_vector_hash("subprojects", _emit_refs(subprojects))}'
        '</project>'
        '</root>'
    )

    return (
        f'<xml {_XML_NS} uuid="{project.uuid}" simple_name="{project.name}" '
        f'description="{project.description}" date="{date}" '
        f'is_trashed="{trashed}" item_type="project" '
        f'user="{project.last_modified_user}" {_REVISION_ALIAS}>'
        f'{content}'
        '</xml>'
    )


def emit_scene_xml(scene: Scene) -> str:
    return _wrap_xml(
        uuid=scene.uuid,
        name=scene.name,
        item_type="scene",
        user=scene.last_modified_user,
        date=_format_date(scene.date),
        extra=scene.svg_data
    )


def emit_macro_xml(macro: Macro) -> str:
    encoded = base64.b64encode(macro.body.encode('utf-8')).decode()
    macro_body = f'<macro>{encoded}</macro>'
    return _wrap_xml(
        uuid=macro.uuid,
        name=macro.name,
        item_type="macro",
        user=macro.last_modified_user,
        date=_format_date(macro.date),
        extra=macro_body
    )


def emit_device_server_xml(server: DeviceServer,
                           device_instances: list[DeviceInstance]) -> str:
    di_tags = ''.join(
        f'<device_instance uuid="{di.uuid}" revision="0"/>'
        for di in device_instances
    )
    content = (f'<device_server server_id="{server.name}" '
               f'host="">{di_tags}</device_server>')
    return _wrap_xml(
        uuid=server.uuid,
        name=server.name,
        item_type="device_server",
        user=server.last_modified_user,
        date=_format_date(server.date),
        extra=content
    )


def emit_device_instance_xml(instance: DeviceInstance,
                             configs: list[DeviceConfig]) -> str:
    fallback_uuid = configs[0].uuid if configs else ''
    active_uuid = next((c.uuid for c in configs if c.is_active),
                       fallback_uuid)

    config_tags = ''.join(
        f'<device_config class_id="{instance.class_id}" '
        f'uuid="{c.uuid}" revision="0"/>'
        for c in configs
    )
    instance_block = (
        f'<device_instance class_id="{instance.class_id}" '
        f'instance_id="{instance.name}" '
        f'active_uuid="{active_uuid}" active_rev="0">'
        f'{config_tags}</device_instance>'
    )
    return _wrap_xml(
        uuid=instance.uuid,
        name=instance.name,
        item_type="device_instance",
        user=instance.last_modified_user,
        date=_format_date(instance.date),
        extra=instance_block
    )


def emit_device_config_xml(config: DeviceConfig) -> str:
    return _wrap_xml(
        uuid=config.uuid,
        name=config.name,
        item_type="device_config",
        user=config.last_modified_user,
        date=_format_date(config.date),
        description=config.description,
        extra=config.config_data
    )
