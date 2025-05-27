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
    BACKBONE, DIRECTORIES, NETWORK, PROJECT, USER, Configuration)


class MockSettings:
    last_path = None

    @classmethod
    def value(cls, path):
        return getattr(cls, path, None)

    @classmethod
    def setValue(cls, name, value):
        setattr(cls, name, value)

    @classmethod
    def remove(cls, key):
        cls.last_path = key
        setattr(cls, key, None)

    def __call__(self, *args, **kwargs):
        return self


def test_configuration_namespace():
    config = Configuration()
    config['broker_topic'] = 'FXE'
    config['domain'] = 'CAS_INTERNAL'
    config['db_token'] = 'expert'
    assert len(config) == 20
    assert config['db_token'] == 'expert'
    assert config['domain'] == 'CAS_INTERNAL'

    keys = [
        'access_level',
        'broker_topic',
        'check_updates',
        'data_dir',
        'db_token',
        'development',
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
        'refresh_token',
        'refresh_token_user',
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
    assert counter == 6


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
    # Nothing happens for cached value
    assert config["wizard"] is False
    assert mock.last_path == "user/wizard"
    # Shared item
    assert config["refresh_token"] is None
    config["refresh_token"] = "12345"
    assert config["refresh_token"] == "12345"
    del config["refresh_token"]
    assert mock.last_path == "authentication/refresh_token"
    assert config["refresh_token"] is None


def test_configuration_groups_info():
    config = Configuration()
    groups = config.groups()
    assert len(groups) == 5
    user_group = [item.name for item in groups[USER]]
    assert len(user_group) == 10
    assert 'wizard' in user_group
    assert 'main_geometry' in user_group
    assert 'highDPI' in user_group
    assert 'development' in user_group
    assert 'project_sort_column' in user_group
    assert 'project_sort_order' in user_group
    network_group = [item.name for item in groups[NETWORK]]
    assert len(network_group) == 4
    assert 'access_level' in network_group
    assert 'username' in network_group
    assert 'gui_servers' in network_group
    project_group = [item.name for item in groups[PROJECT]]
    assert len(project_group) == 2
    assert 'db_token' in project_group
    assert 'domain' in project_group
    dir_group = [item.name for item in groups[DIRECTORIES]]
    assert len(dir_group) == 1
    assert 'data_dir' in dir_group
    bone_group = [item.name for item in groups[BACKBONE]]
    assert len(bone_group) == 1
    assert 'broker_topic' in bone_group

    info = config.info()
    assert USER in info
