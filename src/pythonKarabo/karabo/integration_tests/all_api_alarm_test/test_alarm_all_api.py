from copy import copy
from time import sleep

from karabo.bound import (
    AlarmCondition, Hash, SignalSlotable, Timestamp, Validator, fullyEqual)
from karabo.integration_tests.utils import BoundDeviceTestCase

max_timeout = 20    # in seconds
max_timeout_ms = max_timeout * 1000
global_alarm_key = "alarmCondition"


def getPropsAndAlarm(caller, dev_id):
    """
    Provide configuration and current global alarm condition
    of 'dev_id' by calling slotGetConfiguration via 'caller'
    """
    requestor = caller.request(dev_id, "slotGetConfiguration")
    dev_props, _ = requestor.waitForReply(max_timeout_ms)

    # The property Hash contains the alarm condition as string, so convert
    alarm_cond = AlarmCondition(dev_props.get(global_alarm_key))

    return dev_props, alarm_cond


def reconfigure(caller, dev_id, cfg):
    """
    Reconfigure 'dev_id' by calling slotReconfigure with 'cfg' via 'caller'
    """
    caller.request(dev_id, "slotReconfigure", cfg).waitForReply(max_timeout_ms)


class TestDeviceAlarmApi(BoundDeviceTestCase):

    def test_alarms_cpp(self):
        self._test_alarms("cpp")

    def test_alarms_bound(self):
        self._test_alarms("bound")

    def test_alarms_mdl(self):
        self._test_alarms("mdl")

    def _test_alarms(self, api):

        dev_id = self._initialize_device(api)

        # Helper to
        # - call slots with arguments
        # - add a slot and connect it to a remote signal
        # - and have it ensured that a signal triggered by a slot call
        #   is received before the slot reply
        # Note: Use API specific name since we cannot be sure when Python
        #       calls the destructor of the underlying C++ SignalSlotable
        #       of the incarnation for the previously tested API.
        caller = SignalSlotable("helperAlarmTests_" + api)
        caller.start()
        signal_id = signal_payload = None

        def slotAlarmUpdates(the_id, payload):
            nonlocal signal_id, signal_payload
            signal_id = the_id
            signal_payload = copy(payload)  # why copy? (else crash...)
        caller.registerSlot(slotAlarmUpdates)

        self.assertTrue(caller.connect(dev_id, "signalAlarmUpdate",
                                       "", "slotAlarmUpdates"))

        # Initially all is fine
        _, alarmCond = getPropsAndAlarm(caller, dev_id)
        self.assertEqual(alarmCond, AlarmCondition.NONE)

        #########################################################
        # 1) Set property to warn level
        # (implicitly since PropertyTest device transfers reconfig. property
        #  values to their readOnly counterparts)
        #########################################################

        reconfigure(caller, dev_id, Hash("doubleProperty", 50.))

        # Check that the signalAlarmUpdate arrived as expected
        # Its signal_id is known, the signal_payload needs to be "assembled".
        # HACK: Due to ordering, alarm signal should have arrived before
        #       the reconfigure slot reply. But the immediate 'assertEqual'
        #       sometimes fails - get info whether it would work "later".
        waitTime = 1.
        while signal_id != dev_id and waitTime > 0.:
            waitTime -= 0.05
            sleep(0.05)

        # HACK end
        self.assertEqual(signal_id, dev_id, f"waited {1 - waitTime} seconds")
        # HACK part: For now still fail if waiting was needed
        self.assertEqual(1., waitTime)

        # Global alarm should be raised...
        devProps, alarmCond = getPropsAndAlarm(caller, dev_id)
        self.assertEqual(alarmCond, AlarmCondition.WARN)
        # ...and property in configuration carries alarmCondition as well
        attrs = devProps.getAttributes("doublePropertyReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "warnHigh")
        stampDouble = Timestamp.fromHashAttributes(attrs)
        hDoubleAdd = Hash("toClear", Hash(),
                          "toAdd", Hash("doublePropertyReadOnly.warnHigh",
                                        Hash("type", "warnHigh",
                                             "description", "Rather high",
                                             "needsAcknowledging", False)))
        attrs = hDoubleAdd.getAttributes("toAdd.doublePropertyReadOnly"
                                         ".warnHigh")
        stampDouble.toHashAttributes(attrs)

        # Now we can check the signal_payload
        self.assertTrue(fullyEqual(signal_payload, hDoubleAdd, False),
                        str(hDoubleAdd) + '\nvs\n' + str(signal_payload))

        # Now see how various calls to slotReSubmitAlarms react
        # 1a) empty input: return the alarm in 'toAdd'
        hashArg = Hash()
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        self.assertTrue(fullyEqual(res, hDoubleAdd, False),
                        str(hDoubleAdd) + '\nvs\n' + str(res))

        # 1b) existing alarm input: return the alarm in 'toAdd' as well
        hashArg = Hash("doublePropertyReadOnly.warnHigh", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg,
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        self.assertTrue(fullyEqual(res, hDoubleAdd, False),
                        str(hDoubleAdd) + '\nvs\n' + str(res))

        # 1c) non-existing alarm input:
        # return the existing alarm in 'toAdd' again
        #        and the non-existing in 'toClear'
        hashArg = Hash("int64PropertyReadOnly.alarmLow", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg,
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        hDoubleAddInt64Clear = copy(hDoubleAdd)
        hInt64Clear = Hash("toClear.int64PropertyReadOnly", ["alarmLow"],
                           "toAdd", Hash())
        hDoubleAddInt64Clear.merge(hInt64Clear)
        self.assertTrue(fullyEqual(res, hDoubleAddInt64Clear, False),
                        str(hDoubleAddInt64Clear) + '\nvs\n' + str(res))

        #########################################################
        # 2) Now set another property to a higher alarm level
        #########################################################
        signal_id = signal_payload = None
        reconfigure(caller, dev_id, Hash("int64Property", -3_200_000_001))
        # Check that the signalAlarmUpdate arrived as expected
        self.assertEqual(signal_id, dev_id)

        # Global alarm should be raised...
        devProps, alarmCond = getPropsAndAlarm(caller, dev_id)
        self.assertEqual(alarmCond, AlarmCondition.ALARM)
        # ...and property in configuration carries alarmCondition as well
        attrs = devProps.getAttributes("int64PropertyReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "alarmLow")
        stampInt64 = Timestamp.fromHashAttributes(attrs)
        tmpKey = "toAdd.int64PropertyReadOnly.alarmLow"
        hInt64Add = Hash(tmpKey, Hash("type", "alarmLow",
                                      "description", "Too low",
                                      "needsAcknowledging", True),
                         "toClear", Hash())
        attrsInt64 = hInt64Add.getAttributes(tmpKey)
        stampInt64.toHashAttributes(attrsInt64)

        # Now we can check the signal_payload for int64PropertyReadOnly
        self.assertTrue(fullyEqual(signal_payload, hInt64Add, False),
                        str(hInt64Add) + '\nvs\n' + str(signal_payload))

        hDoubleInt64Add = copy(hDoubleAdd)
        hDoubleInt64Add.merge(hInt64Add)

        # Now see how various calls to slotReSubmitAlarms react
        # 2a) Whether input empty or containing one or both of the existing
        #     alarms: return both alarms in 'toAdd'
        hashArgs = [Hash(),
                    Hash("doublePropertyReadOnly.warnHigh", Hash()),
                    Hash("int64PropertyReadOnly.alarmLow", Hash()),
                    Hash("doublePropertyReadOnly.warnHigh", Hash(),
                         "int64PropertyReadOnly.alarmLow", Hash())]
        for hashArg in hashArgs:

            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                          ).waitForReply(max_timeout_ms)
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
                                          ).waitForReply(max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hDoubleInt64AddInt32Clear) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hDoubleInt64AddInt32Clear, False),
                            msg)

        #########################################################
        # 3) Now release the higher alarm
        #########################################################
        signal_id = signal_payload = None
        reconfigure(caller, dev_id, Hash("int64Property", 0))
        # Check that the signalAlarmUpdate arrived as expected
        self.assertEqual(signal_id, dev_id)

        # Global alarm should be lowered
        _, alarmCond = getPropsAndAlarm(caller, dev_id)
        self.assertEqual(alarmCond, AlarmCondition.WARN)

        # Check that the signalAlarmUpdate cleared int64Property
        self.assertTrue(fullyEqual(signal_payload, hInt64Clear, False),
                        str(hInt64Clear) + '\nvs\n' + str(signal_payload))

        # 3a) Check again for zero, one or two of the ever active alarms:
        #     double is always in toAdd, int64 is in toClear when in input
        expected = [hDoubleAdd, hDoubleAdd,  # for hashArgs[0&1]
                    hDoubleAddInt64Clear, hDoubleAddInt64Clear]  # hashArg[2&3]
        for i, hashArg in enumerate(hashArgs):
            hExpected = expected[i]
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                          ).waitForReply(max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hExpected) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hExpected, False), msg)

        #########################################################
        # 4) Now set global alarm to alarm level
        #########################################################
        signal_id = signal_payload = None
        reconfigure(caller, dev_id, Hash("stringProperty", "alarm"))
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)

        # Check that the signalAlarmUpdate arrived
        self.assertEqual(signal_id, dev_id)

        # Global alarm should have risen again
        devProps, alarmCond = getPropsAndAlarm(caller, dev_id)
        self.assertEqual(alarmCond, AlarmCondition.ALARM)
        attrs = devProps.getAttributes(global_alarm_key)
        stampGlobal = Timestamp.fromHashAttributes(attrs)

        # MDL global alarms have no description
        desc = "" if api == "mdl" else "Acknowledgment requiring alarm"
        tmpKey = "toAdd.global.alarm"
        hGlobalAdd = Hash(tmpKey, Hash("type", "alarm",
                                       "description", desc,
                                       "needsAcknowledging", True),
                          "toClear", Hash())
        attrs = hGlobalAdd.getAttributes(tmpKey)
        stampGlobal.toHashAttributes(attrs)

        # Check that the signalAlarmUpdate indeed added global
        self.assertTrue(fullyEqual(signal_payload, hGlobalAdd, False),
                        str(hGlobalAdd) + '\nvs\n' + str(signal_payload))

        hDoubleGlobalAdd = copy(hDoubleAdd)
        hDoubleGlobalAdd.merge(hGlobalAdd)

        # 4a) Check for any combination of global and doubleProperty alarms:
        #     Always both are returned
        for hashArg in [Hash(),
                        Hash("global.alarm", Hash()),
                        Hash("doublePropertyReadOnly.warnHigh", Hash()),
                        Hash("global.alarm", Hash(),
                             "doublePropertyReadOnly.warnHigh", Hash())]:
            (an_id, res) = caller.request(dev_id,
                                          "slotReSubmitAlarms", hashArg
                                          ).waitForReply(max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))

            msg = str(hashArg) + ":\n"
            msg += str(hDoubleGlobalAdd) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hDoubleGlobalAdd, False), msg)

        #########################################################
        # 5) Now set property under node to warn (needs acknowledging)
        #########################################################
        signal_id = signal_payload = None
        reconfigure(caller, dev_id,
                    Hash("node.counter", 2_000_000))  # ==> warnHigh
        # Check that the signalAlarmUpdate arrived
        self.assertEqual(signal_id, dev_id)

        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes("node.counterReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "warnHigh")
        stampCounter = Timestamp.fromHashAttributes(attrs)
        sp = Validator.kAlarmParamPathSeparator
        tmpKey = "toAdd.node" + sp + "counterReadOnly.warnHigh"
        hCounterAdd = Hash(tmpKey, Hash("type", "warnHigh",
                                        "description", "Rather high",
                                        "needsAcknowledging", True),
                           "toClear", Hash())
        attrs = hCounterAdd.getAttributes(tmpKey)
        stampCounter.toHashAttributes(attrs)

        # Check that the signalAlarmUpdate indeed added counterReadOnly
        self.assertTrue(fullyEqual(signal_payload, hCounterAdd, False),
                        str(hCounterAdd) + '\nvs\n' + str(signal_payload))

        hDoubleGlobalCounterAdd = copy(hDoubleGlobalAdd)
        hDoubleGlobalCounterAdd.merge(hCounterAdd)

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
                                          ).waitForReply(max_timeout_ms)
            self.assertEqual(an_id, dev_id)
            msg = str(hashArg) + ": "
            msg += str(hDoubleGlobalCounterAdd) + '\nvs\n' + str(res)
            ok = fullyEqual(res, hDoubleGlobalCounterAdd, False)
            self.assertTrue(ok, msg)

        #########################################################
        # 6) Increase to alarm level for property under node.
        #    Also remove global alarm (to avoid the mdl timestamp problem).
        #########################################################
        signal_id = signal_payload = None
        reconfigure(caller, dev_id,
                    Hash("node.counter", 200_000_000))  # ==> alarmHigh

        # Check that the signalAlarmUpdate arrived
        self.assertEqual(signal_id, dev_id)

        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes("node.counterReadOnly")
        if api != "mdl":
            # FIXME: MDL does not send alarm condition as attribute
            self.assertEqual(attrs.get("alarmCondition"), "alarmHigh")
        stampCounter = Timestamp.fromHashAttributes(attrs)

        tmpKey = "toAdd.node" + sp + "counterReadOnly.alarmHigh"
        hCounterAddClear = Hash(tmpKey, Hash("type", "alarmHigh",
                                             "description", "Too high",
                                             "needsAcknowledging", False),
                                "toClear.node" + sp + "counterReadOnly",
                                ["warnHigh"])
        attrs = hCounterAddClear.getAttributes(tmpKey)
        stampCounter.toHashAttributes(attrs)

        # Check that the signalAlarmUpdate indeed switched level
        # of counterReadOnly
        self.assertTrue(fullyEqual(signal_payload, hCounterAddClear, False),
                        str(hCounterAddClear) + '\nvs\n' + str(signal_payload))

        # set global to none
        reconfigure(caller, dev_id, Hash("stringProperty", "none"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        # Check that the signalAlarmUpdate arrived
        self.assertEqual(signal_id, dev_id)

        hGlobalClear = Hash("toAdd", Hash(),
                            "toClear.global", ["alarm"])

        # Check that the signalAlarmUpdate indeed cleared global
        self.assertTrue(fullyEqual(signal_payload, hGlobalClear, False),
                        str(hGlobalClear) + '\nvs\n' + str(signal_payload))

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
                                          ).waitForReply(max_timeout_ms)
            self.assertEqual(an_id, dev_id, str(hashArg))
            msg = str(hashArg) + ":\n"
            msg += str(hExpected) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hExpected, False), msg)

        #########################################################
        # 7) Reset everything and test a global alarm that does
        #    NOT require ackowledgement
        #########################################################
        signal_id = signal_payload = None
        reconfigure(caller, dev_id, Hash("node.counter", 2,
                                         "doubleProperty", -2.))

        # Check that signalAlarmUpdate arrived
        self.assertEqual(signal_id, dev_id)

        _, alarmCond = getPropsAndAlarm(caller, dev_id)
        self.assertEqual(alarmCond, AlarmCondition.NONE)

        # Check signal that indeed everything is released
        hDoubleCounterClear = Hash("toAdd", Hash(),
                                   "toClear.doublePropertyReadOnly",
                                   ["warnHigh"],
                                   "toClear.node" + sp + "counterReadOnly",
                                   ["alarmHigh"])
        text = str(hDoubleCounterClear) + '\nvs\n' + str(signal_payload)
        self.assertTrue(fullyEqual(signal_payload, hDoubleCounterClear, False),
                        text)

        if api != "mdl":
            # MDL does not know global alarms that do NOT require acknowledging
            reconfigure(caller, dev_id, Hash("stringProperty", "warn"))
            caller.request(dev_id,
                           "setNoAckAlarm").waitForReply(max_timeout_ms)
            devProps, _ = getPropsAndAlarm(caller, dev_id)
            attrs = devProps.getAttributes(global_alarm_key)
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
                                          ).waitForReply(max_timeout_ms)
            self.assertEqual(an_id, dev_id)
            msg = str(hExpected) + '\nvs\n' + str(res)
            self.assertTrue(fullyEqual(res, hExpected, False), msg)

            # Reset again everything ==> all APIs start equally into next test
            reconfigure(caller, dev_id, Hash("stringProperty", "none"))
            signal_id = signal_payload = None
            caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
            self.assertEqual(signal_id, dev_id)
            hGlobalClear["toClear.global"] = ["warn"]
            self.assertTrue(fullyEqual(signal_payload, hGlobalClear, False),
                            str(hGlobalClear) + '\nvs\n' + str(signal_payload))
            _, alarmCond = getPropsAndAlarm(caller, dev_id)
            self.assertEqual(alarmCond, AlarmCondition.NONE)

        #########################################################
        # 8) Test that global 'warn' is kept when global 'alarm' appears
        #    and only when global goes to 'none' all are cleared
        #########################################################
        # 8a) go to warn
        reconfigure(caller, dev_id, Hash("stringProperty", "warn"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        self.assertEqual(signal_id, dev_id)
        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes(global_alarm_key)
        stamp = Timestamp.fromHashAttributes(attrs)
        tmpKey = "toAdd.global.warn"
        hGlobalWarnAdd = Hash(tmpKey, Hash("type", "warn",
                                           "description", desc,
                                           "needsAcknowledging", True),
                              "toClear", Hash())
        attrs = hGlobalWarnAdd.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)
        self.assertTrue(fullyEqual(signal_payload, hGlobalWarnAdd, False),
                        str(hGlobalWarnAdd) + '\nvs\n' + str(signal_payload))

        # 8b) increase to alarm - only the new 'alarm' level is in toAdd!
        reconfigure(caller, dev_id, Hash("stringProperty", "alarm"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        self.assertEqual(signal_id, dev_id)
        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes(global_alarm_key)
        stamp = Timestamp.fromHashAttributes(attrs)
        tmpKey = "toAdd.global.alarm"
        hGlobalAlarmAdd = Hash(tmpKey, Hash("type", "alarm",
                                            "description", desc,
                                            "needsAcknowledging", True),
                               "toClear", Hash())
        attrs = hGlobalAlarmAdd.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)

        self.assertTrue(fullyEqual(signal_payload, hGlobalAlarmAdd, False),
                        str(hGlobalAlarmAdd) + '\nvs\n' + str(signal_payload))

        # If we request current alarms, both are there!
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", Hash()
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        hGlobalBothAdd = copy(hGlobalWarnAdd)
        hGlobalBothAdd.merge(hGlobalAlarmAdd)

        self.assertTrue(fullyEqual(res, hGlobalBothAdd, False),
                        str(hGlobalBothAdd) + '\nvs\n' + str(res))

        # 8c) Increase to interlock
        #     only the new 'interlock' level is in toAdd!
        reconfigure(caller, dev_id, Hash("stringProperty", "interlock"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        self.assertEqual(signal_id, dev_id)
        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes(global_alarm_key)
        stamp = Timestamp.fromHashAttributes(attrs)
        tmpKey = "toAdd.global.interlock"
        hGlobalInterlockAdd = Hash(tmpKey, Hash("type", "interlock",
                                                "description", desc,
                                                "needsAcknowledging", True),
                                   "toClear", Hash())
        attrs = hGlobalInterlockAdd.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)
        self.assertTrue(fullyEqual(signal_payload, hGlobalInterlockAdd, False),
                        str(hGlobalInterlockAdd) + '\nvs\n'
                        + str(signal_payload))
        # If we request current alarms, all three are there!
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", Hash()
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        hGlobalThreeAdd = copy(hGlobalBothAdd)
        hGlobalThreeAdd.merge(hGlobalInterlockAdd)
        self.assertTrue(fullyEqual(res, hGlobalThreeAdd, False),
                        str(hGlobalThreeAdd) + '\nvs\n' + str(res))

        # 8d) release to warn - alarm and interlock are cleared
        reconfigure(caller, dev_id, Hash("stringProperty", "warn"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        self.assertEqual(signal_id, dev_id)
        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes(global_alarm_key)
        stamp = Timestamp.fromHashAttributes(attrs)
        # overwrite new timestamp
        tmpKey = "toAdd.global.warn"
        stamp.toHashAttributes(hGlobalWarnAdd.getAttributes(tmpKey))
        hGlobalWarnAddTwoClear = copy(hGlobalWarnAdd)
        # Order is fixed?
        hGlobalWarnAddTwoClear["toClear.global"] = ["alarm", "interlock"]
        self.assertTrue(fullyEqual(signal_payload, hGlobalWarnAddTwoClear,
                                   False),
                        str(hGlobalWarnAddTwoClear) + '\nvs\n'
                        + str(signal_payload))
        # If we request current alarms, only warn is there, interlock and alarm
        # are gone).  (We test here that at least for global alarms the slot
        # understands two alarm levels as input for same key.)
        hashArg = Hash("global.interlock", Hash(), "global.alarm", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        hGlobalWarnAddInterlClear = copy(hGlobalWarnAdd)
        hGlobalWarnAddInterlClear["toClear.global"] = ["interlock", "alarm"]
        isOK = fullyEqual(res, hGlobalWarnAddInterlClear, False)
        if not isOK:  # order is irrelevant - try the other one
            hGlobalWarnAddInterlClear["toClear.global"] = ["alarm",
                                                           "interlock"]
            isOK = fullyEqual(res, hGlobalWarnAddInterlClear, False)
        self.assertTrue(isOK,
                        str(hGlobalWarnAddInterlClear) + '\nvs\n' + str(res))

        # 8e) increase to interlock again (i.e. skip alarm), then go to alarm
        reconfigure(caller, dev_id, Hash("stringProperty", "interlock"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        self.assertEqual(signal_id, dev_id)
        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes(global_alarm_key)
        stamp = Timestamp.fromHashAttributes(attrs)
        # overwrite new timestamp
        tmpKey = "toAdd.global.interlock"
        attrs = hGlobalInterlockAdd.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)
        self.assertTrue(fullyEqual(signal_payload, hGlobalInterlockAdd,
                                   False),
                        str(hGlobalInterlockAdd) + '\nvs\n'
                        + str(signal_payload))
        # If alarms requested, we get warn and interlock
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", Hash()
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        # now both globals alive, stamps are known
        hGlobalWarnInterlockAdd = copy(hGlobalWarnAdd)
        hGlobalWarnInterlockAdd.merge(hGlobalInterlockAdd)
        self.assertTrue(fullyEqual(res, hGlobalWarnInterlockAdd, False),
                        str(hGlobalWarnInterlockAdd) + '\nvs\n' + str(res))

        # now go to alarm, i.e. toAdd alarm, toClear interlock
        reconfigure(caller, dev_id, Hash("stringProperty", "alarm"))
        signal_id = signal_payload = None
        caller.request(dev_id, "setAlarm").waitForReply(max_timeout_ms)
        self.assertEqual(signal_id, dev_id)
        devProps, _ = getPropsAndAlarm(caller, dev_id)
        attrs = devProps.getAttributes(global_alarm_key)
        stamp = Timestamp.fromHashAttributes(attrs)
        hGlobalAlarmAddInterlClear = copy(hGlobalAlarmAdd)
        # overwrite timestamp
        tmpKey = "toAdd.global.alarm"
        attrs = hGlobalAlarmAddInterlClear.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)
        hGlobalAlarmAddInterlClear["toClear.global"] = ["interlock"]
        self.assertTrue(fullyEqual(signal_payload, hGlobalAlarmAddInterlClear,
                                   False),
                        str(hGlobalAlarmAddInterlClear) + '\nvs\n'
                        + str(signal_payload))

        # If we request current alarms, both warn and alarm are there!
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", Hash()
                                      ).waitForReply(max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        # Can re-use hGlobalBothAdd, but update timestamps
        tmpKey = "toAdd.global.alarm"
        attrs = hGlobalBothAdd.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)
        tmpKey = "toAdd.global.warn"
        attrs = hGlobalWarnAdd.getAttributes(tmpKey)
        stamp = Timestamp.fromHashAttributes(attrs)
        attrs = hGlobalBothAdd.getAttributes(tmpKey)
        stamp.toHashAttributes(attrs)
        self.assertTrue(fullyEqual(res, hGlobalBothAdd, False),
                        str(hGlobalBothAdd) + '\nvs\n' + str(res))

    def _initialize_device(self, api):
        "Start server and PropertyTest device for api"

        klass = "PropertyTest"
        if api == "mdl":
            klass += "MDL"
        server_id = "server/" + api
        self.start_server(api, server_id, [klass])  # , logLevel="INFO")

        dev_id = "DEV/TO/TEST_ALARM_" + api
        self.start_device(server_id, klass, dev_id, cfg=Hash())

        return dev_id

    def start_device(self, server_id, class_id, dev_id, cfg=Hash()):
        """
        Start device with cfg on server
        """
        cfg.set("deviceId", dev_id)

        ok, msg = self.dc.instantiate(server_id, class_id, cfg, max_timeout)
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(class_id, server_id, msg))
