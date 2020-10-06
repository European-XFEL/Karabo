import os
import unittest
import shutil
from karabo.config_db.configuration_database import DbHandle, ConfigurationDatabase
from karabo.config_db.utils import ConfigurationDBError

TEST_DB_PATH = 'cfgMngrTest.db'

CFG_NAME_1 = 'Simple config for test'
CFG_NAME_2 = 'Second config for test'
CFG_NAME_3 = 'Third config for test'

CFG_DEVICE_1 = 'SCS/Cameras/Basle_1'
CFG_DEVICE_2 = 'CAS_LAB/Digitizer/ADQ-12'

TIMESTAMP_1 = '2020-01-30 12:02:02.123'
TIMESTAMP_2 = '2019-01-30 12:02:02.123'

CFG_1 = [
    {
        'deviceId': CFG_DEVICE_1,
        'config': 'JFDHFYE947AGEUFHFSJDPOWIAKLS',
        'schema': 'QREPLCMBab789oeiryJAAHDDBAVDUUWY98==='
    }
]

CFG_SET = [
    {
        'deviceId': CFG_DEVICE_1,
        'config': 'JFDHFYE947AGEUFHFSJDPOWIAKLSIDHD',
        'schema': 'QREPLCMBab789oeiryJAAHDDBAVDUUWY93948==='
    },
    {
        'deviceId': CFG_DEVICE_2,
        'config': 'JFDHFYE947AGEUFHFSJDPOWIAKLSIDHD',
        'schema': 'QREPLCMBab789oeiryJAAHDDBAVDUUWY93948==='
    }
]


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
        cfg = self.database.list_devices_configurations(
            devices_ids, CFG_NAME_2[:4]
        )
        self.assertEqual(len(cfg), 2)
        self.assertTrue(cfg[0]['deviceId'] in [CFG_DEVICE_1, CFG_DEVICE_2])
        self.assertTrue(cfg[1]['deviceId'] in [CFG_DEVICE_1, CFG_DEVICE_2])
        self.assertEqual(CFG_NAME_2, cfg[0]['name'])
        self.assertEqual(cfg[0]['timepoint'], TIMESTAMP_2)
        self.assertEqual('Bla bla', cfg[0]['description'])
        self.assertEqual('Bob', cfg[0]['user'])
        self.assertEqual(3, cfg[0]['priority'])

        cfg = self.database.list_configurations(CFG_DEVICE_1)
        # With an empty name part, all the configs for CFG_DEVICE_1 should be
        # retrieved
        self.assertEqual(len(cfg), 2)
        self.assertTrue(cfg[0]['name'] in [CFG_NAME_1, CFG_NAME_2])
        self.assertTrue(cfg[1]['name'] in [CFG_NAME_1, CFG_NAME_2])

        cfg = self.database.list_devices_configurations(devices_ids)
        # With an empty name part, all the configs for CFG_DEVICE_1 and
        # CFG_DEVICE_2 should be retrieved.
        self.assertEqual(len(cfg), 3)
        self.assertTrue(cfg[0]['name'] in [CFG_NAME_1, CFG_NAME_2])
        self.assertTrue(cfg[1]['name'] in [CFG_NAME_1, CFG_NAME_2])
        self.assertTrue(cfg[2]['name'] in [CFG_NAME_1, CFG_NAME_2])

        # A name part known to not happen in the data should retrieve nothing.
        cfg = self.database.list_configurations(CFG_DEVICE_1, 'inv part')
        self.assertEqual(cfg, [])
        cfg = self.database.list_devices_configurations(
            [CFG_DEVICE_1, CFG_DEVICE_2], 'inv part'
        )
        self.assertEqual(cfg, [])

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


if __name__ == '__main__':
    unittest.main()
