# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common.alarm_conditions import AlarmCondition
from karabo.native import (
    AccessLevel, AccessMode, Configurable, DaqPolicy, EnumValue, Hash, String,
    TypeHash)

from .signalslot import Signal, slot


class Alarm(String):
    def __init__(self, **kwargs):
        super().__init__(enum=AlarmCondition, **kwargs)

    def alarmCondition(self, value):
        return value.enum


class AlarmMixin(Configurable):
    separator = "KRB_ALARM_SEP_REPLACEMENT"  # for dots in nested properties

    alarmCondition = String(
        enum=AlarmCondition,
        displayedName="Alarm condition", displayType='AlarmCondition',
        description="The current alarm condition of the device. "
                    "Evaluates to the highest condition on any property "
                    "if not set manually.",
        accessMode=AccessMode.READONLY,
        defaultValue=AlarmCondition.NONE,
        classId="AlarmCondition",
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
        classId="AlarmCondition",
        daqPolicy=DaqPolicy.OMIT)

    signalAlarmUpdate = Signal(String(), TypeHash())

    def __init__(self, configuration):
        self._alarmConditions = {}
        self._changed_alarms = set()
        self._old_alarms = {}
        self.accumulatedGlobalAlarms = dict()
        super(AlarmMixin, self).__init__(configuration)

    def setChildValue(self, key, value, desc):

        # Alarm-internally mangle key as needed for external communication
        key_sep = self.separator.join(key.split('.')) if '.' in key else key
        if key_sep == "globalAlarmCondition":
            key_sep = "global"

        old_alarm, _, _ = self._alarmConditions.get(
            key_sep, (AlarmCondition.NONE, None, None))
        new_alarm = desc.alarmCondition(value)
        if old_alarm is not new_alarm:
            self._alarmConditions[key_sep] = new_alarm, desc, value.timestamp
            if new_alarm > self.alarmCondition.enum:
                lev, ts = new_alarm.criticalityLevel(), value.timestamp
                self.alarmCondition = EnumValue(lev, timestamp=ts)
            elif (new_alarm.criticalityLevel() < old_alarm.criticalityLevel()
                  is self.alarmCondition.enum):
                alarmCondition = max(
                    (v.criticalityLevel()
                     for v, _, _ in self._alarmConditions.values()),
                    default=AlarmCondition.NONE)
                if alarmCondition != self.alarmCondition:
                    lv, ts = alarmCondition.criticalityLevel(), value.timestamp
                    self.alarmCondition = EnumValue(lv, timestamp=ts)
            # book-keeping for self.__gather_alarms()
            self._changed_alarms.add(key_sep)
            self._old_alarms[key_sep] = old_alarm
            if key_sep == 'global' and new_alarm != AlarmCondition.NONE:
                # May override previous setting
                self.accumulatedGlobalAlarms[new_alarm.value] = value.timestamp

        # Outside, of course use unmangled key
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

            old = self._old_alarms.get(prop, AlarmCondition.NONE)

            # First check what is to add:
            if cond is not AlarmCondition.NONE:
                toAdd[prop] = Hash(cond.value, Hash(
                    "type", cond.value,
                    "description",
                    getattr(desc, "alarmInfo_{}".format(cond.value), ""),
                    "needsAcknowledging",
                    bool(getattr(desc, "alarmNeedsAck_{}".format(cond.value),
                                 True))))
                toAdd[prop][cond.value, ...] = timestamp.toDict()

            # Now see what is to clear:
            # Note: 'old is cond' may be when called from slotReSubmitAlarms
            if old is not AlarmCondition.NONE and old is not cond:
                if prop == 'global':
                    # global alarms accumulate:
                    # clear only those more critical than the new one
                    for accum_str in list(self.accumulatedGlobalAlarms.keys()):
                        accum = AlarmCondition(accum_str)
                        if accum.criticalityLevel() > cond.criticalityLevel():
                            toClear.setdefault(prop, []).append(accum_str)
                            del self.accumulatedGlobalAlarms[accum_str]
                else:
                    toClear.setdefault(prop, []).append(old.value)

        self._old_alarms = {}
        self._changed_alarms = set()

        if toAdd or toClear:
            return Hash("toAdd", toAdd, "toClear", toClear)

    @slot
    def slotReSubmitAlarms(self, existing):
        ret = Hash("toAdd", Hash(), "toClear", Hash())

        # Treat all but global alarm condition
        self._changed_alarms = set(self._alarmConditions.keys())
        self._changed_alarms.update(existing.keys())
        self._changed_alarms.discard("global")
        self._old_alarms = {k: AlarmCondition(next(iter(v)))
                            for k, v in existing.items()
                            if k != "global"}
        property_changes = self.__gather_alarms()
        if property_changes is not None:
            ret.merge(property_changes)

        # Now add what is needed for global alarm condition
        for alarm_str, timestamp in self.accumulatedGlobalAlarms.items():
            # global alarms: no description, but always require acknowledgment
            ret["toAdd.global." + alarm_str] = Hash("type", alarm_str,
                                                    "description", "",
                                                    "needsAcknowledging", True)
            ret["toAdd.global." + alarm_str, ...] = timestamp.toDict()

        globalAlarmsToClear = [a for a in existing.get("global", Hash()).keys()
                               if a not in self.accumulatedGlobalAlarms]
        if globalAlarmsToClear:
            ret["toClear.global"] = globalAlarmsToClear

        return self.deviceId, ret
