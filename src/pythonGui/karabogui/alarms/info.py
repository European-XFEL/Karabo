#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 12, 2017
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from traits.api import Dict, HasStrictTraits, Property, String

from karabo.common.api import AlarmCondition


class AlarmInfo(HasStrictTraits):
    # Keep track of all alarm_types (key: alarm type, value: set of device
    # properties)
    alarm_dict = Dict
    # A property which returns the alarm_type of the highest priority
    alarm_type = Property(String)

    def append_alarm_type(self, dev_property, alarm_type):
        """Append given ``alarm_type`` to dict and update set with device
        properties
        """
        if not isinstance(alarm_type, AlarmCondition):
            alarm_type = AlarmCondition.fromString(alarm_type)

        self.alarm_dict.setdefault(alarm_type, set()).add(dev_property)

    def remove_alarm_type(self, dev_property, alarm_type):
        """Remove given ``dev_property`` for ``alarm_type`` from dict set
        or remove ``alarm_type`` from dict if set is empty afterwards
        """
        if not isinstance(alarm_type, AlarmCondition):
            alarm_type = AlarmCondition.fromString(alarm_type)

        if alarm_type in self.alarm_dict:
            dev_props = self.alarm_dict[alarm_type]
            if dev_property in dev_props:
                # XXX: somehow the dev_property get lost after the same
                # property triggers multiple time of different warnings.
                # to be investigated.
                dev_props.remove(dev_property)
            if len(dev_props) == 0:
                del self.alarm_dict[alarm_type]

    def _get_alarm_type(self):
        """Return the ``alarm_type`` with the highest priority
        """
        if self.alarm_dict:
            # AlarmCondition supports the less-than operator, so max() returns
            # the highest priority alarm type in the dictionary.
            return max(self.alarm_dict).asString()
        return ''
