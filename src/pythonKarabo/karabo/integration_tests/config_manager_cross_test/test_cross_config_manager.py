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
import os
import os.path as op
import unittest

from karabo.bound import Hash, Logger
from karabo.bound.testing import BoundDeviceTestCase

Logger.configure(Hash())


class TestCrossConfigManager(BoundDeviceTestCase):
    """Tests for sync and async DeviceClient operations related to the
       ConfigurationManager.

       Uses a Cpp PropertyTest device instance and an instance of the MDL
       ConfigurationManager device.
    """
    MAX_TIMEOUT = 60  # in seconds

    MDL_SERVER_ID = 'cfgManager_server'
    CPP_SERVER_ID = 'propTest_server'

    DB_NAME = 'test_cross_config_manager.db'
    DB_PATH = op.join(os.environ["KARABO"], 'var', 'data',
                      'config_db', DB_NAME)

    # CFG_MANAGER_ID must match the default ConfigurationManagerId used by
    # the DeviceClient.
    CFG_MANAGER_ID = 'KaraboConfigurationManager'
    CFG_MANAGER_CLASS_ID = 'ConfigurationManager'

    PROP_TEST_DEVICE_ID = 'PropertyTestDevice'
    PROP_TEST_CLASS_ID = 'PropertyTest'

    def test_sync_operations(self):
        self._setup_devices()
        self._test_save_get_config()
        self._test_save_list_config()

    def _setup_devices(self):
        # Starts the MDL server that will host the ConfigurationManager device.
        self.start_server('mdl', self.MDL_SERVER_ID,
                          [self.CFG_MANAGER_CLASS_ID])
        # Starts the Cpp server that will host the PropertyTest device.
        self.start_server('cpp', self.CPP_SERVER_ID,
                          [self.PROP_TEST_CLASS_ID])
        # Instantiates the ConfigurationManager device.
        manager_config = Hash(
            'classId', self.CFG_MANAGER_CLASS_ID,
            'deviceId', self.CFG_MANAGER_ID,
            'configuration', Hash('dbName', self.DB_NAME)
        )
        ok, msg = self.dc.instantiate(
            self.MDL_SERVER_ID, self.CFG_MANAGER_CLASS_ID,
            manager_config, self.MAX_TIMEOUT
        )
        self.assertTrue(
            ok,
            f"Could not start ConfigurationManager '{self.CFG_MANAGER_ID}' "
            f"on server '{self.MDL_SERVER_ID}': '{msg}'."
        )
        # Instantiates the Cpp PropertyTest device.
        proptest_config = Hash('deviceId', self.PROP_TEST_DEVICE_ID)
        ok, msg = self.dc.instantiate(
            self.CPP_SERVER_ID, self.PROP_TEST_CLASS_ID,
            proptest_config, self.MAX_TIMEOUT
        )
        self.assertTrue(
            ok,
            f"Could not start PropertyTest '{self.PROP_TEST_DEVICE_ID}' "
            f"on server '{self.CPP_SERVER_ID}': '{msg}'."
        )

    def tearDown(self):
        super().tearDown()  # Shutdown all Device Servers created by tests.
        # Clean-up the ConfigurationManager database.
        os.remove(self.DB_PATH)

    def _test_save_get_config(self):
        """Checks that saving one config and then retrieving it by its exact
           name works.

           Also checks that invalid parameters to the save operation trigger
           RemoteExceptions. Examples of invalid parameters are out of range
           priority values and use of names already in use for saved configu-
           rations.
        """
        # Gets the current device configuration for later checking
        dev_config = self.dc.get(self.PROP_TEST_DEVICE_ID)

        config_name = 'PropertyTestConfigI'
        ok, msg = self.dc.saveConfigurationFromName(
            config_name, [self.PROP_TEST_DEVICE_ID])
        self.assertTrue(
            ok,
            f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'"
        )
        ret = self.dc.getConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, config_name
        )
        self.assertTrue(
            ret['success'],
            f"Get configuration named '{config_name}' failed for device "
            f"'{self.PROP_TEST_DEVICE_ID}: '{ret['reason']}")
        config = ret['config']
        self.assertEqual(config['name'], config_name)
        # XXX: Config Attributes for Tables have a rowSchema, hence
        # cannot use fullyEqual at the moment
        for node in dev_config:
            value = node.getValue()
            key = node.getKey()
            assert config['config'][key] == value

        # config_name was used for the successful save case. Always overwrite!
        ret_used_name = self.dc.saveConfigurationFromName(
            config_name, [self.PROP_TEST_DEVICE_ID])
        self.assertTrue(ret_used_name[0])

    def _test_save_list_config(self):
        """Checks that saving more than one config and then retrieving them by
           specifying a common name part works.

           Also checks that an empty name retrieves all configurations saved
           for the device and that an unused name part returns an empty list
           of configurations.
        """
        config_name_2 = 'PropertyTestConfigII'
        ok, msg = self.dc.saveConfigurationFromName(
            config_name_2, [self.PROP_TEST_DEVICE_ID])
        self.assertTrue(
            ok, f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'")
        config_name_3 = 'PropertyTestConfigIII'
        ok, msg = self.dc.saveConfigurationFromName(
            config_name_3, [self.PROP_TEST_DEVICE_ID])
        self.assertTrue(
            ok,
            f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'")

        ret = self.dc.listConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, config_name_2)
        self.assertTrue(
            ret['success'],
            f"List configuration containting '{config_name_2}' failed "
            f"for device '{self.PROP_TEST_DEVICE_ID}': {ret['reason']}")
        self.assertEqual(len(ret['configs']), 2)
        config = ret['configs'][0]
        self.assertEqual(config['name'], config_name_2)
        # NOTE: listConfigurationFromName doesn't return the full details of
        #       each configuration. In particular, it neither returns the
        #       config itself nor the associated schema.
        config = ret['configs'][1]
        self.assertEqual(config['name'], config_name_3)
        # Empty name part should retrieve all the configs for the device in
        # the database - the two saved in this test and a third one that should
        # have been saved by the previous test, _test_save_get_config.
        # NOTE: an attempt to reuse ret in here caused the test to
        #       crash immediately. Debugging the crash showed that it
        #       happened in the interop layer built with Boost::Python.
        ret_all = self.dc.listConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, '')
        self.assertTrue(
            ret_all['success'],
            f"List configuration with empty name part failed for device "
            f"'{self.PROP_TEST_DEVICE_ID}': {ret_all['reason']}"
        )
        self.assertEqual(len(ret_all['configs']), 3)

        # An unused name part returns an empty list.
        ret_unused = self.dc.listConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, 'An unused config name'
        )
        self.assertTrue(ret_unused['success'])
        self.assertEqual(len(ret_unused['configs']), 0)


if __name__ == '__main__':
    unittest.main()
