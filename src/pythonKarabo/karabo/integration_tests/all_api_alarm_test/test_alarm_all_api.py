#from datetime import datetime
#from time import sleep, time

from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.bound import AlarmCondition, Hash, SignalSlotable, State, Timestamp
from karathon import fullyEqual

class TestDeviceAlarmApi(BoundDeviceTestCase):
    _max_timeout = 20    # in seconds
    _max_timeout_ms = _max_timeout * 1000


    def test_alarm_cpp(self):
        self._test_alarm("cpp")

    def test_alarm_bound(self):
        self._test_alarm("bound")

    def test_alarm_mdl(self):
        self._test_alarm("mdl")

    def _test_alarm(self, api):

        # Start server, device and helper to call slots with arguments
        # (the latter is needed until DeviceClient can call these slots)
        klass = "PropertyTest"
        if api == "mdl":
            klass += "MDL"
        server_id = "server/" + api
        self.start_server(api, server_id, [klass]) #, **kwargs for log level?

        dev_id = "DEV/TO/TEST_ALARM_" + api
        self.start_device(server_id, klass, dev_id, cfg=Hash())

        caller = SignalSlotable("helperAlarmTests")
        caller.start()

        globalAlarmKey = "alarmCondition"
        # Initially all is fine
        alarmCond = self.dc.get(dev_id, globalAlarmKey)
        self.assertEqual(alarmCond, AlarmCondition.NONE)

        # 1) Set property to warn level
        # (implicitly since PropertyTest device transfers reconfig. property
        # values to their readOnly counterparts)
        #
        self.dc.set(dev_id, "doubleProperty", 50.)
        # Global alarm should be raised... 
        alarmCond = self.dc.get(dev_id, globalAlarmKey)
        self.assertEqual(alarmCond, AlarmCondition.WARN)
        # ...and property in configuration carries alarmCondition as well
        attrs = self.dc.get(dev_id).getAttributes("doublePropertyReadOnly")
        if api != "mdl":  # FIXME!
            # MDL does not send alarm condition as attribute 
            self.assertEqual(attrs.get("alarmCondition"), "warnHigh")
        stamp = Timestamp.fromHashAttributes(attrs)

        # Now see how various calls to slotReSubmitAlarms react
        # 1a) empty input: return the alarm in 'toAdd'
        hashArg = Hash()
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg
                                      ).waitForReply(self._max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        h = Hash("toClear", Hash(),
                 "toAdd", Hash("doublePropertyReadOnly.warnHigh",
                               Hash("type", "warnHigh",
                                    "description", "Rather high",
                                    "needsAcknowledging", False)))
        attrs = h.getAttributes("toAdd.doublePropertyReadOnly.warnHigh")
        stamp.toHashAttributes(attrs)
        self.assertTrue(fullyEqual(res, h, False),
                        str(h) + '\nvs\n' + str(res))

        # 1b) existing alarm input: return the alarm in 'toAdd' as well
        hashArg = Hash("doublePropertyReadOnly.warnHigh", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg,
                                      ).waitForReply(self._max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        if api == "mdl":
            # FIXME: I see our alarm also in 'toClear'!
            self.assertTrue(fullyEqual(res["toAdd"], h["toAdd"], False),
                            str(h) + '\nvs\n' + str(res))
        else:
            self.assertTrue(fullyEqual(res, h, False),
                            str(h) + '\nvs\n' + str(res))

        # 1c) non-existing alarm input:
        # return the existing alarm in 'toAdd' again
        #        and the non-existing in 'toClear'
        hashArg = Hash("int32PropertyReadOnly.alarmHigh", Hash())
        (an_id, res) = caller.request(dev_id,
                                      "slotReSubmitAlarms", hashArg,
                                      ).waitForReply(self._max_timeout_ms)
        self.assertEqual(an_id, dev_id)
        h["toClear.int32PropertyReadOnly"] = ["alarmHigh"]
        self.assertTrue(fullyEqual(res, h, False),
                        str(h) + '\nvs\n' + str(res))

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
