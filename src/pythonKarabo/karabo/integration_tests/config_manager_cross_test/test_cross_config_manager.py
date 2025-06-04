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
import json
import os
from pathlib import Path

import pytest

from karabo.bound import AccessType, Hash, Logger
from karabo.bound.testing import ServerContext, eventLoop, sleepUntil

Logger.configure(Hash())

TEST_DEVICE_ID = "PropertyTestDevice"


@pytest.fixture(scope="module")
def configTest(eventLoop: eventLoop):
    MDL_SERVER_ID = "cfgManager_server"
    CPP_SERVER_ID = "propTest_server"

    DB_NAME = "test_cross_config_manager.db"
    db_path = Path(os.environ["KARABO"]).joinpath(
        "var", "data", "config_db", DB_NAME)

    # MANAGER_ID must match the default ConfigurationManagerId used by
    # the DeviceClient.
    MANAGER_ID = "KaraboConfigurationManager"
    config = {MANAGER_ID: {
        "classId": "ConfigurationManager", "dbName": DB_NAME}}
    mdl_server = ServerContext(
        MDL_SERVER_ID, [f"init={json.dumps(config)}"], api="mdl")

    config = {TEST_DEVICE_ID: {"classId": "PropertyTest"}}
    cpp_server = ServerContext(
        CPP_SERVER_ID, [f"init={json.dumps(config)}"], api="cpp")
    with mdl_server, cpp_server:
        # We wait for the device to be online
        remote = cpp_server.remote()
        sleepUntil(lambda: TEST_DEVICE_ID in remote.getDevices(), timeout=10)
        yield cpp_server

    db_path.unlink(missing_ok=True)


def test_save_get_config(configTest):
    """Checks that saving one config and then retrieving it by its exact
    name works.

    Also checks that invalid parameters to the save operation trigger
    RemoteExceptions. Examples of invalid parameters are out of range
    priority values and use of names already in use for saved configu-
    rations.
    """
    dc = configTest.remote()
    # Put changes from default
    dc.set(TEST_DEVICE_ID, Hash("int32Property", 20))

    # Gets the current device configuration for later checking
    dev_config = dc.get(TEST_DEVICE_ID)
    config_name = "PropertyTestConfigI"
    ok, msg = dc.saveInitConfiguration(config_name, [TEST_DEVICE_ID])

    text = (
        f"Save configuration for '{TEST_DEVICE_ID}' " f"failed: '{msg}'")
    assert ok, text

    ret = dc.getInitConfiguration(TEST_DEVICE_ID, config_name)
    text = (f"Get configuration named '{config_name}' failed for device "
            f"'{TEST_DEVICE_ID}: '{ret['reason']}")
    assert ret["success"], text

    # XXX: Must be item
    config = ret["config"]
    assert config["name"] == config_name

    configuration = config["config"]
    # XXX: How to make sure to get all paths
    schema = dc.getDeviceSchema(TEST_DEVICE_ID)
    for key in dev_config.keys():
        value = dev_config[key]
        is_node = schema.isNode(key)
        if is_node:
            continue
        if schema.hasDefaultValue(key):
            default = schema.getDefaultValue(key)
            if default == dev_config[key]:
                assert key not in configuration
                continue
        is_internal = (schema.hasAssignment(key)
                       and schema.isAssignmentInternal(key))
        mode = schema.getAccessMode(key)
        if not is_internal and mode in (AccessType.WRITE, AccessType.INIT):
            config_value = configuration[key]
            assert config_value == value

    # config_name was used for the successful save case. Always overwrite!
    ret = dc.saveInitConfiguration(config_name, [TEST_DEVICE_ID])
    assert ret[0]


def test_save_list_config(configTest):
    """Checks that saving more than one config and then retrieving them by
    specifying a common name part works.

    Also checks that an empty name retrieves all configurations saved
    for the device and that an unused name part returns an empty list
    of configurations.
    """
    dc = configTest.remote()
    config_name_2 = "PropertyTestConfigII"
    ok, msg = dc.saveInitConfiguration(config_name_2, [TEST_DEVICE_ID])
    text = f"Save configuration for '{TEST_DEVICE_ID}' " f"failed: '{msg}'"
    assert ok, text

    config_name_3 = "PropertyTestConfigIII"
    ok, msg = dc.saveInitConfiguration(config_name_3, [TEST_DEVICE_ID])
    text = f"Save configuration for '{TEST_DEVICE_ID}' " f"failed: '{msg}'"
    assert ok, text

    ret = dc.listInitConfigurations(TEST_DEVICE_ID, config_name_2)

    text = (f"List configuration containting '{config_name_2}' failed "
            f"for device '{TEST_DEVICE_ID}': {ret['reason']}")
    assert ret["success"], text

    assert len(ret["configs"]) == 2
    config = ret["configs"][0]
    assert config["name"] == config_name_2

    # NOTE: listInitConfigurations doesn't return the full details of
    #       each configuration. In particular, it does not return the
    #       config itself
    config = ret["configs"][1]
    assert config["name"] == config_name_3

    # Empty name part should retrieve all the configs for the device in
    # the database - the two saved in this test and a third one that should
    # have been saved by the previous test, _test_save_get_config.
    # NOTE: an attempt to reuse ret in here caused the test to
    #       crash immediately. Debugging the crash showed that it
    #       happened in the interop layer built with Boost::Python.
    ret = dc.listInitConfigurations(TEST_DEVICE_ID, "")
    text = (f"List configuration with empty name part failed for device "
            f"'{TEST_DEVICE_ID}': {ret['reason']}")
    assert ret["success"], text
    assert len(ret["configs"]) == 3

    # An unused name part returns an empty list.
    ret = dc.listInitConfigurations(TEST_DEVICE_ID, "An unused config name")
    assert ret["success"]
    assert len(ret["configs"]) == 0
