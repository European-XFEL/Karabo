import os
import time
import unittest

from karabo.integration_tests.utils import BoundDeviceTestCase

from karabo.bound import (Hash, SignalSlotable)


def createTestGroupHashes():
    # Hash for an empty group
    config = Hash()
    config.set("group.id", "EMPTY_GROUP")
    config.set("owner.name", "CAS_TestUser")

    config_empty = Hash(
        "classId", "RunConfigurationGroup",
        "deviceId", "TEST/EMPTY_GROUP",
        "configuration", config)

    # Hash for Group1 with two devices
    d0 = Hash("source", "SASE0/GEN_0",
              "type", "control",
              "behavior", "record-all")
    d1 = Hash("source", "SASE0/GEN_1",
              "type", "control",
              "behavior", "record-all")
    d2 = Hash("source", "SASE0/CAM_1:output",
              "type", "instrument",
              "behavior", "record-all")

    config = Hash(
        "group.id", "TEST_GROUP_1",
        "owner.name", "CAS_TestUser",
        "group.expert", [d0, d1, d2])

    config_group1 = Hash(
        "classId", "RunConfigurationGroup",
        "deviceId", "TEST/DAQ_TEST_1_GROUP",
        "configuration", config)

    return [config_empty, config_group1]


def createMoreGroupHashes():
    # Hash for an empty group
    # Hash for Group1 with three devices
    d0 = Hash("source", "SASE0/DOOCS",
              "type", "control",
              "behavior", "record-all")
    d1 = Hash("source", "SASE0/MONO_1",
              "type", "control",
              "behavior", "record-all")
    d2 = Hash("source", "SASE0/XGM_1:output",
              "type", "instrument",
              "behavior", "record-all")
    d3 = Hash("source", "SASE0/INULL_1:output",
              "type", "instrument",
              "behavior", "record-all")
    config = Hash(
        "group.id", "TEST_GROUP_V2",
        "owner.name", "CAS_TestUser",
        "group.expert", [d0, d1, d2, d3])

    config_group2 = Hash(
        "classId", "RunConfigurationGroup",
        "deviceId", "TEST/DAQ_TEST_2_GROUP",
        "configuration", config)

    return [config_group2]


emitted_hash = None
hash_updated = False


def signalSaveHandler(data):
    global emitted_hash
    global hash_updated
    # We need a deep copy here (otherwise it segfaults)
    emitted_hash = Hash(data)
    hash_updated = True
    # print("Hash updated")
    # Simple assignment just segfaults
    # emitted_hash = data


