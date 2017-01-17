from nose.tools import assert_raises
from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    read_device_server, write_device_server)
from karabo.testing.utils import temp_xml_file, xml_is_equal

UUID = 'c43e5c53-bea4-4e9e-921f-042b52e58f4c'
SERVER_XML = """
<device_server server_id='testServer' host='serverserverFoo'>
    <device_instance uuid='{uuid}' revision='0' />
    <device_instance uuid='{uuid}' revision='1' />
</device_server>
""".format(uuid=UUID)


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_reading():
    with temp_xml_file(SERVER_XML) as fn:
        server = read_device_server(fn)

    assert server.server_id == 'testServer'
    assert server.host == 'serverserverFoo'
    assert len(server.devices) == 2

    dev0 = server.devices[0]
    assert dev0.uuid == UUID
    assert dev0.revision == 0
    assert not dev0.initialized

    dev1 = server.devices[1]
    assert dev1.uuid == UUID
    assert dev1.revision == 1
    assert not dev1.initialized


def test_writing():
    dev0 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID, revision=0)
    dev1 = DeviceConfigurationModel(class_id='QuxClass', uuid=UUID, revision=1)
    dev2 = DeviceConfigurationModel(class_id='QuxClass', uuid=UUID, revision=2)
    foo = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                              if_exists='ignore', configs=[dev0],
                              active_config_ref=(UUID, 0), uuid=UUID,
                              revision=0)
    bar = DeviceInstanceModel(class_id='QuxClass', instance_id='barDevice',
                              if_exists='restart', configs=[dev1, dev2],
                              active_config_ref=(UUID, 2), uuid=UUID,
                              revision=1)
    server = DeviceServerModel(server_id='testServer', host='serverserverFoo',
                               devices=[foo, bar])

    xml = write_device_server(server)
    assert xml_is_equal(SERVER_XML, xml)


def test_simple_round_trip():
    with temp_xml_file(SERVER_XML) as fn:
        server = read_device_server(fn)

    xml = write_device_server(server)
    assert xml_is_equal(SERVER_XML, xml)


def test_child_modification_tracking():
    dev0 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID, revision=0,
                                    initialized=True)
    dev1 = DeviceConfigurationModel(class_id='QuxClass', uuid=UUID, revision=1,
                                    initialized=True)
    dev2 = DeviceConfigurationModel(class_id='QuxClass', uuid=UUID, revision=2,
                                    initialized=True)
    foo = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                              if_exists='ignore', configs=[dev0],
                              active_config_ref=(UUID, 0), uuid=UUID,
                              revision=0, initialized=True)
    bar = DeviceInstanceModel(class_id='QuxClass', instance_id='barDevice',
                              if_exists='restart', configs=[dev1, dev2],
                              active_config_ref=(UUID, 2), uuid=UUID,
                              revision=1, initialized=True)
    server = DeviceServerModel(server_id='testServer', host='serverserverFoo',
                               devices=[foo], initialized=True)
    server.devices.append(bar)

    server.modified = False
    foo.if_exists = 'restart'
    assert server.modified

    server.modified = foo.modified = False

    bar.active_config_ref = (UUID, 1)
    assert server.modified

    server.devices.pop()
    server.modified = False
    foo.if_exists = 'ignore'
    assert server.modified


def test_child_configuration_rejection():
    dev0 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID, revision=0)
    dev1 = DeviceConfigurationModel(class_id='QuxClass', uuid=UUID, revision=1)
    inst = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                               if_exists='ignore', configs=[dev0],
                               active_config_ref=(UUID, 0), uuid=UUID,
                               revision=0)

    with assert_raises(ValueError):
        inst.configs.append(dev1)
    assert len(inst.configs) == 1

    inst.class_id = 'QuxClass'
    assert len(inst.configs) == 0
    assert inst.active_config_ref == ('', 0)

    inst.configs.append(dev1)
    assert len(inst.configs) == 1


def test_child_finding():
    conf = DeviceConfigurationModel(class_id='BazClass', uuid=UUID, revision=0)
    foo = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                              configs=[conf], active_config_ref=(UUID, 0),
                              uuid=UUID, revision=0)
    bar = DeviceInstanceModel(class_id='BazClass', instance_id='barDevice',
                              configs=[conf], active_config_ref=(UUID, 0),
                              uuid=UUID, revision=1)
    server = DeviceServerModel(server_id='testServer', host='serverserverFoo',
                               devices=[foo, bar])

    found = server.get_device_instance('barDevice')
    assert found is bar

    not_found = server.get_device_instance('banana')
    assert not_found is None
