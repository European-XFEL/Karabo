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
from pytest import raises as assert_raises

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    MacroModel, ProjectModel, check_instance_duplicates, device_config_exists,
    device_instance_exists, find_parent_object, get_project_models,
    macro_exists)


def test_find_parent_object():
    dev0 = DeviceInstanceModel(instance_id="dev0")
    dev1 = DeviceInstanceModel(instance_id="dev1")
    serv0 = DeviceServerModel(server_id="serv0", devices=[dev0, dev1])
    dev2 = DeviceInstanceModel(instance_id="dev2")
    serv1 = DeviceServerModel(server_id="serv1", devices=[dev2])
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
    dev0 = DeviceInstanceModel(instance_id="dev0")
    serv0 = DeviceServerModel(server_id="serv0", devices=[dev0])
    sub_proj0 = ProjectModel(macros=[mac0])
    sub_proj1 = ProjectModel(macros=[mac1], servers=[serv0])
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])
    parentless = MacroModel()

    parent = find_parent_object(mac0, proj, ProjectModel)
    assert parent is sub_proj0

    parent = find_parent_object(mac1, proj, ProjectModel)
    assert parent is sub_proj1

    parent = find_parent_object(dev0, proj, ProjectModel)
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


def test_find_subproject_parent():
    sub_proj0 = ProjectModel()
    sub_proj1 = ProjectModel()
    proj = ProjectModel(subprojects=[sub_proj0, sub_proj1])

    parent = find_parent_object(sub_proj0, proj, ProjectModel)
    assert parent is proj
    parent = find_parent_object(sub_proj1, proj, ProjectModel)
    assert parent is proj


def test_device_instance_exists():
    dev0 = DeviceConfigurationModel(class_id="BazClass")
    foo = DeviceInstanceModel(
        class_id="BazClass", instance_id="fooDevice", configs=[dev0]
    )
    serv0 = DeviceServerModel(
        server_id="fooServer", host="serverserverFoo", devices=[foo]
    )
    dev1 = DeviceConfigurationModel(class_id="QuxClass")
    bar = DeviceInstanceModel(
        class_id="QuxClass", instance_id="barDevice", configs=[dev1]
    )
    serv1 = DeviceServerModel(
        server_id="barServer", host="serverserverFoo", devices=[bar]
    )

    proj = ProjectModel(servers=[serv0])
    assert device_instance_exists(proj, "fooDevice")
    assert not device_instance_exists(proj, "barDevice")
    proj.servers.append(serv1)
    assert device_instance_exists(proj, "barDevice")
    assert not device_instance_exists(proj, "blahDevice")
    assert device_instance_exists(proj, ("barDevice", "blahDevice"))

    dev0 = DeviceConfigurationModel(class_id="BazClass")
    blah = DeviceInstanceModel(
        class_id="BazClass", instance_id="blahDevice", configs=[dev0]
    )
    serv0 = DeviceServerModel(
        server_id="blahServer", host="serverserverBlah", devices=[blah]
    )
    sub_proj = ProjectModel(servers=[serv0])
    proj.subprojects.append(sub_proj)
    assert device_instance_exists(sub_proj, "blahDevice")
    assert device_instance_exists(proj, "blahDevice")

    # Test for duplicates, bar device lost its configuration
    bar2 = DeviceInstanceModel(
        class_id="NotQuxClass", instance_id="barDevice", configs=[]
    )
    serv0.devices.append(bar2)
    data = check_instance_duplicates(proj)
    assert data["Instances"] == {"devices": 3, "servers": 3}
    assert data["Duplicates"] == {
        "devices": {"barDevice": 2},
        "servers": {}
    }
    # And a new server
    serv1 = DeviceServerModel(
        server_id="blahServer", host="differenthost", devices=[]
    )
    proj.servers.append(serv1)
    data = check_instance_duplicates(proj)
    assert data["Instances"] == {"devices": 3, "servers": 3}
    assert data["Duplicates"] == {
        "devices": {"barDevice": 2},
        "servers": {"blahServer": 2}
    }


def test_device_config_exists():
    dev0 = DeviceConfigurationModel(class_id="BazClass")
    foo = DeviceInstanceModel(
        class_id="BazClass", instance_id="fooDevice", configs=[dev0]
    )
    serv0 = DeviceServerModel(
        server_id="fooServer", host="serverserverFoo", devices=[foo]
    )
    dev1 = DeviceConfigurationModel(
        class_id="QuxClass", simple_name="new-conf1"
    )
    bar = DeviceInstanceModel(
        class_id="QuxClass", instance_id="barDevice", configs=[dev1]
    )
    serv1 = DeviceServerModel(
        server_id="barServer", host="serverserverFoo", devices=[bar]
    )

    proj = ProjectModel(servers=[serv0])
    assert device_config_exists(proj, foo.instance_id, "default")
    assert not device_config_exists(proj, foo.instance_id, "new-conf1")
    proj.servers.append(serv1)
    assert not device_config_exists(proj, bar.instance_id, "new-conf2")
    assert device_config_exists(
        proj, foo.instance_id, ("default", "new-conf1")
    )

    dev0 = DeviceConfigurationModel(
        class_id="BazClass", simple_name="new-conf2"
    )
    blah = DeviceInstanceModel(
        class_id="BazClass", instance_id="blahDevice", configs=[dev0]
    )
    serv0 = DeviceServerModel(
        server_id="blahServer", host="serverserverBlah", devices=[blah]
    )
    sub_proj = ProjectModel(servers=[serv0])
    proj.subprojects.append(sub_proj)
    assert device_config_exists(sub_proj, blah.instance_id, "new-conf2")
    assert device_config_exists(proj, blah.instance_id, "new-conf2")


def test_macro_exists():
    foo = MacroModel(simple_name="fooMacro")
    bar = MacroModel(simple_name="barMacro")
    proj = ProjectModel(macros=[foo])
    assert macro_exists(proj, foo.simple_name)
    assert not macro_exists(proj, bar.simple_name)


def test_project_models():
    proj = ProjectModel()
    sub_project_1 = ProjectModel()
    sub_project_2 = ProjectModel()
    sub_sub_project_1 = ProjectModel()
    sub_sub_project_2 = ProjectModel()

    sub_project_1.subprojects = [sub_sub_project_1, sub_sub_project_2]
    proj.subprojects = [sub_project_1, sub_project_2]

    models = get_project_models(proj)
    assert proj in models
    assert sub_project_1 in models
    assert sub_project_2 in models
    assert sub_sub_project_1 in models
    assert sub_sub_project_2 in models