class TestRunConfigurator(BoundDeviceTestCase):

    def setUp(self):
        super(TestRunConfigurator, self).setUp()

        # Start the test server
        print("###################################\nCreating server\n")
        own_dir = str(os.path.dirname(os.path.abspath(__file__)))
        class_ids = ['RunConfigurationGroup', 'RunConfigurator']
        self.start_server("bound", "testServerRunConfig", class_ids,
                          plugin_dir=own_dir)

        # Creating the signal handler
        self.ss = SignalSlotable.create("TestSignalSlot")
        self.ss.start()
        self.ss.registerSlot(signalSaveHandler)
        self.ss.connect("TEST/DAQ_CONFIGURATOR", "signalRunConfiguration",
                        "", "signalSaveHandler")

    def tearDown(self):
        self.ss = None
        print("###################################\n"
              "Finished integration test for RunConfigurator\n")
        super(TestRunConfigurator, self).tearDown()

    def test_run_configurator(self):
        global hash_updated
        global emitted_hash

        ####################################################################
        # We instantiate devices here to avoid teardown problems
        ####################################################################
        print("###################################\nAdding devices\n")
        # Adding the RunConfigurator to the server
        res = self.dc.instantiate("testServerRunConfig", Hash(
            "classId", "RunConfigurator",
            "deviceId", "TEST/DAQ_CONFIGURATOR",
            "configuration", Hash()), timeoutInSeconds=30)
        assert res[0], res[1]

        # Adding groups to the server
        hashes = createTestGroupHashes()
        for hh in hashes:
            res = self.dc.instantiate("testServerRunConfig",
                                      hh,
                                      timeoutInSeconds=30)
            assert res[0], res[1]

        ###################################################################
        # Here we test an empty configuration (no groups are selected)
        ###################################################################
        print("\nRC iTest: Testing empty selection from the configurator\n")
        # Now the configuration should have two groups, none selected
        config = self.dc.get("TEST/DAQ_CONFIGURATOR", "configurations")
        msg = "The expected number of groups is 2, found %d" % len(config)
        self.assertEqual(len(config), 2, msg=msg)

        # buildConfigurationInUse should emmit an empty string
        # It will also crash the DAQ so we don't reply
        emitted_hash = None
        self.ss.request("TEST/DAQ_CONFIGURATOR", "buildConfigurationInUse")\
            .waitForReply(1000)

        msg = "RunConfigurator should not emmit an empty hash, but it " \
              "sent:\n%s" % str(emitted_hash)
        self.assertIsNone(emitted_hash, msg)

        ###################################################################
        # Here we tick in some groups an check the config
        ###################################################################
        print("\nRC iTest: Testing if the configurator's signal has the "
              "correct length\n")
        # Set the group to use (this equals ticking the checkbox)
        config = self.dc.get("TEST/DAQ_CONFIGURATOR", "configurations")
        config[0].set('_RunConfiguratorGroup.use', True)
        config[1].set('_RunConfiguratorGroup.use', True)
        self.dc.set("TEST/DAQ_CONFIGURATOR", "configurations", config)

        # There shpould be two devices in the configuration
        self.ss.request("TEST/DAQ_CONFIGURATOR", "buildConfigurationInUse")\
            .waitForReply(1000)
        msg = "Expected 3 devices in configuration, found %d." \
              % len(emitted_hash.get('configuration'))
        self.assertEqual(len(emitted_hash.get('configuration')), 3, msg=msg)

        ###################################################################
        # Here we test if the signal's internal settings are correct
        ###################################################################
        print("\nRC iTest: Testing the emitted signal\n")
        signal = emitted_hash.get('configuration')
        self.assertEqual(signal.getAttribute("SASE0/GEN_0", 'pipeline'), 0,
                         msg="Control device pipeline should be 0.")
        self.assertEqual(signal.getAttribute("SASE0/GEN_0", 'expertData'), 1)
        self.assertEqual(signal.getAttribute("SASE0/GEN_0", 'userData'), 0)
        self.assertEqual(
            signal.getAttribute("SASE0/CAM_1:output", 'pipeline'), 1,
            msg="Camera device pipeline was not set to 1.")
        self.assertEqual(
            signal.getAttribute("SASE0/CAM_1:output", 'expertData'), 1)
        self.assertEqual(
            signal.getAttribute("SASE0/CAM_1:output", 'userData'), 0)

        ###################################################################
        # Here we test adding another group
        ###################################################################
        print("\nRC iTest: Testing if a new group gets detected by the "
              "RunConfigurator\n")
        res = self.dc.instantiate("testServerRunConfig",
                                  createMoreGroupHashes()[0],
                                  timeoutInSeconds=30)
        assert res[0], res[1]

        # The RunConfigurator should detect the new group
        config = self.dc.get("TEST/DAQ_CONFIGURATOR", "configurations")
        msg = "The expected number of groups is 3, found %d" % len(config)
        self.assertEqual(len(config), 3, msg)

        ###################################################################
        # Here we test updating a group (this was bugged before)
        ###################################################################
        print("\nRC iTest: Testing if group updates appear in the "
              "RunConfigurator\n")
        new_device = [Hash("source", "SASE0/SHUTTER",
                           "type", "control",
                           "behavior", "record-all")]
        self.dc.set("TEST/DAQ_TEST_2_GROUP", "group.user", new_device)

        # First we check if it appears in the configuration
        found_update = False
        tries_left = 20
        while tries_left > 0 and not found_update:
            tries_left -= 1
            time.sleep(0.2)
            config = self.dc.get("TEST/DAQ_CONFIGURATOR", "configurations")
            for group in config:
                if group.get('_RunConfiguratorGroup.groupId') \
                        == "TEST_GROUP_V2":
                    for it in group.get('_RunConfiguratorGroup.sources'):
                        if it.get('source') == "SASE0/SHUTTER":
                            found_update = True
        self.assertTrue(found_update,
                        "Group update is not visible in the configurator.")

        # Then we first tick in the use for all groups
        config = self.dc.get("TEST/DAQ_CONFIGURATOR", "configurations")
        for it in config:
            it.set('_RunConfiguratorGroup.use', True)
        self.dc.set("TEST/DAQ_CONFIGURATOR", "configurations", config)

        # Then in the emitted signal we also look for the new device
        self.ss.request("TEST/DAQ_CONFIGURATOR", "buildConfigurationInUse")\
            .waitForReply(1000)
        found_update = False
        for key in emitted_hash.get('configuration'):
            if str(key) == "SASE0/SHUTTER":
                found_update = True
        self.assertTrue(found_update,
                        "Group update is not visible in the emitted "
                        "configuration signal.")


if __name__ == '__main__':
    unittest.main()
