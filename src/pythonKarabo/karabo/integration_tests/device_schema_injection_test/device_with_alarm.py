from karabo.bound import (
    KARABO_CLASSINFO, PythonDevice, DOUBLE_ELEMENT)


@KARABO_CLASSINFO("DeviceWithAlarm", "1.0")
class DeviceWithAlarm(PythonDevice):
    ALARM_HIGH = 1000.

    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(DeviceWithAlarm.ALARM_HIGH).needsAcknowledging(False)
            .commit(),
        )

    def __init__(self, config):
        super(DeviceWithAlarm, self).__init__(config)
