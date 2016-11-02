from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    read_device_server, write_device_server)
from karabo.testing.utils import temp_xml_file, xml_is_equal

UUID = 'c43e5c53-bea4-4e9e-921f-042b52e58f4c'
SERVER_XML = """
<device_server server_id='testServer'>
    <device instance_id='fooDevice'
            if_exists='ignore'
            active_uuid='{uuid}'
            active_rev='0'>
        <config uuid='{uuid}' revision='0'/>
    </device>
    <device instance_id='barDevice'
            if_exists='restart'
            active_uuid='{uuid}'
            active_rev='2'>
        <config uuid='{uuid}' revision='1'/>
        <config uuid='{uuid}' revision='2'/>
    </device>
</device_server>
""".format(uuid=UUID)


def test_reading():
    with temp_xml_file(SERVER_XML) as fn:
        server = read_device_server(fn)

    assert server.server_id == 'testServer'
    assert len(server.devices) == 2

    dev0 = server.devices[0]
    assert dev0.instance_id == 'fooDevice'
    assert dev0.if_exists == 'ignore'
    assert len(dev0.configs) == 1
    assert dev0.configs[0].revision == 0
    assert dev0.configs[0].uuid == UUID
    assert dev0.active_config_ref == (UUID, 0)

    dev1 = server.devices[1]
    assert dev1.instance_id == 'barDevice'
    assert dev1.if_exists == 'restart'
    assert len(dev1.configs) == 2
    assert dev1.configs[0].revision == 1
    assert dev1.configs[0].uuid == UUID
    assert dev1.configs[1].revision == 2
    assert dev1.configs[1].uuid == UUID
    assert dev1.active_config_ref == (UUID, 2)


def test_writing():
    dev0 = DeviceConfigurationModel(uuid=UUID, revision=0)
    dev1 = DeviceConfigurationModel(uuid=UUID, revision=1)
    dev2 = DeviceConfigurationModel(uuid=UUID, revision=2)
    foo = DeviceInstanceModel(instance_id='fooDevice', if_exists='ignore',
                              configs=[dev0], active_config_ref=(UUID, 0))
    bar = DeviceInstanceModel(instance_id='barDevice', if_exists='restart',
                              configs=[dev1, dev2],
                              active_config_ref=(UUID, 2))
    server = DeviceServerModel(server_id='testServer', devices=[foo, bar])

    xml = write_device_server(server)
    assert xml_is_equal(SERVER_XML, xml)
