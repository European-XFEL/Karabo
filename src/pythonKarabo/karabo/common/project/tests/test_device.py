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

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.api import walk_traits_object
from karabo.common.project.api import (
    BaseProjectObjectModel, DeviceConfigurationModel, DeviceInstanceModel,
    read_device, write_device)
from karabo.testing.utils import temp_xml_file, xml_is_equal

UUIDS = [str(uuid4()) for i in range(3)]
DEVICE_XML = """
<device_instance class_id='BazClass'
                 instance_id='fooDevice'
                 active_uuid='{uuids[0]}'>
    <device_config uuid='{uuids[0]}' class_id='BazClass' />
    <device_config uuid='{uuids[1]}' class_id='BazClass' />
    <device_config uuid='{uuids[2]}' class_id='BazClass' />
</device_instance>
""".format(
    uuids=UUIDS
)
INCOMPLETE_XML = """
<device_instance>
    <device_config uuid='{uuids[0]}' />
</device_instance>
""".format(
    uuids=UUIDS
)


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_reading():
    with temp_xml_file(DEVICE_XML) as fn:
        device = read_device(fn)

    assert device.class_id == "BazClass"
    assert device.instance_id == "fooDevice"
    assert device.active_config_ref == UUIDS[0]
    assert not device.initialized
    conf0 = device.configs[0]
    assert not conf0.initialized
    conf1 = device.configs[1]
    assert not conf1.initialized
    conf2 = device.configs[2]
    assert not conf2.initialized

    conf0.initialized = True
    assert not device.initialized
    conf1.initialized = True
    assert not device.initialized
    conf2.initialized = True
    assert device.initialized


def test_reading_incomplete():
    with temp_xml_file(INCOMPLETE_XML) as fn:
        device = read_device(fn)

    assert device.class_id == ""
    assert device.instance_id == ""
    assert device.active_config_ref == ""
    assert not device.initialized


def test_writing():
    conf0 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[0])
    conf1 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[1])
    conf2 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[2])
    foo = DeviceInstanceModel(
        class_id="BazClass",
        instance_id="fooDevice",
        configs=[conf0, conf1, conf2],
        active_config_ref=UUIDS[0],
    )

    xml = write_device(foo)
    assert xml_is_equal(DEVICE_XML, xml)


def test_simple_round_trip():
    with temp_xml_file(DEVICE_XML) as fn:
        device = read_device(fn)

    xml = write_device(device)
    assert xml_is_equal(DEVICE_XML, xml)


def test_child_finding():
    conf0 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[0])
    conf1 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[1])
    foo = DeviceInstanceModel(
        class_id="BazClass",
        instance_id="fooDevice",
        configs=[conf0, conf1],
        active_config_ref=UUIDS[0],
    )
    conf = foo.select_config(UUIDS[1])
    assert conf == conf1

    conf = foo.select_config(UUIDS[2])
    assert conf is None


def test_uuid_update():
    def _visitor(model):
        if isinstance(model, BaseProjectObjectModel):
            model.reset_uuid()

    conf0_uuid = str(uuid4())
    conf1_uuid = str(uuid4())
    dev_uuid = str(uuid4())
    conf0 = DeviceConfigurationModel(class_id="BarClass", uuid=conf0_uuid)
    conf1 = DeviceConfigurationModel(class_id="BarClass", uuid=conf1_uuid)
    dev = DeviceInstanceModel(
        class_id="BarClass",
        instance_id="fooDevice",
        configs=[conf0, conf1],
        active_config_ref=conf0_uuid,
        uuid=dev_uuid,
    )

    selected_conf = dev.select_config(conf0_uuid)
    assert selected_conf is conf0

    walk_traits_object(dev, _visitor)
    assert conf0.uuid != conf0_uuid
    assert conf1.uuid != conf1_uuid
    assert dev.uuid != dev_uuid
    assert dev.active_config_ref == conf0.uuid

    selected_conf = dev.select_config(conf0.uuid)
    assert selected_conf is conf0
