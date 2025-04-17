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
from unittest import TestCase
from uuid import uuid4

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    read_device_server, write_device_server)
from karabo.testing.utils import temp_xml_file, xml_is_equal

UUIDS = [str(uuid4()) for i in range(5)]
SERVER_XML = """
<device_server server_id='testServer' host='serverserverFoo'>
    <device_instance uuid='{uuids[0]}' />
    <device_instance uuid='{uuids[1]}' />
</device_server>
""".format(
    uuids=UUIDS
)
INCOMPLETE_XML = """
<device_server uuid='{uuids[0]}'>
    <device_instance uuid='{uuids[1]}' />
</device_server>
""".format(
    uuids=UUIDS
)


class TestServer(TestCase):
    def setUp(self):
        push_exception_handler(lambda *args: None, reraise_exceptions=True)

    def tearDown(self):
        pop_exception_handler()

    def test_reading(self):
        with temp_xml_file(SERVER_XML) as fn:
            server = read_device_server(fn)

        assert server.server_id == "testServer"
        assert server.host == "serverserverFoo"
        assert len(server.devices) == 2

        dev0 = server.devices[0]
        assert dev0.uuid == UUIDS[0]
        assert not dev0.initialized

        dev1 = server.devices[1]
        assert dev1.uuid == UUIDS[1]
        assert not dev1.initialized

    def test_reading_incomplete(self):
        with temp_xml_file(INCOMPLETE_XML) as fn:
            server = read_device_server(fn)

        assert server.server_id == ""
        assert server.host == ""
        assert len(server.devices) == 1

        dev0 = server.devices[0]
        assert dev0.uuid == UUIDS[1]
        assert not dev0.initialized

    def test_writing(self):
        dev0 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[2])
        dev1 = DeviceConfigurationModel(class_id="QuxClass", uuid=UUIDS[2])
        dev2 = DeviceConfigurationModel(class_id="QuxClass", uuid=UUIDS[2])
        foo = DeviceInstanceModel(
            class_id="BazClass",
            instance_id="fooDevice",
            configs=[dev0],
            active_config_ref=UUIDS[2],
            uuid=UUIDS[0],
        )
        bar = DeviceInstanceModel(
            class_id="QuxClass",
            instance_id="barDevice",
            configs=[dev1, dev2],
            active_config_ref=UUIDS[2],
            uuid=UUIDS[1],
        )
        server = DeviceServerModel(
            server_id="testServer", host="serverserverFoo", devices=[foo, bar]
        )

        xml = write_device_server(server)
        assert xml_is_equal(SERVER_XML, xml)

    def test_simple_round_trip(self):
        with temp_xml_file(SERVER_XML) as fn:
            server = read_device_server(fn)

        xml = write_device_server(server)
        assert xml_is_equal(SERVER_XML, xml)

    def test_child_modification_tracking(self):
        dev0 = DeviceConfigurationModel(
            class_id="BazClass", uuid=UUIDS[2], initialized=True
        )
        dev1 = DeviceConfigurationModel(
            class_id="QuxClass", uuid=UUIDS[3], initialized=True
        )
        dev2 = DeviceConfigurationModel(
            class_id="QuxClass", uuid=UUIDS[4], initialized=True
        )
        foo = DeviceInstanceModel(
            class_id="BazClass",
            instance_id="fooDevice",
            configs=[dev0],
            active_config_ref=UUIDS[2],
            uuid=UUIDS[0],
            initialized=True,
        )
        bar = DeviceInstanceModel(
            class_id="QuxClass",
            instance_id="barDevice",
            configs=[dev1, dev2],
            active_config_ref=UUIDS[3],
            uuid=UUIDS[1],
            initialized=True,
        )
        server = DeviceServerModel(
            server_id="testServer",
            host="serverserverFoo",
            devices=[foo],
            initialized=True,
        )
        server.devices.append(bar)

        server.modified = foo.modified = False

        bar.active_config_ref = UUIDS[4]
        assert server.modified

        server.devices.pop()
        assert server.modified

        server.modified = False
        assert not server.modified

    def test_child_configuration_rejection(self):
        dev0 = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[1])
        dev1 = DeviceConfigurationModel(class_id="QuxClass", uuid=UUIDS[2])
        inst = DeviceInstanceModel(
            class_id="BazClass",
            instance_id="fooDevice",
            configs=[dev0],
            active_config_ref=UUIDS[1],
            uuid=UUIDS[0],
        )

        inst.configs.append(dev1)
        # Device changes the classId
        assert dev1.class_id == "BazClass"
        assert len(inst.configs) == 2

        inst.class_id = "QuxClass"
        assert len(inst.configs) == 2
        assert inst.configs[0].class_id == "QuxClass"
        assert inst.configs[1].class_id == "QuxClass"
        assert inst.active_config_ref == dev0.uuid

        inst.configs.remove(dev0)
        assert inst.active_config_ref == ""

    def test_child_finding(self):
        conf = DeviceConfigurationModel(class_id="BazClass", uuid=UUIDS[2])
        foo = DeviceInstanceModel(
            class_id="BazClass",
            instance_id="fooDevice",
            configs=[conf],
            active_config_ref=UUIDS[2],
            uuid=UUIDS[0],
        )
        bar = DeviceInstanceModel(
            class_id="BazClass",
            instance_id="barDevice",
            configs=[conf],
            active_config_ref=UUIDS[2],
            uuid=UUIDS[1],
        )
        server = DeviceServerModel(
            server_id="testServer", host="serverserverFoo", devices=[foo, bar]
        )

        found = server.get_device_instance("barDevice")
        assert found is bar

        not_found = server.get_device_instance("banana")
        assert not_found is None

    def test_child_server_id_minding(self):
        dev = DeviceInstanceModel(
            class_id="BazClass", instance_id="fooDevice", initialized=True
        )
        server = DeviceServerModel(
            server_id="testServer", host="machine", devices=[dev]
        )

        assert not dev.modified
        assert dev.server_id == server.server_id

        server.server_id = "anotherServer"
        assert not dev.modified
        assert dev.server_id == server.server_id

        second = DeviceInstanceModel(
            class_id="klaus", instance_id="chuck", initialized=True
        )
        server.devices.append(second)
        assert not second.modified
        assert second.server_id == server.server_id

        server.devices = []
        server.server_id = "lastOne"
        assert dev.server_id != server.server_id
        assert second.server_id != server.server_id

        dev.server_id = second.server_id = "garbage"
        server.devices = [second, dev]
        assert dev.server_id == server.server_id
        assert second.server_id == server.server_id
