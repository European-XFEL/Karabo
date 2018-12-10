from karabo.common.alarm_conditions import AlarmCondition
from .enums import AccessLevel, AccessMode, DaqPolicy
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
        displayedName="Alarm condition", displayType='AlarmCondition',
        description="The current alarm condition of the device. "
                    "Evaluates to the highest condition on any property "
                    "if not set manually.",
        accessMode=AccessMode.READONLY,
        defaultValue=AlarmCondition.NONE,
        daqPolicy=DaqPolicy.OMIT)

    globalAlarmCondition = Alarm(
        displayedName="Global Alarm Condition", displayType='AlarmCondition',
        description="This is the global alarm condition for this device. "
                    "The current alarm condition of this device will be "
                    "calculated from this alarm condition and the alarm "
                    "condition of each property.",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.ADMIN,
        defaultValue=AlarmCondition.NONE,
        daqPolicy=DaqPolicy.OMIT)

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
            # Needed for our validator in case of node elements
            prop_sep = "KRB_ALARM_SEP_REPLACEMENT".join(prop.split('.'))
            prop_sep = 'global' if prop == 'globalAlarmCondition' else prop_sep
            old = self._old_alarms.get(prop, AlarmCondition.NONE)
            if old is not AlarmCondition.NONE:
                if (cond is AlarmCondition.NONE and
                        prop == 'globalAlarmCondition'):
                    toClear.setdefault(prop_sep, []).extend(
                        self.accumulatedGlobalAlarms)
                    self.accumulatedGlobalAlarms.clear()
                else:
                    toClear.setdefault(prop_sep, []).append(old.value)

            if cond is not AlarmCondition.NONE:
                toAdd[prop_sep] = Hash(cond.value, Hash(
                    "type", cond.value,
                    "description",
                    getattr(desc, "alarmInfo_{}".format(cond.value), ""),
                    "needsAcknowledging",
                    bool(getattr(desc, "alarmNeedsAck_{}".format(cond.value),
                                 True))))
                toAdd[prop_sep][cond.value, ...] = timestamp.toDict()
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
