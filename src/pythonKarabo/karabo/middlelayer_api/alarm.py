from karabo.common.alarm_conditions import AlarmCondition
from .enums import AccessMode
from .hash import String
from .schema import Configurable


class AlarmMixin(Configurable):
    alarmCondition = String(
        enum=AlarmCondition,
        displayedName="Alarm condition",
        description="The current alarm condition of the device. "
                    "Evaluates to the highest condition on any property "
                    "if not set manually.",
        accessMode=AccessMode.READONLY, defaultValue=AlarmCondition.NONE)

    def __init__(self, configuration):
        self._alarmConditions = {}
        super(AlarmMixin, self).__init__(configuration)

    def setChildValue(self, key, value, desc):
        old_alarm = self._alarmConditions.get(key, AlarmCondition.NONE)
        new_alarm = desc.alarmCondition(value)
        if old_alarm is not new_alarm:
            self._alarmConditions[key] = new_alarm
            if new_alarm > self.alarmCondition.enum:
                self.alarmCondition = new_alarm
            elif new_alarm < old_alarm is self.alarmCondition.enum:
                self.alarmCondition = max(
                    self._alarmConditions.values(),
                    default=AlarmCondition.NONE)
        super(AlarmMixin, self).setChildValue(key, value, desc)
