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

from uuid import uuid4

MACRO_BODY = '<macro>ZnJvbSBrYXJhYm8ubWlkZGxlbGF5ZXIgaW1wb3J0IE1hY3Jv</macro>'


def generate_uuid() -> str:
    return str(uuid4())


async def create_device(db):
    """Create a device instance with two configurations."""
    config_uuids = [generate_uuid(), generate_uuid()]

    for config_uuid in config_uuids:
        config_xml = (
            f'<xml uuid="{config_uuid}" simple_name="{config_uuid}" '
            'description="" item_type="device_config" '
            'revision="0" alias="default">'
            '<root KRB_Artificial="">'
            '<PropertyTest KRB_Type="HASH"/>'
            '</root>'
            '</xml>')
        await db.save_item("LOCAL", config_uuid, config_xml)

    device_uuid = generate_uuid()
    instance_id = f"device-karabo-{device_uuid}"
    device_xml = (
        f'<xml item_type="device_instance" uuid="{device_uuid}">'
        '<device_instance active_rev="0" class_id="a_class" '
        f'instance_id="{instance_id}" active_uuid="{config_uuids[0]}">'
        f'<device_config revision="0" uuid="{config_uuids[0]}" />'
        f'<device_config revision="1" uuid="{config_uuids[1]}" />'
        '</device_instance>'
        '</xml>')

    await db.save_item("LOCAL", device_uuid, device_xml)
    return instance_id, device_uuid, config_uuids[0]


async def create_hierarchy(
        db, scene_name: str | None = None) -> tuple[str, dict]:
    """Create a project with scenes, macros, servers, and devices.

    The dict return as the second tuple element contains the
    `instance_id`s as keys and the `uuid` of the `revision:="0"`
    configurations for each of the 4x4 devices created.

    In total:
        - 1 Project
        - 4 x Macro
        - 4 x Scene
        - 4 x Device Server with 4 devices each

    25 objects are created in the database.

    """
    project_uuid = generate_uuid()
    project_xml = (
        f'<xml item_type="project" uuid="{project_uuid}" '
        'simple_name="Project"><root><project><scenes>')
    # Scenes
    for _ in range(4):
        scene_uuid = generate_uuid()
        name = scene_name or scene_uuid

        project_xml += (
            f'<xml item_type="scene" uuid="{scene_uuid}" '
            f'simple_name="{name}" />')

        scene_xml = (
            f'<xml item_type="scene" uuid="{scene_uuid}" simple_name="{name}">'
            '<svg:svg xmlns:svg="http://www.w3.org/2000/svg" '
            'xmlns:krb="http://karabo.eu/scene" height="768" width="1024" '
            f'krb:uuid="{scene_uuid}" krb:version="2">'
            '</svg:svg></xml>')
        await db.save_item("LOCAL", scene_uuid, scene_xml)

    project_xml += '</scenes>'

    # Macros
    project_xml += '<macros>'
    for i in range(4):
        macro_uuid = generate_uuid()
        project_xml += f'<KRB_Item><uuid>{macro_uuid}</uuid></KRB_Item>'

        macro_name = f"macroname-{i}"
        macro_xml = (
            f'<xml uuid="{macro_uuid}" simple_name="{macro_name}" '
            f'description="" item_type="macro">{MACRO_BODY}</xml>')
        await db.save_item("LOCAL", macro_uuid, macro_xml)

    project_xml += '</macros>'

    # Device Servers
    project_xml += '<servers>'
    device_config_map = {}
    for _ in range(4):
        server_uuid = generate_uuid()
        project_xml += f'<KRB_Item><uuid>{server_uuid}</uuid></KRB_Item>'

        devices_xml = ""
        for _ in range(4):
            deviceId, dev_uuid, config_uuid = await create_device(db)
            devices_xml += f'<device_instance uuid="{dev_uuid}" />'
            device_config_map[deviceId] = config_uuid

        server_xml = (
            f'<xml item_type="device_server" uuid="{server_uuid}" '
            f'simple_name="{server_uuid}">'
            f'<device_server>{devices_xml}</device_server></xml>'
        )

        await db.save_item("LOCAL", server_uuid, server_xml)

    project_xml += '</servers></project></root></xml>'
    await db.save_item("LOCAL", project_uuid, project_xml)

    return project_uuid, device_config_map


async def create_trashed_project(db, is_trashed=True):
    """Create a trashed project entry."""
    project_uuid = generate_uuid()
    trashed_str = str(is_trashed).lower()
    xml = (
        f'<xml item_type="project" uuid="{project_uuid}" '
        f'simple_name="{project_uuid}" is_trashed="{trashed_str}"></xml>')

    await db.save_item("LOCAL", project_uuid, xml)
    return project_uuid


async def create_unattached_scenes(db):
    """Create scenes unattached to project, sharing the same simple_name."""
    for _ in range(4):
        scene_uuid = generate_uuid()
        scene_xml = (
            f'<xml item_type="scene" uuid="{scene_uuid}" '
            'simple_name="Scene!" >中文</xml>')
        await db.save_item("LOCAL", scene_uuid, scene_xml)
