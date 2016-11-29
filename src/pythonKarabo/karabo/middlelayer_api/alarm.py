from karabo.common.alarm_conditions import AlarmCondition
from .enums import AccessMode
from .hash import Hash, HashType, String
from .schema import Configurable
from .signalslot import Signal, slot


class AlarmMixin(Configurable):
    alarmCondition = String(
        enum=AlarmCondition,
        displayedName="Alarm condition",
        description="The current alarm condition of the device. "
                    "Evaluates to the highest condition on any property "
                    "if not set manually.",
        accessMode=AccessMode.READONLY, defaultValue=AlarmCondition.NONE)

    signalAlarmUpdate = Signal(String(), HashType())

    def __init__(self, configuration):
        self._alarmConditions = {}
        self._changed_alarms = set()
        self._old_alarms = {}
        super(AlarmMixin, self).__init__(configuration)

    def setChildValue(self, key, value, desc):
        old_alarm, _, _ = self._alarmConditions.get(
            key, (AlarmCondition.NONE, None, None))
        new_alarm = desc.alarmCondition(value)
        if old_alarm is not new_alarm:
            self._alarmConditions[key] = new_alarm, desc, value.timestamp
            if new_alarm > self.alarmCondition.enum:
                self.alarmCondition = new_alarm
            elif new_alarm < old_alarm is self.alarmCondition.enum:
                self.alarmCondition = max(
                    (v for v, _, _ in self._alarmConditions.values()),
                    default=AlarmCondition.NONE)
            self._changed_alarms.add(key)
            self._old_alarms[key] = old_alarm
        super(AlarmMixin, self).setChildValue(key, value, desc)

    def update(self):
        toAdd = Hash()
        toClear = Hash()

        for prop in self._changed_alarms:
            cond, desc, timestamp = self._alarmConditions.get(
                prop, (AlarmCondition.NONE, None, None))
            if cond is AlarmCondition.NONE:
                old = self._old_alarms.get(prop, AlarmCondition.NONE)
                if old is not AlarmCondition.NONE:
                    toClear.setdefault(prop, []).append(old.value)
            else:
                toAdd[prop] = Hash(
                    "type", cond.value,
                    "description",
                    getattr(desc, "alarmInfo_{}".format(cond.value), ""),
                    "needsAcknowledging",
                    getattr(desc, "alarmNeedsAck_{}".format(cond.value), ""))
                toAdd[prop, ...] = timestamp.toDict()
        self._old_alarms = {}
        self._changed_alarms = set()
        if toAdd or toClear:
            self.signalAlarmUpdate(self.deviceId,
                                   Hash("toAdd", toAdd, "toClear", toClear))

        super(AlarmMixin, self).update()
