from nose.tools import assert_raises

from karabo.common.project.api import (ProjectModel, MacroModel,
                                       find_parent_project)


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
