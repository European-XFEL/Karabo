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

from karabo.bound import Hash, fullyEqual
from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.middlelayer_devices.configuration_manager import (
    KARABO_CONFIG_DB_FOLDER, ConfigurationManager)
from karabo.native.exceptions import KaraboError


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
    DB_PATH = op.join(KARABO_CONFIG_DB_FOLDER, DB_NAME)

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
        self._test_save_get_last_config()

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
        dev_cfg = self.dc.get(self.PROP_TEST_DEVICE_ID)

        cfg_name = 'PropertyTestConfigI'
        cfg_desc = 'A simple description for the configuration'
        ok, msg = self.dc.saveConfigurationFromName(
            cfg_name, [self.PROP_TEST_DEVICE_ID], cfg_desc
        )
        self.assertTrue(
            ok,
            f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'"
        )
        get_resp = self.dc.getConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, cfg_name
        )
        self.assertTrue(
            get_resp['success'],
            f"Get configuration named '{cfg_name}' failed for device "
            f"'{self.PROP_TEST_DEVICE_ID}: '{get_resp['reason']}"
        )
        config = get_resp['config']
        self.assertEqual(config['name'], cfg_name)
        self.assertEqual(config['description'], cfg_desc)
        self.assertEqual(config['priority'], 1)  # 1 is the default priority
        self.assertEqual(config['user'], '.')  # '.' is the "default" user
        self.assertTrue(fullyEqual(config['config'], dev_cfg))

        # Attempt to save with invalid priority is rejected
        get_resp_inv_pri = self.dc.saveConfigurationFromName(
            'An invalid priority', [self.PROP_TEST_DEVICE_ID], 'Desc.', 5
        )
        self.assertFalse(get_resp_inv_pri[0])
        self.assertIn('argument out of range', get_resp_inv_pri[1])

        # Attempt to save with existing name triggers RemoteException.
        # cfg_name was used for the successful save case.
        get_resp_used_name = self.dc.saveConfigurationFromName(
            cfg_name, [self.PROP_TEST_DEVICE_ID]
        )
        self.assertFalse(get_resp_used_name[0])
        self.assertIn('The config name PropertyTestConfigI is already taken',
                      get_resp_used_name[1])

    def _test_save_list_config(self):
        """Checks that saving more than one config and then retrieving them by
           specifying a common name part works.

           Also checks that an empty name retrieves all configurations saved
           for the device and that an unused name part returns an empty list
           of configurations.
        """
        cfg_name_2 = 'PropertyTestConfigII'
        cfg_desc_2 = 'Another simple description for the configuration'
        ok, msg = self.dc.saveConfigurationFromName(
            cfg_name_2, [self.PROP_TEST_DEVICE_ID], cfg_desc_2
        )
        self.assertTrue(
            ok,
            f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'"
        )
        cfg_name_3 = 'PropertyTestConfigIII'
        cfg_desc_3 = 'Yet another simple description for the configuration'
        ok, msg = self.dc.saveConfigurationFromName(
            cfg_name_3, [self.PROP_TEST_DEVICE_ID], cfg_desc_3
        )
        self.assertTrue(
            ok,
            f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'"
        )
        # cfg_desc_2 is part of cfg_desc_3, so the two are expected in the
        # reply
        get_resp = self.dc.listConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, cfg_name_2
        )
        self.assertTrue(
            get_resp['success'],
            f"List configuration with name containting '{cfg_name_2}' failed "
            f"for device '{self.PROP_TEST_DEVICE_ID}': {get_resp['reason']}"
        )
        self.assertEqual(len(get_resp['configs']), 2)
        config = get_resp['configs'][0]
        self.assertEqual(config['name'], cfg_name_2)
        self.assertEqual(config['description'], cfg_desc_2)
        self.assertEqual(config['priority'], 1)  # 1 is the default priority
        self.assertEqual(config['user'], '.')  # '.' is the "default" user
        # NOTE: listConfigurationFromName doesn't return the full details of
        #       each configuration. In particular, it neither returns the
        #       config itself nor the associated schema.
        config = get_resp['configs'][1]
        self.assertEqual(config['name'], cfg_name_3)
        self.assertEqual(config['description'], cfg_desc_3)
        self.assertEqual(config['priority'], 1)  # 1 is the default priority
        self.assertEqual(config['user'], '.')  # '.' is the "default" user

        # Empty name part should retrieve all the configs for the device in
        # the database - the two saved in this test and a third one that should
        # have been saved by the previous test, _test_save_get_config.
        # NOTE: an attempt to reuse get_resp in here caused the test to
        #       crash immediately. Debugging the crash showed that it
        #       happened in the interop layer built with Boost::Python.
        get_resp_all = self.dc.listConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, ''
        )
        self.assertTrue(
            get_resp_all['success'],
            f"List configuration with empty name part failed for device "
            f"'{self.PROP_TEST_DEVICE_ID}': {get_resp_all['reason']}"
        )
        self.assertEqual(len(get_resp_all['configs']), 3)

        # An unused name part returns an empty list.
        get_resp_unused = self.dc.listConfigurationFromName(
            self.PROP_TEST_DEVICE_ID, 'An unused config name'
        )
        self.assertTrue(get_resp_unused['success'])
        self.assertEqual(len(get_resp_unused['configs']), 0)

    def _test_save_get_last_config(self):
        """Checks that saving more than one config and then retrieving the
           latest with a given priority works.

           Also checks that an attempt to retrieve the latest configuration
           for a priority that hasn't yet been used triggers a RemoteException.
        """
        cfg_name_4 = 'PropertyTestConfigIV'
        cfg_desc_4 = 'Another simple description for the configuration'
        ok, msg = self.dc.saveConfigurationFromName(
            cfg_name_4, [self.PROP_TEST_DEVICE_ID], cfg_desc_4, 2
        )
        self.assertTrue(
            ok,
            f"Save configuration for device '{self.PROP_TEST_DEVICE_ID}' "
            f"failed: '{msg}'"
        )
        cfg_name_5 = 'PropertyTestConfigV'
        cfg_desc_5 = 'Yet another simple description for the configuration'
        ok, msg = self.dc.saveConfigurationFromName(
            cfg_name_5, [self.PROP_TEST_DEVICE_ID], cfg_desc_5, 1
        )

        get_resp = self.dc.getLastConfiguration(
            self.PROP_TEST_DEVICE_ID, 2
        )
        self.assertTrue(
            get_resp['success'],
            "Get last configuration with priority '2' failed for device "
            f"'{self.PROP_TEST_DEVICE_ID}: '{get_resp['reason']}"
        )
        config = get_resp['config']
        self.assertEqual(config['name'], cfg_name_4)
        self.assertEqual(config['description'], cfg_desc_4)
        self.assertEqual(config['priority'], 2)

        get_resp_pri1 = self.dc.getLastConfiguration(
            self.PROP_TEST_DEVICE_ID, 1
        )
        self.assertTrue(
            get_resp_pri1['success'],
            "Get last configuration with priority '1' failed for device "
            f"'{self.PROP_TEST_DEVICE_ID}: '{get_resp_pri1['reason']}"
        )
        config = get_resp_pri1['config']
        self.assertEqual(config['name'], cfg_name_5)
        self.assertEqual(config['description'], cfg_desc_5)
        self.assertEqual(config['priority'], 1)

        # An attempt to retrieve the last configuration of a given
        # priority when such a configuration has not yet been saved,
        # raises a RemoteException and returns a failure.
        get_resp_pri3 = self.dc.getLastConfiguration(
            self.PROP_TEST_DEVICE_ID, 3
        )
        self.assertEqual(get_resp_pri3['success'], False)
        self.assertIn('Remote Exception', get_resp_pri3['reason'])
        self.assertIn('No configuration for device', get_resp_pri3['reason'])


if __name__ == '__main__':
    unittest.main()
