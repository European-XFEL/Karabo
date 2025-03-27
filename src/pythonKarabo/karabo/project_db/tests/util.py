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


def _gen_uuid():
    return str(uuid4())


def create_device(db):
    sub_uuid = _gen_uuid()
    conf_uuid1 = _gen_uuid()
    conf_uuid2 = _gen_uuid()
    # The MySQL back-end requires a config referenced by an instance to exist
    # in the database. So we add the configs before adding the instances that
    # refer to them.
    for conf_uuid in [conf_uuid1, conf_uuid2]:
        xml = (f'<xml uuid="{conf_uuid}" simple_name="{conf_uuid}" '
               'description="" item_type="device_config" revision="0" '
               'alias="default">'
               '<root KRB_Artificial="">'
               '    <PropertyTestMDL KRB_Type="HASH"/>'
               '</root>'
               '</xml>')
        db.save_item("LOCAL", conf_uuid, xml)

    # The MySQL back-end requires device instances to have a 'class_id' and an
    # 'active_uuid' attribute
    xml = ('<xml item_type="{atype}" uuid="{uuid}">'
           '<device_instance active_rev="0" class_id="a_class" '
           'instance_id="{instance_id}" active_uuid="{conf_uuid1}">'
           '<device_config revision="0" uuid="{conf_uuid1}" />'
           '<device_config revision="1" uuid="{conf_uuid2}" />'
           '</device_instance>'
           '</xml>').format(uuid=sub_uuid,
                            atype='device_instance',
                            instance_id=sub_uuid,
                            conf_uuid1=conf_uuid1,
                            conf_uuid2=conf_uuid2
                            )

    db.save_item("LOCAL", sub_uuid, xml)
    return sub_uuid, conf_uuid1


def create_hierarchy(db, scene_name=None):
    """
    Create a minimal project hierarchy representative of Karabo entries in
    ExistDB

    :param db: a `ProjectDatabase` instance
    :param scene_name: if defined, the name to be used for the project scenes
    :return: a tuple consisting of:
      - the uuid of the project created
      - a dictionary mapping device ids (key) to their active
         configuration's uuid (value)

    The following entities are created in hierarchical order, with attributes
    mentioned in parentheses. The simple_name is frequently set to the uuid,
    and uuids are randomly generated at each call::

        + Project(item_type:='project', uuid, simple_name:='Project')
        |
        +- 4x Scene(item_type:='scene', uuid, simple_name:=uuid|scene_name)
        |
        +- 4x Server(item_type:='device_server', uuid, simple_name:=uuid)
           |
           +- 4x Device(item_type:='device_instance', uuid)
              |
              2x Device Config(revision=0|1, uuid)

    The Project XML has the format::

        <xml item_type="Project", uuid="...", simple_name="Project">
            <root>
                <project>
                    <scenes>
                        ...
                    </scenes>
                    <servers>
                        ...
                    </servers>
                </project>
            </root>
        </xml>

    Each scene in the project XML has a minimal XML of the form::

        <xml item_type="scene", uuid="...", simple_name="..." />

    For each scene a top-level entry is created::

        <xml item_type="scene" uuid="..."' simple_name="..." >
            中文
        </xml>'

    Each server entry in the project XML has a minimal XML of the form::

        <KRB_Item>
            <uuid>uuid</uuid>
        </KRB_Item>

    For each server a top-level entry is created::

        <xml item_type="device_server" uuid="..."' simple_name="..." >
            <device_server>
                  <device_instance uuid="..." />
            </device_server>
        </xml>

    Here the uuid in each device_instance refer to device XMLs of the form::

        <xml item_type="device_instance uuid="...">
            <device_instance active_rev="0" instance_id="...">
                <device_config revision="0" uuid="..." />
                <device_config revision="1" uuid="..." />
            </device_instance>
        </xml>

    The mapping dict return as the second tuple element contains the
    `instance_id`s as keys and the `uuid` of the `revision:="0"`
    configurations for each of the 4x4 devices created.

    In total thus 1 Project + 4 x Scene + 4 x Device Server x 4 x Device = 21
    objects are created in the database.

    """
    uuid = _gen_uuid()
    xml = ('<xml item_type="{atype}" uuid="{uuid}" '
           'simple_name="{name}">').format(uuid=uuid, atype='project',
                                           name='Project')
    xml += "<root>"
    xml += "<project>"
    xml += "<scenes>"

    # create some scenes
    for i in range(4):
        sub_uuid = _gen_uuid()
        scene_simple_name = scene_name if scene_name is not None else sub_uuid
        xml += ('<xml item_type="{atype}" uuid="{uuid}"'
                ' simple_name="{name}" />').format(
                    uuid=sub_uuid, atype='scene',
                    name=scene_simple_name)

        scene_xml = (f'<xml item_type="scene" uuid="{sub_uuid}"'
                     f' simple_name="{scene_simple_name}" >'
                     f'<svg:svg xmlns:svg="http://www.w3.org/2000/svg" '
                     f'xmlns:krb="http://karabo.eu/scene" height="768" '
                     f'width="1024" krb:uuid="{sub_uuid}" krb:version="2">'
                     '</svg:svg></xml>')

        db.save_item("LOCAL", sub_uuid, scene_xml)

    xml += "</scenes>"

    # create some device_servers
    xml += "<servers>"
    device_id_conf_map = {}
    for i in range(4):
        sub_uuid = _gen_uuid()

        xml += ('<KRB_Item>'
                '<uuid>{uuid}</uuid>'
                '</KRB_Item>'.format(uuid=sub_uuid))
        ins_xml = ""
        for j in range(4):
            dev_uuid, conf_uuid = create_device(db)
            ins_xml += ('<device_instance '
                        'uuid="{uuid}" />'.format(uuid=dev_uuid))
            device_id_conf_map[dev_uuid] = conf_uuid

        ds_xml = ('<xml item_type="{atype}" uuid="{uuid}"'
                  ' simple_name="{name}" >'
                  '<device_server>'
                  '{instances}'
                  '</device_server>'
                  '</xml>'
                  .format(uuid=sub_uuid, atype='device_server', name=sub_uuid,
                          instances=ins_xml))

        db.save_item("LOCAL", sub_uuid, ds_xml)
    xml += "</servers>"
    xml += "</project>"
    xml += "</root>"
    xml += "</xml>"
    db.save_item("LOCAL", uuid, xml)
    return uuid, device_id_conf_map
