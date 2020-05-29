#from datetime import datetime
#from time import sleep, time

import sys
from copy import copy

from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.bound import (AlarmCondition, Hash, SignalSlotable, State,
                          Timestamp, Validator)
from karathon import fullyEqual

class TestDeviceAlarmApi(BoundDeviceTestCase):
    _max_timeout = 20    # in seconds
    _max_timeout_ms = _max_timeout * 1000


    def test_slotReSubmitAlarms_cpp(self):
        self._test_slotReSubmitAlarms("cpp")

    def test_slotReSubmitAlarms_bound(self):
        self._test_slotReSubmitAlarms("bound")

    def test_slotReSubmitAlarms_mdl(self):
        self._test_slotReSubmitAlarms("mdl")

    def _test_slotReSubmitAlarms(self, api):

        # Start server, device and helper to call slots with arguments
        # (the latter is needed until DeviceClient can call these slots)
        klass = "PropertyTest"
        if api == "mdl":
            klass += "MDL"
        server_id = "server/" + api
        self.start_server(api, server_id, [klass])  #, logLevel="INFO")

        dev_id = "DEV/TO/TEST_ALARM_" + api
        self.start_device(server_id, klass, dev_id, cfg=Hash())

        caller = SignalSlotable("helperAlarmTests")  # for slots with args
        caller.start()

        global_alarm_key = "alarmCondition"
        # Initially all is fine
        alarmCond = self.dc.get(dev_id, global_alarm_key)
        self.assertEqual(alarmCond, AlarmCondition.NONE)

        #########################################################
        # 1) Set property to warn level
        # (implicitly since PropertyTest device transfers reconfig. property
        #  values to their readOnly counterparts)
        #########################################################

        self.dc.set(dev_id, "doubleProperty", 50.)
        # Global alarm should be raised...
        alarmCond = self.dc.get(dev_id, global_alarm_key)
        self.assertEqual(alarmCond, AlarmCondition.WARN)
        # ...and property in configuration carries alarmCondition as well
        attrs = self.dc.get(dev_id).getAttributes("doublePropertyReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "warnHigh")
        stampDouble = Timestamp.fromHashAttributes(attrs)

        # Now see how various calls to slotReSubmitAlarms react
        # 1a) empty input: return the alarm in 'toAdd'
        hashArg = Hash()
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg
                                      ).waitForReply(self._max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        hDoubleAdd = Hash("toClear", Hash(),
                          "toAdd", Hash("doublePropertyReadOnly.warnHigh",
                                        Hash("type", "warnHigh",
                                             "description", "Rather high",
                                             "needsAcknowledging", False)))
        attrs = hDoubleAdd.getAttributes("toAdd.doublePropertyReadOnly.warnHigh")
        stampDouble.toHashAttributes(attrs)
        self.assertTrue(fullyEqual(res, hDoubleAdd, False),
                        str(hDoubleAdd) + '\nvs\n' + str(res))

        # 1b) existing alarm input: return the alarm in 'toAdd' as well
        hashArg = Hash("doublePropertyReadOnly.warnHigh", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg,
                                      ).waitForReply(self._max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        self.assertTrue(fullyEqual(res, hDoubleAdd, False),
                        str(hDoubleAdd) + '\nvs\n' + str(res))

        # 1c) non-existing alarm input:
        # return the existing alarm in 'toAdd' again
        #        and the non-existing in 'toClear'
        hashArg = Hash("int64PropertyReadOnly.alarmLow", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg,
                                      ).waitForReply(self._max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        hDoubleAddInt64Clear = copy(hDoubleAdd)
        hDoubleAddInt64Clear["toClear.int64PropertyReadOnly"] = ["alarmLow"]
        self.assertTrue(fullyEqual(res, hDoubleAddInt64Clear, False),
                        str(hDoubleAddInt64Clear) + '\nvs\n' + str(res))


        #########################################################
        # 2) Now set another property to a higher alarm level
        #########################################################
        self.dc.set(dev_id, "int64Property", -3200000001)
        # Global alarm should be raised...
        alarmCond = self.dc.get(dev_id, global_alarm_key)
        self.assertEqual(alarmCond, AlarmCondition.ALARM)
        # ...and property in configuration carries alarmCondition as well
        attrs = self.dc.get(dev_id).getAttributes("int64PropertyReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "alarmLow")
        stampInt64 = Timestamp.fromHashAttributes(attrs)

        # Now see how various calls to slotReSubmitAlarms react
        # 2a) Whether input empty or containing one or both of the existing
        #     alarms: return both alarms in 'toAdd'
        hDoubleInt64Add = copy(hDoubleAdd)
        tmpKey = "toAdd.int64PropertyReadOnly.alarmLow"
        hDoubleInt64Add[tmpKey] = Hash("type", "alarmLow",
                                  "description", "Too low",
                                  "needsAcknowledging", True)
        attrsInt64 = hDoubleInt64Add.getAttributes(tmpKey)
        stampInt64.toHashAttributes(attrsInt64)

        hashArgs = [Hash(),
                    Hash("doublePropertyReadOnly.warnHigh", Hash()),
                    Hash("int64PropertyReadOnly.alarmLow", Hash()),
                    Hash("doublePropertyReadOnly.warnHigh", Hash(),
                         "int64PropertyReadOnly.alarmLow", Hash())]
        for hashArg in hashArgs:

            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                         ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hDoubleInt64Add) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hDoubleInt64Add, False), msg)

        # 2b) Whether input containing zero, one or both of the existing alarms
        #     and one not existing: return both alarms in 'toAdd' and the
        #     other in 'toClear'
        hDoubleInt64AddInt32Clear = copy(hDoubleInt64Add)
        tmpKey = "toClear.int32PropertyReadOnly"
        hDoubleInt64AddInt32Clear[tmpKey] = ["warnLow"]

        for hashArg in hashArgs:
            hashArg = copy(hashArg)
            hashArg["int32PropertyReadOnly.warnLow"] = Hash()
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                         ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hDoubleInt64AddInt32Clear) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hDoubleInt64AddInt32Clear, False),
                            msg)


        #########################################################
        # 3) Now release the higher alarm
        #########################################################
        self.dc.set(dev_id, "int64Property", 0)
        # Global alarm should be lowered
        alarmCond = self.dc.get(dev_id, global_alarm_key)
        self.assertEqual(alarmCond, AlarmCondition.WARN)

        # 3a) Check again for zero, one or two of the ever active alarms:
        #     double is always in toAdd, int64 is in toClear when in input
        expected = [hDoubleAdd, hDoubleAdd,  # for hashArgs[0&1]
                   hDoubleAddInt64Clear, hDoubleAddInt64Clear]  # hashArgs[2&3]
        for i, hashArg in enumerate(hashArgs):
            hExpected = expected[i]
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                         ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hExpected) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hExpected, False), msg)

        #########################################################
        # 4) Now set global alarm to alarm level
        #########################################################
        self.dc.set(dev_id, "stringProperty", "alarm")
        self.dc.execute(dev_id, "setAlarm")

        # Global alarm should have risen again
        alarmCond = self.dc.get(dev_id, global_alarm_key)
        self.assertEqual(alarmCond, AlarmCondition.ALARM)
        attrs = self.dc.get(dev_id).getAttributes(global_alarm_key)
        stampGlobal = Timestamp.fromHashAttributes(attrs)

        hDoubleGlobalAdd = copy(hDoubleAdd)
        # MDL global alarms have no description
        desc = "" if api == "mdl" else "Acknowledgment requiring alarm"
        tmpKey = "toAdd.global.alarm"
        hDoubleGlobalAdd[tmpKey] = Hash("type", "alarm",
                                        "description", desc,
                                        "needsAcknowledging", True)
        attrs = hDoubleGlobalAdd.getAttributes(tmpKey)
        stampGlobal.toHashAttributes(attrs)

        # 4a) Check for any combination of global and doubleProperty alarms:
        #     Always both are returned
        for hashArg in [Hash(),
                        Hash("global.alarm", Hash()),
                        Hash("doublePropertyReadOnly.warnHigh", Hash()),
                        Hash("global.alarm", Hash(),
                             "doublePropertyReadOnly.warnHigh", Hash())]:
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                         ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hDoubleGlobalAdd) + '\nvs\n' + str(res)
            ok = fullyEqual(res, hDoubleGlobalAdd, False)
            if not ok and api == "mdl":
                # FIXME: For MDL, the Epochstamp of the 'global' alarm is not
                #        occasionally wrong! For now, check at least the rest.
                print("\nFix MDL problem of timestamp for global!\n",
                      file=sys.stderr)  # stdout is swallowed in tests
                # remove epochstamp from result...
                attrs = res.getAttributes(tmpKey)
                attrs.erase("sec")
                attrs.erase("frac")
                # ... and from object to be compared
                hDoubleGlobalAdd_copy = copy(hDoubleGlobalAdd)
                attrs = hDoubleGlobalAdd_copy.getAttributes(tmpKey)
                attrs.erase("sec")
                attrs.erase("frac")
                ok = fullyEqual(res, hDoubleGlobalAdd_copy, False)
            self.assertTrue(ok, msg)


        #########################################################
        # 5) Now set property under node to warn (needs acknowledging)
        #########################################################
        self.dc.set(dev_id, "node.counter", 2000000)  # 2.e6: ==> warnHigh

        attrs = self.dc.get(dev_id).getAttributes("node.counterReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "warnHigh")
        stampCounter = Timestamp.fromHashAttributes(attrs)
        hDoubleGlobalCounterAdd = copy(hDoubleGlobalAdd)
        sp = Validator.kAlarmParamPathSeparator
        tmpKey = "toAdd.node" + sp + "counterReadOnly.warnHigh"
        hDoubleGlobalCounterAdd[tmpKey] = Hash("type", "warnHigh",
                                               "description", "Rather high",
                                               "needsAcknowledging", True)
        attrs = hDoubleGlobalCounterAdd.getAttributes(tmpKey)
        stampCounter.toHashAttributes(attrs)

        # 5a) Test all combination of existing alarms as input
        hashArgs = [Hash(),
                    Hash("global.alarm", Hash()),
                    Hash("doublePropertyReadOnly.warnHigh", Hash()),
                    Hash("global.alarm", Hash(),
                         "doublePropertyReadOnly.warnHigh", Hash()),
                    Hash("node" + sp + "counterReadOnly.warnHigh", Hash()),
                    Hash("global.alarm", Hash(),
                         "node" + sp + "counterReadOnly.warnHigh", Hash()),
                    Hash("doublePropertyReadOnly.warnHigh", Hash(),
                         "node" + sp + "counterReadOnly.warnHigh", Hash()),
                    Hash("global.alarm", Hash(),
                         "doublePropertyReadOnly.warnHigh", Hash(),
                         "node" + sp + "counterReadOnly.warnHigh", Hash())]
        for hashArg in hashArgs:
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg,
                                         ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id)
            msg = str(hashArg) + ": "
            msg += str(hDoubleGlobalCounterAdd) + '\nvs\n' + str(res)
            ok = fullyEqual(res, hDoubleGlobalCounterAdd, False)
            self.assertTrue(ok, msg)

        #########################################################
        # 6) Increase to alarm level for property under node.
        #    Also remove global alarm (to avoid the mdl timestamp problem).
        #########################################################
        self.dc.set(dev_id, "node.counter", 200000000)  # 2.e8: ==> alarmHigh
        self.dc.set(dev_id, "stringProperty", "none")
        self.dc.execute(dev_id, "setAlarm")

        attrs = self.dc.get(dev_id).getAttributes("node.counterReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "alarmHigh")
        stampCounter = Timestamp.fromHashAttributes(attrs)
        # Now assemble expected results, depending on slot arguments:
        # If previous alarm on property in node is not in, the result has
        # only the new alarm in 'toAdd' (==> hDoubleCounter2Add).
        # If we ask for the previous, it will be in 'toClear'
        # (==> hDoubleAddCounterAddClear)
        hDoubleCounter2Add = copy(hDoubleGlobalCounterAdd)
        del hDoubleCounter2Add["toAdd.global"]
        tmpKey = "toAdd.node" + sp + "counterReadOnly"
        del hDoubleCounter2Add[tmpKey + ".warnHigh"]
        tmpKey += ".alarmHigh"
        hDoubleCounter2Add[tmpKey] = Hash("type", "alarmHigh",
                                          "description", "Too high",
                                          "needsAcknowledging", False)
        attrs = hDoubleCounter2Add.getAttributes(tmpKey)
        stampCounter.toHashAttributes(attrs)

        hDoubleAddCounterAddClear = copy(hDoubleCounter2Add)
        tmpKey = "toClear.node" + sp + "counterReadOnly"
        hDoubleAddCounterAddClear[tmpKey] = ["warnHigh"]


        # 6a) Check again for zero, one or two of previously active
        #     alarms: If asking for node.counter.warnHigh, this is in 'toClear'
        hashArgs = [Hash(),
                    Hash("doublePropertyReadOnly.warnHigh", Hash()),
                    Hash("node" + sp + "counterReadOnly.warnHigh", Hash()),
                    Hash("doublePropertyReadOnly.warnHigh", Hash(),
                         "node" + sp + "counterReadOnly.warnHigh", Hash())]
        expected = [hDoubleCounter2Add, hDoubleCounter2Add,
                    hDoubleAddCounterAddClear, hDoubleAddCounterAddClear]
        for i, hashArg in enumerate(hashArgs):
            hExpected = expected[i]
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                         ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))
            msg = str(hashArg) + ":\n"
            msg += str(hExpected) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hExpected, False), msg)

        #########################################################
        # 7) Reset everything and test a global alarm that does
        #    NOT require ackowledgement
        #########################################################
        self.dc.set(dev_id, "node.counter", 2)
        self.dc.set(dev_id, "doubleProperty", -2.)
        alarmCond = self.dc.get(dev_id, global_alarm_key)
        self.assertEqual(alarmCond, AlarmCondition.NONE)
        if api != "mdl":
            # MDL does not know global alarms that do NOT require acknowledging
            self.dc.set(dev_id, "stringProperty", "warn")
            self.dc.execute(dev_id, "setAlarmNoNeedAck")
            attrs = self.dc.get(dev_id).getAttributes(global_alarm_key)
            stamp = Timestamp.fromHashAttributes(attrs)
            tmpKey = 'toAdd.global.warn'
            txt = "No acknowledgment requiring alarm"
            hExpected = Hash("toClear.global", ["alarm"],
                             tmpKey, Hash("type", "warn",
                                          "description", txt,
                                          "needsAcknowledging", False))
            attrs = hExpected.getAttributes(tmpKey)
            stamp.toHashAttributes(attrs)
            hashArg = Hash("global.alarm", Hash())
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                          ).waitForReply(self._max_timeout_ms)
            self.assertEqual(an_id, dev_id)
            msg = str(hExpected) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hExpected, False), msg)

    def test_signalAlarmUpdate(self):
        # TODO!
        pass

    def start_device(self, server_id, class_id, dev_id, cfg=Hash()):
        """
        Start device with cfg on server
        """
        cfg.set("deviceId", dev_id)

        ok, msg = self.dc.instantiate(server_id, class_id, cfg,
                                      self._max_timeout)
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(class_id, server_id, msg))

    def waitUntilEqual(self, devId, propertyName, whatItShouldBe, timeout):
        """
        Wait until property 'propertyName' of device 'deviceId' equals
        'whatItShouldBe'.
        Try up to 'timeOut' seconds and wait 0.5 seconds between each try.
        """
        start = datetime.now()
        while (datetime.now() - start).seconds < timeout:
            res = None
            try:
                res = self.dc.get(devId, propertyName)
            except RuntimeError as re:
                print("Problem retrieving property value: {}".format(re))
            if res == whatItShouldBe:
                return True
            else:
                sleep(.5)
        return False
