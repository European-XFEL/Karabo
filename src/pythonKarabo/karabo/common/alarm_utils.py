#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 12, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Dict, HasStrictTraits, Property, String


class AlarmInfo(HasStrictTraits):
    # Keep track of all alarm_types (key: alarm type, value: list of device
    # properties)
    alarm_types_dict = Dict
    # A property which returns the alarm_type of the highest priority
    alarm_type = Property(String)

    def append_alarm_type(self, dev_property, alarm_type):
        """Append given ``alarm_type`` to dict and update list with device
        properties
        """
        if alarm_type in self.alarm_types_dict:
            self.alarm_types_dict[alarm_type].append(dev_property)
        else:
            self.alarm_types_dict[alarm_type] = [dev_property]

    def remove_alarm_type(self, dev_property, alarm_type):
        """Remove given ``dev_property`` for ``alarm_type`` from dict list
        or remove ``alarm_type`` from dict if list is empty afterwards
        """
        if alarm_type in self.alarm_types_dict:
            dev_props = self.alarm_types_dict[alarm_type]
            dev_props.remove(dev_property)
            if dev_props:
                self.alarm_types_dict[alarm_type] = dev_props
            else:
                del self.alarm_types_dict[alarm_type]

    def _get_alarm_type(self):
        """Return the ``alarm_type`` with the highest priority

        Note: `alarmHigh`<`alarmLow`<`warnHigh`<`warnLow` is the priority
        the key list can be sorted alphabetically and returns the first element
        """
        if self.alarm_types_dict:
            keys = sorted(self.alarm_types_dict.keys())
            return keys[0]
