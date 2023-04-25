# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import unittest
from datetime import datetime

from ..configuration_database import ConfigurationDatabase, DbHandle
from ..utils import (
    CONFIG_DB_CONFIG_SET_ID, CONFIG_DB_DATA, CONFIG_DB_DESCRIPTION,
    CONFIG_DB_DEVICE_ID, CONFIG_DB_DIFF_TIMEPOINT, CONFIG_DB_MAX_TIMEPOINT,
    CONFIG_DB_MIN_TIMEPOINT, CONFIG_DB_NAME, CONFIG_DB_OVERWRITABLE,
    CONFIG_DB_PRIORITY, CONFIG_DB_SCHEMA, CONFIG_DB_TIMEPOINT, CONFIG_DB_USER,
    ISO8601_FORMAT, ConfigurationDBError)

TEST_DB_PATH = 'configManagerTest.db'

CFG_NAME_1 = 'FirstConfig'
CFG_NAME_2 = 'SecondConfig'
CFG_NAME_3 = 'ThirdConfig'

CFG_DEVICE_1 = 'SCS/CAM/BASLER'
CFG_DEVICE_2 = 'CAS_LAB/DIGI/ADQ-12'
CFG_DEVICE_3 = 'KaraboProjectManager'

TIMESTAMP_1 = '2020-01-30T20:02:02.123000'
TIMESTAMP_2 = '2019-01-30T09:02:02.123000'


def create_config(deviceId):
    return {CONFIG_DB_DEVICE_ID: deviceId,
            CONFIG_DB_DATA: 'JFDHFYE947AGEUFHFSJDPOWIAKLSIDHD',
            CONFIG_DB_SCHEMA: 'QREPLCMBab789oeiryJAAHDDBAVDUUWY93948==='}


CFG_1 = [create_config(CFG_DEVICE_1)]
CFG_2 = [create_config(CFG_DEVICE_2)]
CFG_3 = [create_config(CFG_DEVICE_3)]

CFG_SET = [create_config(CFG_DEVICE_1), create_config(CFG_DEVICE_2)]
CFG_SET_2 = [
    create_config(CFG_DEVICE_1), create_config(CFG_DEVICE_2),
    create_config(CFG_DEVICE_3)]


