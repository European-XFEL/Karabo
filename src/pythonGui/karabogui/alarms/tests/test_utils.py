# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabogui.testing import alarm_data

from .. import utils


def test_extract_alarms_data():
    data = alarm_data()
    ret = utils.extract_alarms_data('dev1', data)
    assert ret['instance_id'] == 'dev1'
    assert ret['update_types'] == ['uptype1', 'uptype1']
    assert ret['alarm_entries'][0].acknowledge == (True, True)
    assert ret['alarm_entries'][1].acknowledge == (False, False)
    sdate = ret['alarm_entries'][0].timeOfFirstOccurrence.toUTC().date()
    sdate = (sdate.dayOfYear(), sdate.year())
    assert sdate == (110, 2017)
    sdate = ret['alarm_entries'][1].timeOfFirstOccurrence.toUTC().date()
    sdate = (sdate.dayOfYear(), sdate.year())
    assert sdate == (110, 2017)
    sdate = ret['alarm_entries'][0].timeOfOccurrence.toUTC().date()
    sdate = (sdate.dayOfYear(), sdate.year())
    assert sdate == (110, 2017)
    sdate = ret['alarm_entries'][1].timeOfOccurrence.toUTC().date()
    sdate = (sdate.dayOfYear(), sdate.year())
    assert sdate == (110, 2017)
    stime = ret['alarm_entries'][0].timeOfFirstOccurrence.toUTC().time()
    stime = (stime.hour(), stime.minute())
    assert stime == (9, 32)
    stime = ret['alarm_entries'][1].timeOfFirstOccurrence.toUTC().time()
    stime = (stime.hour(), stime.minute())
    assert stime == (9, 12)
    stime = ret['alarm_entries'][0].timeOfOccurrence.toUTC().time()
    stime = (stime.hour(), stime.minute())
    assert stime == (10, 32)
    stime = ret['alarm_entries'][1].timeOfOccurrence.toUTC().time()
    stime = (stime.hour(), stime.minute())
    assert stime == (10, 12)
