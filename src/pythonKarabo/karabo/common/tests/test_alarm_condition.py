# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import pytest

from karabo.common.api import AlarmCondition


def test_alarm_condition_roundtrip():
    condition = AlarmCondition.ALARM
    assert condition.asString() == "alarm"
    condition = AlarmCondition.fromString(condition.asString())
    assert condition.asString() == "alarm"
    assert condition == AlarmCondition.ALARM


def test_alarm_condition_no_other_conditions():
    pytest.raises(ValueError, AlarmCondition, "SOME_NEW_NAME")


def test_alarm_condition_significance_evaluation():
    condition_list = [
        AlarmCondition.ALARM,
        AlarmCondition.WARN,
        AlarmCondition.INTERLOCK,
    ]
    most_significant = AlarmCondition.returnMostSignificant(condition_list)
    assert most_significant == AlarmCondition.INTERLOCK
    condition_list = condition_list[:-1]
    most_significant = AlarmCondition.returnMostSignificant(condition_list)
    assert most_significant == AlarmCondition.ALARM
