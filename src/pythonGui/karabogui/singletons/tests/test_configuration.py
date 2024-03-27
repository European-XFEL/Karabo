# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import pytest

from ..configuration import (
    BACKBONE, DIRECTORIES, NETWORK, PANEL, PROJECT, USER, Configuration)


class MockSettings:

    def __init__(self):
        self.last_path = None

    @classmethod
    def value(cls, path):
        return getattr(cls, path, None)

    @classmethod
    def setValue(cls, name, value):
        setattr(cls, name, value)

    def remove(self, key):
        self.last_path = key

    def __call__(self, *args, **kwargs):
        return self


def test_configuration_namespace():
    config = Configuration()
    config['broker_topic'] = 'FXE'
    config['device_domain'] = 'CAS_INTERNAL'
    config['domain'] = 'CAS_INTERNAL'
    config['db_token'] = 'admin'
    assert len(config) == 21
    assert config['db_token'] == 'admin'
    assert config['device_domain'] == 'CAS_INTERNAL'
    assert config['domain'] == 'CAS_INTERNAL'

    keys = [
        'access_level',
        'broker_topic',
        'console_visible',
        'data_dir',
        'db_token',
        'development',
        'device_domain',
        'documentation',
        'domain',
        'gui_servers',
        'highDPI',
        'logbook_header_style',
        'logbook_stream',
        'logbook_topic',
        'macro_development',
        'main_geometry',
        'project_sort_column',
        'project_sort_order',
        'reactive_login',
        'username',
        'wizard']

    assert list(config.keys()) == keys


def test_set_wrong_key():
    config = Configuration()
    with pytest.raises(KeyError):
        config['noitem'] = "Beep"

    with pytest.raises(KeyError):
        config.noitem = "Boom"


def test_default_value():
    """Every item that specifies a data type must have a default value"""
    config = Configuration()
    counter = 0
    for key in config.keys():
        item = getattr(config.__class__, key)
        if item.dtype is not None:
            counter += 1
            assert item.default is not None
    assert counter == 7


def test_set_bool_value(mocker):
    target = 'karabogui.singletons.configuration.QSettings'
    mocker.patch(target, new=MockSettings)
    config = Configuration()
    assert config["wizard"] is True
    config["wizard"] = False
    config['db_token'] = "observer"
    assert config["wizard"] is False
    assert config["db_token"] == "observer"


def test_erase_value(mocker):
    target = 'karabogui.singletons.configuration.QSettings'
    mock = MockSettings()
    mocker.patch(target, new=mock)
    config = Configuration()
    assert config["wizard"] is True
    config["wizard"] = False
    del config["wizard"]
    assert mock.last_path == "user/wizard"


def test_configuration_groups_info():
    config = Configuration()
    groups = config.groups()
    assert len(groups) == 6
    user_group = [item.name for item in groups[USER]]
    assert len(user_group) == 9
    assert 'wizard' in user_group
    assert 'main_geometry' in user_group
    assert 'highDPI' in user_group
    assert 'development' in user_group
    assert 'project_sort_column' in user_group
    assert 'project_sort_order' in user_group
    network_group = [item.name for item in groups[NETWORK]]
    assert len(network_group) == 5
    assert 'access_level' in network_group
    assert 'username' in network_group
    assert 'gui_servers' in network_group
    assert 'reactive_login' in network_group
    project_group = [item.name for item in groups[PROJECT]]
    assert len(project_group) == 3
    assert 'db_token' in project_group
    assert 'domain' in project_group
    assert 'device_domain' in project_group
    dir_group = [item.name for item in groups[DIRECTORIES]]
    assert len(dir_group) == 1
    assert 'data_dir' in dir_group
    bone_group = [item.name for item in groups[BACKBONE]]
    assert len(bone_group) == 2
    assert 'broker_topic' in bone_group
    assert 'documentation' in bone_group
    panel_group = [item.name for item in groups[PANEL]]
    assert len(panel_group) == 1
    assert 'console_visible' in panel_group

    info = config.info()
    assert PANEL in info
    assert USER in info
