from karabo.common.alarm_conditions import AlarmCondition
from .enums import AccessMode
from .hash import Hash, HashType, String
from .schema import Configurable
from .signalslot import Signal, slot


class Alarm(String):
    def __init__(self, **kwargs):
        super().__init__(enum=AlarmCondition, **kwargs)

    def alarmCondition(self, value):
        return value.enum


class AlarmMixin(Configurable):
    alarmCondition = String(
        enum=AlarmCondition,
        displayedName="Alarm condition",
        description="The current alarm condition of the device. "
                    "Evaluates to the highest condition on any property "
                    "if not set manually.",
        accessMode=AccessMode.READONLY, defaultValue=AlarmCondition.NONE)

    globalAlarmCondition = Alarm(
        displayedName="Global Alarm Condition",
        description="This is the alarm condition of the entire device",
        accessMode=AccessMode.READONLY, defaultValue=AlarmCondition.NONE)

    signalAlarmUpdate = Signal(String(), HashType())

    def __init__(self, configuration):
        self._alarmConditions = {}
        self._changed_alarms = set()
        self._old_alarms = {}
        self.accumulatedGlobalAlarms = set()
        super(AlarmMixin, self).__init__(configuration)

    def setChildValue(self, key, value, desc):
        old_alarm, _, _ = self._alarmConditions.get(
            key, (AlarmCondition.NONE, None, None))
        new_alarm = desc.alarmCondition(value)
        if old_alarm is not new_alarm:
            self._alarmConditions[key] = new_alarm, desc, value.timestamp
            if new_alarm > self.alarmCondition.enum:
                self.alarmCondition = new_alarm.criticalityLevel()
            elif (new_alarm.criticalityLevel() < old_alarm.criticalityLevel()
                  is self.alarmCondition.enum):
                self.alarmCondition = max(
                    (v.criticalityLevel()
                     for v, _, _ in self._alarmConditions.values()),
                    default=AlarmCondition.NONE)
            self._changed_alarms.add(key)
            self._old_alarms[key] = old_alarm
            if key == 'globalAlarmCondition':
                self.accumulatedGlobalAlarms.add(new_alarm.value)
        super(AlarmMixin, self).setChildValue(key, value, desc)

    def update(self):
        changes = self.__gather_alarms()
        if changes is not None:
            self.signalAlarmUpdate(self.deviceId, changes)
        super(AlarmMixin, self).update()

    def __gather_alarms(self):
        toAdd = Hash()
        toClear = Hash()

        for prop in self._changed_alarms:
            cond, desc, timestamp = self._alarmConditions.get(
                prop, (AlarmCondition.NONE, None, None))
            if cond is AlarmCondition.NONE:
                old = self._old_alarms.get(prop, AlarmCondition.NONE)
                if old is not AlarmCondition.NONE:
                    if prop == 'globalAlarmCondition':
                        toClear.setdefault(prop, []).extend(
                            self.accumulatedGlobalAlarms)
                        self.accumulatedGlobalAlarms.clear()
                    else:
                        toClear.setdefault(prop, []).append(old.value)
            else:
                toAdd[prop] = Hash(cond.value, Hash(
                    "type", cond.value,
                    "description",
                    getattr(desc, "alarmInfo_{}".format(cond.value), ""),
                    "needsAcknowledging",
                    bool(getattr(desc, "alarmNeedsAck_{}".format(cond.value),
                                 True))))
                toAdd[prop][cond.value, ...] = timestamp.toDict()
        self._old_alarms = {}
        self._changed_alarms = set()

        if toAdd or toClear:
            return Hash("toAdd", toAdd, "toClear", toClear)

    @slot
    def slotReSubmitAlarms(self, existing):
        self._changed_alarms = set(self._alarmConditions.keys())
        self._changed_alarms.update(existing.keys())
        self._old_alarms = {k: AlarmCondition(next(iter(v)))
                            for k, v in existing.items()}

        changes = self.__gather_alarms()
        if changes is not None:
            return self.deviceId, changes
        else:
            return self.deviceId, Hash("toAdd", Hash(), "toClear", Hash())