class TestConfigurationManagerData(unittest.TestCase):

    @classmethod
    def setUp(cls):
        cls.connection = DbHandle(TEST_DB_PATH)
        cls.database = ConfigurationDatabase(cls.connection)
        cls.database.assureExisting()

    @classmethod
    def tearDown(cls):
        os.remove(cls.database.path)

    def test_save_get_config(self):
        """ Checks that saving one config and then retrieving it by its exact
            name works.
        """
        self.database.save_configuration(CFG_NAME_1, CFG_1)
        cfg = self.database.get_configuration(CFG_DEVICE_1, CFG_NAME_1)

        self.assertEqual(CFG_NAME_1, cfg['name'])
        self.assertEqual(CFG_1[0][CONFIG_DB_DATA], cfg[CONFIG_DB_DATA])
        self.assertEqual(CFG_1[0][CONFIG_DB_SCHEMA], cfg[CONFIG_DB_SCHEMA])
        self.assertEqual('', cfg[CONFIG_DB_DESCRIPTION])  # Empty default
        self.assertEqual('.', cfg[CONFIG_DB_USER])  # Default anonymous user
        self.assertEqual(1, cfg[CONFIG_DB_PRIORITY])  # Default priority, 1

        # Trying to retrieve with a non-existing name should return nothing.
        cfg = self.database.get_configuration(CFG_DEVICE_1,
                                              'Non-existing name')
        self.assertEqual(cfg, {})

    def test_save_get_config_name_part(self):
        """ Checks that saving a set of configs and then retrieving them by
            name parts works.
        """
        # Save with priority 1
        self.database.save_configuration(CFG_NAME_1, CFG_1,
                                         timestamp=TIMESTAMP_1)

        # CFG_SET contains one config for CFG_DEVICE_1 and one for CFG_DEVICE_2
        # in that order.
        self.database.save_configuration(CFG_NAME_2, CFG_SET,
                                         description='Bla bla',
                                         user='Bob',
                                         priority=3,
                                         timestamp=TIMESTAMP_2)

        cfg = self.database.list_configurations(CFG_DEVICE_1, CFG_NAME_2[:4])
        self.assertEqual(len(cfg), 1)
        self.assertEqual(CFG_NAME_2, cfg[0][CONFIG_DB_NAME])
        self.assertEqual(cfg[0][CONFIG_DB_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual('Bla bla', cfg[0][CONFIG_DB_DESCRIPTION])
        self.assertEqual('Bob', cfg[0][CONFIG_DB_USER])
        self.assertEqual(3, cfg[0][CONFIG_DB_PRIORITY])
        # The config wasn't the only one saved by the save config call -
        # CFG_SET was used.
        self.assertNotEqual(-1, cfg[0][CONFIG_DB_CONFIG_SET_ID])

        devices_ids = [CFG_DEVICE_1, CFG_DEVICE_2]
        cfg = self.database.list_configuration_sets(devices_ids)
        self.assertEqual(len(cfg), 1)
        config_set = cfg[0]
        self.assertTrue(CONFIG_DB_DEVICE_ID not in config_set)
        self.assertEqual(config_set[CONFIG_DB_NAME], CFG_NAME_2)
        self.assertEqual(config_set[CONFIG_DB_MIN_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual(config_set[CONFIG_DB_MAX_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual(config_set[CONFIG_DB_DIFF_TIMEPOINT], 0.0)
        self.assertEqual(config_set[CONFIG_DB_DESCRIPTION], 'Bla bla')
        self.assertEqual(config_set[CONFIG_DB_USER], 'Bob')
        self.assertEqual(config_set[CONFIG_DB_PRIORITY], 3)

        cfg = self.database.list_configurations(CFG_DEVICE_1)
        # With an empty name part, all the configs for CFG_DEVICE_1 should be
        # retrieved
        self.assertEqual(len(cfg), 2)
        self.assertTrue(cfg[0][CONFIG_DB_NAME] in [CFG_NAME_1, CFG_NAME_2])
        self.assertTrue(cfg[1][CONFIG_DB_NAME] in [CFG_NAME_1, CFG_NAME_2])

        # A name part known to not happen in the data should retrieve nothing.
        cfg = self.database.list_configurations(CFG_DEVICE_1, 'inv part')
        self.assertEqual(cfg, [])

        # We should have stored 2 devices in the database
        devices = self.database.list_devices()
        self.assertIn(CFG_DEVICE_1, devices)
        self.assertIn(CFG_DEVICE_2, devices)
        self.assertEqual(len(devices), 2)

        devices = self.database.list_devices(priority=2)
        self.assertEqual(devices, [])

        devices = self.database.list_devices(priority=1)
        self.assertIn(CFG_DEVICE_1, devices)
        self.assertNotIn(CFG_DEVICE_2, devices)
        self.assertEqual(len(devices), 1)

    def test_save_get_last_config_priority(self):
        """ Checks that saving a set of configs and then retrieving the latest
            for a device with a given priority works.
        """
        self.database.save_configuration(CFG_NAME_1, CFG_SET,
                                         priority=2,
                                         timestamp=TIMESTAMP_2)

        # CFG_SET contains one config for CFG_DEVICE_1 and one for CFG_DEVICE_2
        # in that order. TIMESTAMP_1 is more recent than TIMESTAMP_2.
        self.database.save_configuration(CFG_NAME_2, CFG_SET,
                                         description='Bla bla',
                                         user='Bob',
                                         priority=1,
                                         timestamp=TIMESTAMP_2)

        self.database.save_configuration(CFG_NAME_3, CFG_SET,
                                         description='Bla bla',
                                         user='Bob',
                                         priority=1,
                                         timestamp=TIMESTAMP_1)

        cfg = self.database.get_last_configuration(CFG_DEVICE_1, priority=1)
        # For priority 1, the last config is the one saved under CFG_NAME_3.
        self.assertEqual(len(cfg), 8)
        self.assertEqual(CFG_NAME_3, cfg[CONFIG_DB_NAME])
        self.assertEqual(cfg[CONFIG_DB_TIMEPOINT], TIMESTAMP_1)
        self.assertEqual('Bla bla', cfg[CONFIG_DB_DESCRIPTION])
        self.assertEqual('Bob', cfg[CONFIG_DB_USER])
        self.assertEqual(1, cfg[CONFIG_DB_PRIORITY])
        self.assertEqual(False, cfg[CONFIG_DB_OVERWRITABLE])

        cfg = self.database.get_last_configuration(CFG_DEVICE_1, priority=2)
        # For priority 2, the last config is the one saved under CFG_NAME_1.
        self.assertEqual(len(cfg), 8)
        self.assertEqual(CFG_NAME_1, cfg[CONFIG_DB_NAME])
        self.assertEqual(cfg[CONFIG_DB_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual('', cfg[CONFIG_DB_DESCRIPTION])
        self.assertEqual('.', cfg[CONFIG_DB_USER])
        self.assertEqual(2, cfg[CONFIG_DB_PRIORITY])
        self.assertEqual(False, cfg[CONFIG_DB_OVERWRITABLE])

        cfg = self.database.get_last_configuration(CFG_DEVICE_1, priority=3)
        # For priority 3, there's no config of CFG_DEVICE_1.
        self.assertEqual(cfg, {})

    def test_reject_config_repeat_name(self):
        """ Checks that an attempt to save a configuration with a name that is
            already in use by the same device fails.
        """
        self.database.save_configuration(CFG_NAME_1, CFG_1)
        with self.assertRaises(ConfigurationDBError) as verr:
            self.database.save_configuration(CFG_NAME_1, CFG_SET)
        self.assertTrue(verr.exception.args[0].find(
            'is already taken for at least') > -1)

    def test_config_name_taken(self):
        """ Checks that the is_config_name_taken returns true when there is at
            least one non-overwritable config for any of the devices in the
            list and if there's an overwritable config with the same name, it
            is for the exact same set of devices.
        """
        self.database.save_configuration(CFG_NAME_1, CFG_1)

        self.assertFalse(
            self.database.is_config_name_taken(
                CFG_NAME_1, [CFG_DEVICE_2]
            )
        )
        self.assertTrue(
            self.database.is_config_name_taken(CFG_NAME_1, [CFG_DEVICE_1])
        )
        self.assertTrue(
            self.database.is_config_name_taken(
                CFG_NAME_1,
                [CFG_DEVICE_1, CFG_DEVICE_2])
        )
        self.assertFalse(
            self.database.is_config_name_taken(
                CFG_NAME_2,
                [CFG_DEVICE_1, CFG_DEVICE_2])
        )

        # Checks for equally named overwritable configs for different sets of
        # devices - that condition must also trigger a name already taken
        # error.
        self.database.save_configuration(
            CFG_NAME_3, CFG_SET, overwritable=True)
        with self.assertRaises(ConfigurationDBError) as verr:
            self.database.save_configuration(
                CFG_NAME_3, CFG_1, overwritable=False
            )
        self.assertTrue(verr.exception.args[0].find(
            'is already taken for at least') > -1)
        with self.assertRaises(ConfigurationDBError) as verr:
            self.database.save_configuration(
                CFG_NAME_3, CFG_1, overwritable=True
            )
        self.assertTrue(verr.exception.args[0].find(
            'is already taken for at least') > -1)

    def test_device_conf_limit(self):
        from .. import configuration_database as dbsettings
        dbsettings.CONFIGURATION_LIMIT = 20
        deviceName = "DeviceExceeds"

        config = [create_config(deviceName)]
        for i in range(dbsettings.CONFIGURATION_LIMIT):
            config_name = f"exceed-{i}"
            self.database.save_configuration(config_name, config)

        with self.assertRaises(ConfigurationDBError):
            self.database.save_configuration("exceed-crash", config)

    def test_overwritable_conf(self):
        """ Checks that an overwritable configuration can be overwritten,
            even at the edge of configuration limit per device. Also checks
            that ovewriting a configuration without specifying a timestamp
            updates the timestamp to the current time.
        """
        from .. import configuration_database as dbsettings
        dbsettings.CONFIGURATION_LIMIT = 10
        deviceName = "DeviceExceeds-WithOverwritableConfig"

        config = [create_config(deviceName)]
        for i in range(dbsettings.CONFIGURATION_LIMIT - 1):
            config_name = f"exceed-{i}"
            self.database.save_configuration(config_name, config)

        # Saves an overwritable configuration at the limit.
        overCfg = [create_config(deviceName)]
        overCfgName = f"exceed-over-{dbsettings.CONFIGURATION_LIMIT}"
        self.database.save_configuration(
            overCfgName, overCfg, overwritable=True)

        cfg = self.database.get_configuration(deviceName, overCfgName)
        self.assertEqual(len(cfg), 8)
        self.assertEqual(overCfgName, cfg[CONFIG_DB_NAME])
        self.assertEqual('', cfg[CONFIG_DB_DESCRIPTION])
        self.assertEqual('.', cfg[CONFIG_DB_USER])
        self.assertEqual(1, cfg[CONFIG_DB_PRIORITY])
        self.assertEqual(True, cfg[CONFIG_DB_OVERWRITABLE])
        timeStamp1 = datetime.strptime(cfg[CONFIG_DB_TIMEPOINT],
                                       ISO8601_FORMAT)

        # Ovewrites the configuration at the limit.
        # Overwritable flag must not be updated.
        # Priority, description and timestamp must be updated.
        self.database.save_configuration(
            overCfgName, overCfg, description="BlahBlah",
            priority=3, overwritable=False
        )
        cfg = self.database.get_configuration(deviceName, overCfgName)
        self.assertEqual(len(cfg), 8)
        self.assertEqual(overCfgName, cfg[CONFIG_DB_NAME])
        self.assertEqual('BlahBlah', cfg[CONFIG_DB_DESCRIPTION])
        self.assertEqual('.', cfg[CONFIG_DB_USER])
        self.assertEqual(3, cfg[CONFIG_DB_PRIORITY])
        self.assertEqual(True, cfg[CONFIG_DB_OVERWRITABLE])
        timeStamp2 = datetime.strptime(cfg[CONFIG_DB_TIMEPOINT],
                                       ISO8601_FORMAT)
        self.assertGreater(timeStamp2, timeStamp1)

        # Overwrite the configuration a second time, to be
        # sure that all the device configurations will be properly
        # updated.
        self.database.save_configuration(
            overCfgName, overCfg, description="Bleh-Bleh", priority=2)
        cfg = self.database.get_configuration(deviceName, overCfgName)
        self.assertEqual(len(cfg), 8)
        self.assertEqual(overCfgName, cfg[CONFIG_DB_NAME])
        self.assertEqual('Bleh-Bleh', cfg[CONFIG_DB_DESCRIPTION])
        self.assertEqual('.', cfg[CONFIG_DB_USER])
        self.assertEqual(2, cfg[CONFIG_DB_PRIORITY])
        self.assertEqual(True, cfg[CONFIG_DB_OVERWRITABLE])
        timeStamp3 = datetime.strptime(cfg[CONFIG_DB_TIMEPOINT],
                                       ISO8601_FORMAT)
        self.assertGreater(timeStamp3, timeStamp2)

        # An attempt to save a new configuration, overwritable or not,
        # should fail once the limit is reached.
        with self.assertRaises(ConfigurationDBError):
            self.database.save_configuration(
                "exceed-crash",  overCfg, f"{overCfgName}-limit",
                overwritable=True)

    def test_configSets_minSize(self):
        """Checks that retrieving configuration sets specifying a minSetSize
        that is lower than the size of the original configuration sets allows
        retrieving config sets with a subset of the devices specified. Also
        checks that retrieving configuration sets omitting the minSetSize
        retrieves a superset of the devices specified.
        """
        # Save with priority 1
        self.database.save_configuration(CFG_NAME_1, CFG_1,
                                         user='Alice',
                                         timestamp=TIMESTAMP_1)

        # CFG_SET contains one config for CFG_DEVICE_1 and one for CFG_DEVICE_2
        # in that order.
        self.database.save_configuration(CFG_NAME_2, CFG_SET,
                                         description='Bla bla',
                                         user='Bob',
                                         priority=3,
                                         timestamp=TIMESTAMP_2)

        # CFG_SET_2 contains three configs: one for CFG_DEVICE_1, one for
        # CFG_DEVICE_2 and one for CFG_DEVICE_3.
        self.database.save_configuration(CFG_NAME_3, CFG_SET_2,
                                         user='Charlie')

        # Retrieval of all sets of size at least 1 containing CFG_DEVICE_1
        # should return all three saved sets.
        cfgs = self.database.list_configuration_sets([CFG_DEVICE_1], 1)

        self.assertEqual(len(cfgs), 3)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_1)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Alice')
        # list_configurations_sets returns the config set id even for single
        # configuration sets.
        self.assertNotEqual(cfgs[0][CONFIG_DB_CONFIG_SET_ID], -1)

        self.assertEqual(cfgs[1][CONFIG_DB_NAME], CFG_NAME_2)
        self.assertEqual(cfgs[1][CONFIG_DB_USER], 'Bob')
        self.assertNotEqual(cfgs[1][CONFIG_DB_CONFIG_SET_ID], -1)

        self.assertEqual(cfgs[2][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[2][CONFIG_DB_USER], 'Charlie')
        self.assertNotEqual(cfgs[2][CONFIG_DB_CONFIG_SET_ID], -1)

        # Checks the configs in the config set for CFG_NAME_2
        cfgs_in_set = self.database.list_configurations_in_set(
            cfgs[1][CONFIG_DB_CONFIG_SET_ID])
        self.assertEqual(len(cfgs_in_set), 2)
        # Config set with CFG_NAME_2 has CFG_DEVICE_1 and CFG_DEVICE_2
        devs = {CFG_DEVICE_1, CFG_DEVICE_2}
        self.assertTrue(cfgs_in_set[0][CONFIG_DB_DEVICE_ID] in devs)
        self.assertTrue(cfgs_in_set[1][CONFIG_DB_DEVICE_ID] in devs)
        self.assertEqual(cfgs_in_set[0][CONFIG_DB_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual(cfgs_in_set[1][CONFIG_DB_TIMEPOINT], TIMESTAMP_2)

        # Retrieval of all sets of unspecified size containing CFG_DEVICE_1
        # should return all three saved sets - when the value is not specified,
        # the number of devicesIds in the argument list is used; so this is
        # the same call of the previous case.
        cfgs = self.database.list_configuration_sets([CFG_DEVICE_1])
        self.assertEqual(len(cfgs), 3)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_1)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Alice')
        self.assertEqual(cfgs[1][CONFIG_DB_NAME], CFG_NAME_2)
        self.assertEqual(cfgs[1][CONFIG_DB_USER], 'Bob')
        self.assertEqual(cfgs[2][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[2][CONFIG_DB_USER], 'Charlie')
        # minSetSize=0 or negative mean the same as the default None
        cfgs2 = self.database.list_configuration_sets([CFG_DEVICE_1], 0)
        self.assertEqual(cfgs, cfgs2)
        cfgs2 = self.database.list_configuration_sets([CFG_DEVICE_1], -4)
        self.assertEqual(cfgs, cfgs2)

        # Retrieval of all sets of size at least 2 containing both CFG_DEVICE_1
        # and CFG_DEVICE_2 should return both config sets that contained both
        # devices.
        cfgs = self.database.list_configuration_sets(
            [CFG_DEVICE_1, CFG_DEVICE_2], 2)
        self.assertEqual(len(cfgs), 2)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_2)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Bob')
        self.assertEqual(cfgs[1][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[1][CONFIG_DB_USER], 'Charlie')
        # Retrieval of all sets of size at least 1 containing at least
        # CFG_DEVICE_2 or CFG_DEVICE_3 should return the two multi-device set,
        # one for having CFG_DEVICE_2 and the other for having both devices.
        cfgs = self.database.list_configuration_sets(
            [CFG_DEVICE_2, CFG_DEVICE_3], 1)
        self.assertEqual(len(cfgs), 2)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_2)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Bob')
        self.assertEqual(cfgs[1][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[1][CONFIG_DB_USER], 'Charlie')
        # Retrieval of all sets of size at least 2 containing CFG_DEVICE_1,
        # CFG_DEVICE_2 or CFG_DEVICE_3 should return both config sets that have
        # more than one device.
        cfgs = self.database.list_configuration_sets(
            [CFG_DEVICE_1, CFG_DEVICE_2, CFG_DEVICE_3], 2)
        self.assertEqual(len(cfgs), 2)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_2)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Bob')
        self.assertEqual(cfgs[1][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[1][CONFIG_DB_USER], 'Charlie')
        # Retrieval of all sets of size at least 3 containing CFG_DEVICE_1,
        # CFG_DEVICE_2 and CFG_DEVICE_3 should return just the config set that
        # has the three devices (not specifying minSetSize in the same as
        # specifying len(deviceIds)).
        cfgs = self.database.list_configuration_sets(
            [CFG_DEVICE_1, CFG_DEVICE_2, CFG_DEVICE_3])
        self.assertEqual(len(cfgs), 1)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Charlie')
        # Retrieval of all sets of size at least 1 containing CFG_DEVICE_3
        # should return only the device set that had the device and two
        # more - this is the only device set that has CFG_DEVICE_3.
        cfgs = self.database.list_configuration_sets([CFG_DEVICE_3], 1)
        self.assertEqual(len(cfgs), 1)
        self.assertEqual(cfgs[0][CONFIG_DB_NAME], CFG_NAME_3)
        self.assertEqual(cfgs[0][CONFIG_DB_USER], 'Charlie')
        # Retrieval of all sets of size at least 4 containing CFG_DEVICE_3
        # should throw an exception - minSetSize cannot be greater than the
        # number of devices in the first argument.
        with self.assertRaises(ConfigurationDBError):
            cfgs = self.database.list_configuration_sets([CFG_DEVICE_3], 4)
        # Empty list of devices throws exceptions.
        with self.assertRaises(ConfigurationDBError):
            cfgs = self.database.list_configuration_sets([])

    def test_save_get_configurations_in_set(self):
        """Checks that list configurations for device (independent of name
        part specification) returns information about the config set as part
        of which that configuration has been saved, and that all the configs
        in the set can be easily retrieved by list configuration for
        config set.
        """
        # Creates a config set with a single configuration for CFG_DEVICE_1
        self.database.save_configuration(CFG_NAME_1, CFG_1,
                                         user='Alice',
                                         timestamp=TIMESTAMP_1)

        # CFG_SET contains one config for CFG_DEVICE_1 and one for CFG_DEVICE_2
        # in that order.
        self.database.save_configuration(CFG_NAME_2, CFG_SET,
                                         description='Bla bla',
                                         user='Bob',
                                         priority=3,
                                         timestamp=TIMESTAMP_1)

        # CFG_SET_2 contains three configs: one for CFG_DEVICE_1, one for
        # CFG_DEVICE_2 and one for CFG_DEVICE_3.
        self.database.save_configuration(CFG_NAME_3, CFG_SET_2,
                                         user='Charlie',
                                         timestamp=TIMESTAMP_2)

        # Gets all the configs for CFG_DEVICE_1 - all the three generated
        # config sets have one config for CFG_DEVICE_1.
        dev1_cfgs = self.database.list_configurations(CFG_DEVICE_1)
        self.assertEqual(len(dev1_cfgs), 3)
        # The first CFG_DEVICE_1 config has been saved individually.
        self.assertEqual(dev1_cfgs[0][CONFIG_DB_CONFIG_SET_ID], -1)
        # The other two CFG_DEVICE_1 configs have been saved as part of
        # multi-device config sets
        self.assertNotEqual(dev1_cfgs[1][CONFIG_DB_CONFIG_SET_ID], -1)
        self.assertNotEqual(dev1_cfgs[2][CONFIG_DB_CONFIG_SET_ID], -1)
        # Checks that the configurations in each of the multi-device config
        # sets are correct.
        cfgs = self.database.list_configurations_in_set(
            dev1_cfgs[1][CONFIG_DB_CONFIG_SET_ID])
        self.assertEqual(len(cfgs), 2)
        devs = {CFG_DEVICE_1, CFG_DEVICE_2}
        self.assertTrue(cfgs[0][CONFIG_DB_DEVICE_ID] in devs)
        self.assertTrue(cfgs[1][CONFIG_DB_DEVICE_ID] in devs)
        self.assertEqual(cfgs[0][CONFIG_DB_TIMEPOINT], TIMESTAMP_1)
        self.assertEqual(cfgs[1][CONFIG_DB_TIMEPOINT], TIMESTAMP_1)
        cfgs = self.database.list_configurations_in_set(
            dev1_cfgs[2][CONFIG_DB_CONFIG_SET_ID])
        devs.add(CFG_DEVICE_3)
        self.assertEqual(len(cfgs), 3)
        self.assertTrue(cfgs[0][CONFIG_DB_DEVICE_ID] in devs)
        self.assertTrue(cfgs[1][CONFIG_DB_DEVICE_ID] in devs)
        self.assertTrue(cfgs[2][CONFIG_DB_DEVICE_ID] in devs)
        self.assertEqual(cfgs[0][CONFIG_DB_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual(cfgs[1][CONFIG_DB_TIMEPOINT], TIMESTAMP_2)
        self.assertEqual(cfgs[2][CONFIG_DB_TIMEPOINT], TIMESTAMP_2)


if __name__ == '__main__':
    unittest.main()
