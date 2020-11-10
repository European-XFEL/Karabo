import os
import unittest
from karabo.config_db.configuration_database import (
    DbHandle, ConfigurationDatabase)
from karabo.config_db.utils import ConfigurationDBError

TEST_DB_PATH = 'configManagerTest.db'

CFG_NAME_1 = 'FirstConfig'
CFG_NAME_2 = 'SecondConfig'
CFG_NAME_3 = 'ThirdConfig'

CFG_DEVICE_1 = 'SCS/CAM/BASLER'
CFG_DEVICE_2 = 'CAS_LAB/DIGI/ADQ-12'

TIMESTAMP_1 = '2020-01-30 12:02:02.123'
TIMESTAMP_2 = '2019-01-30 12:02:02.123'


def create_config(deviceId):
    return {'deviceId': deviceId,
            'config': 'JFDHFYE947AGEUFHFSJDPOWIAKLSIDHD',
            'schema': 'QREPLCMBab789oeiryJAAHDDBAVDUUWY93948==='}


CFG_1 = [create_config(CFG_DEVICE_1)]
CFG_2 = [create_config(CFG_DEVICE_2)]

CFG_SET = [create_config(CFG_DEVICE_1), create_config(CFG_DEVICE_2)]


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
        self.assertEqual(CFG_1[0]['config'], cfg['config'])
        self.assertEqual(CFG_1[0]['schema'], cfg['schema'])
        self.assertEqual('', cfg['description'])  # Default empty description
        self.assertEqual('.', cfg['user'])  # Default anonymous user
        self.assertEqual(1, cfg['priority'])  # Default priority, 1

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
        self.assertEqual(CFG_NAME_2, cfg[0]['name'])
        self.assertEqual(cfg[0]['timepoint'], TIMESTAMP_2)
        self.assertEqual('Bla bla', cfg[0]['description'])
        self.assertEqual('Bob', cfg[0]['user'])
        self.assertEqual(3, cfg[0]['priority'])

        devices_ids = [CFG_DEVICE_1, CFG_DEVICE_2]
        cfg = self.database.list_configuration_sets(devices_ids)
        self.assertEqual(len(cfg), 1)
        config_set = cfg[0]
        self.assertTrue('deviceId' not in config_set)
        self.assertEqual(config_set['name'], CFG_NAME_2)
        self.assertEqual(config_set['min_timepoint'], TIMESTAMP_2)
        self.assertEqual(config_set['max_timepoint'], TIMESTAMP_2)
        self.assertEqual(config_set['diff_timepoint'], 0.0)
        self.assertEqual(config_set['description'], 'Bla bla')
        self.assertEqual(config_set['user'], 'Bob')
        self.assertEqual(config_set['priority'], 3)

        cfg = self.database.list_configurations(CFG_DEVICE_1)
        # With an empty name part, all the configs for CFG_DEVICE_1 should be
        # retrieved
        self.assertEqual(len(cfg), 2)
        self.assertTrue(cfg[0]['name'] in [CFG_NAME_1, CFG_NAME_2])
        self.assertTrue(cfg[1]['name'] in [CFG_NAME_1, CFG_NAME_2])

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
        self.assertEqual(len(cfg), 7)
        self.assertEqual(CFG_NAME_3, cfg['name'])
        self.assertEqual(cfg['timepoint'], TIMESTAMP_1)
        self.assertEqual('Bla bla', cfg['description'])
        self.assertEqual('Bob', cfg['user'])
        self.assertEqual(1, cfg['priority'])

        cfg = self.database.get_last_configuration(CFG_DEVICE_1, priority=2)
        # For priority 2, the last config is the one saved under CFG_NAME_1.
        self.assertEqual(len(cfg), 7)
        self.assertEqual(CFG_NAME_1, cfg['name'])
        self.assertEqual(cfg['timepoint'], TIMESTAMP_2)
        self.assertEqual('', cfg['description'])
        self.assertEqual('.', cfg['user'])
        self.assertEqual(2, cfg['priority'])

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
            'already has a configuration named') > -1)

    def test_config_name_taken(self):
        """ Checks that the is_config_name_taken returns false when there's no
            no config with a given name for any device in the list and true
            when at least one of the devices in the list has a config with the
            given name.
        """
        self.database.save_configuration(CFG_NAME_1, CFG_1)
        self.assertFalse(
            self.database.is_config_name_taken(CFG_NAME_1, [CFG_DEVICE_2])
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

    def test_device_conf_limit(self):
        import karabo.config_db.configuration_database as dbsettings
        dbsettings.CONFIGURATION_LIMIT = 20
        deviceName = "DeviceExceeds"

        config = [create_config(deviceName)]
        for i in range(dbsettings.CONFIGURATION_LIMIT):
            config_name = f"exceed-{i}"
            self.database.save_configuration(config_name, config)

        with self.assertRaises(ConfigurationDBError):
            self.database.save_configuration(f"exceed-crash", config)


if __name__ == '__main__':
    unittest.main()
