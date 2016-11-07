from karabo.common.project.api import (
    DeviceInstanceModel, DeviceServerModel, MacroModel, ProjectModel,
    find_parent_object, find_parent_project
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

    parent = find_parent_object(inner_proj, proj)
    assert parent is proj

    parent = find_parent_object(macro, proj)
    assert parent is inner_proj

    parent = find_parent_object(dev0, proj)
    assert parent is serv0


def test_find_parent_project():
    mac0 = MacroModel()
    mac1 = MacroModel()
    sub_proj0 = ProjectModel(macros=[mac0])
    sub_proj1 = ProjectModel(macros=[mac1])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])
    parentless = MacroModel()

    parent = find_parent_project(mac0, proj)
    assert parent is sub_proj0

    parent = find_parent_project(mac1, proj)
    assert parent is sub_proj1

    parent = find_parent_project(sub_proj0, proj)
    assert parent is proj

    parent = find_parent_project(parentless, proj)
    assert parent is None

    parent = find_parent_project(proj, proj)
    assert parent is None


def test_find_parent_project_degenerate():
    mac = MacroModel()
    # NOTE: mac is in more than one subproject!
    sub_proj0 = ProjectModel(macros=[mac])
    sub_proj1 = ProjectModel(macros=[mac])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])

    with assert_raises(RuntimeError):
        find_parent_project(mac, proj)
