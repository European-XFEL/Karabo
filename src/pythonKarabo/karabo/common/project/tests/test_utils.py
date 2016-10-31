from nose.tools import assert_raises

from karabo.common.project.api import (
    ProjectModel, MacroModel, DeviceConfigurationModel, find_parent_project)


def test_find_parent_project():
    dev0 = DeviceConfigurationModel()
    dev1 = DeviceConfigurationModel()
    mac = MacroModel()
    sub_proj0 = ProjectModel(macros=[mac])
    sub_proj1 = ProjectModel(devices=[dev0])
    sub_proj2 = ProjectModel(devices=[dev1])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1, sub_proj2])
    parentless = DeviceConfigurationModel()

    parent = find_parent_project(mac, proj)
    assert parent is sub_proj0

    parent = find_parent_project(dev0, proj)
    assert parent is sub_proj1

    parent = find_parent_project(dev1, proj)
    assert parent is sub_proj2

    parent = find_parent_project(sub_proj0, proj)
    assert parent is proj

    parent = find_parent_project(parentless, proj)
    assert parent is None

    parent = find_parent_project(proj, proj)
    assert parent is None


def test_find_parent_project_degenerate():
    dev0 = DeviceConfigurationModel()
    # NOTE: dev0 is in more than one subproject!
    sub_proj0 = ProjectModel(devices=[dev0])
    sub_proj1 = ProjectModel(devices=[dev0])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])

    with assert_raises(RuntimeError):
        find_parent_project(dev0, proj)
