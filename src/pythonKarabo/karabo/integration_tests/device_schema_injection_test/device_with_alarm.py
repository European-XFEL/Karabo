from karabo.bound import (
    KARABO_CLASSINFO, PythonDevice, DOUBLE_ELEMENT)


@KARABO_CLASSINFO("DeviceWithAlarm", "1.0")
class DeviceWithAlarm(PythonDevice):

    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(1000.).needsAcknowledging(False)
            .commit(),
        )

    def __init__(self, config):
        super(DeviceWithAlarm, self).__init__(config)
