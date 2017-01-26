from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel, device_instance_exists, find_parent_object
)
from nose.tools import assert_raises


def test_find_parent_object():
    dev0 = DeviceInstanceModel(instance_id='dev0')
    dev1 = DeviceInstanceModel(instance_id='dev1')
    serv0 = DeviceServerModel(server_id='serv0', devices=[dev0, dev1])
    dev2 = DeviceInstanceModel(instance_id='dev2')
    serv1 = DeviceServerModel(server_id='serv1', devices=[dev2])
    macro = MacroModel(code="print('hello world')")
    inner_proj = ProjectModel(macros=[macro], servers=[serv0])
    proj = ProjectModel(subprojects=[inner_proj], servers=[serv1])

    parent = find_parent_object(inner_proj, proj, ProjectModel)
    assert parent is proj

    parent = find_parent_object(macro, proj, ProjectModel)
    assert parent is inner_proj

    parent = find_parent_object(dev0, proj, DeviceServerModel)
    assert parent is serv0


def test_find_parent_project():
    mac0 = MacroModel()
    mac1 = MacroModel()
    sub_proj0 = ProjectModel(macros=[mac0])
    sub_proj1 = ProjectModel(macros=[mac1])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])
    parentless = MacroModel()

    parent = find_parent_object(mac0, proj, ProjectModel)
    assert parent is sub_proj0

    parent = find_parent_object(mac1, proj, ProjectModel)
    assert parent is sub_proj1

    parent = find_parent_object(sub_proj0, proj, ProjectModel)
    assert parent is proj

    parent = find_parent_object(parentless, proj, ProjectModel)
    assert parent is None

    parent = find_parent_object(proj, proj, ProjectModel)
    assert parent is None


def test_find_parent_project_degenerate():
    mac = MacroModel()
    # NOTE: mac is in more than one subproject!
    sub_proj0 = ProjectModel(macros=[mac])
    sub_proj1 = ProjectModel(macros=[mac])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])

    with assert_raises(RuntimeError):
        find_parent_object(mac, proj, ProjectModel)


def test_device_instance_exists():
    dev0 = DeviceConfigurationModel(class_id='BazClass')
    foo = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                              configs=[dev0])
    serv0 = DeviceServerModel(server_id='fooServer', host='serverserverFoo',
                              devices=[foo])
    dev1 = DeviceConfigurationModel(class_id='QuxClass')
    bar = DeviceInstanceModel(class_id='QuxClass', instance_id='barDevice',
                              configs=[dev1])
    serv1 = DeviceServerModel(server_id='barServer', host='serverserverFoo',
                              devices=[bar])

    proj = ProjectModel(servers=[serv0])
    assert device_instance_exists(proj, 'fooDevice')
    assert not device_instance_exists(proj, 'barDevice')
    proj.servers.append(serv1)
    assert device_instance_exists(proj, 'barDevice')
    assert not device_instance_exists(proj, 'blahDevice')

    dev0 = DeviceConfigurationModel(class_id='BazClass')
    blah = DeviceInstanceModel(class_id='BazClass', instance_id='blahDevice',
                               configs=[dev0])
    serv0 = DeviceServerModel(server_id='blahServer', host='serverserverBlah',
                              devices=[blah])
    sub_proj = ProjectModel(servers=[serv0])
    proj.subprojects.append(sub_proj)
    assert device_instance_exists(sub_proj, 'blahDevice')
    assert device_instance_exists(proj, 'blahDevice')
