from collections import OrderedDict

from ..const import (ACKNOWLEDGEABLE, ALARM_HIGH, ALARM_ID, ALARM_NONE,
                     ALARM_TYPE, DESCRIPTION, DEVICE_ID, NEEDS_ACKNOWLEDGING,
                     PROPERTY, TIME_OF_FIRST_OCCURENCE, TIME_OF_OCCURENCE)
from .. import utils


def test_extract_alarms_data():
    data = OrderedDict()

    data['entry1'] = {'uptype1': {
            ALARM_ID: 0,
            PROPERTY: 'choochability',
            DESCRIPTION: 'choochability unsufficient',
            ACKNOWLEDGEABLE: True,
            ALARM_TYPE: ALARM_HIGH,
            DEVICE_ID: 'Bobby',
            NEEDS_ACKNOWLEDGING: True,
            TIME_OF_OCCURENCE: '2017-04-20T10:32:22 UTC',
            TIME_OF_FIRST_OCCURENCE: '2017-04-20T09:32:22 UTC'}}
    data['entry2'] = {'uptype1': {
            ALARM_ID: 1,
            PROPERTY: 'choochness',
            DESCRIPTION: 'choochness over 90000',
            ACKNOWLEDGEABLE: False,
            ALARM_TYPE: ALARM_HIGH,
            DEVICE_ID: 'Jenny',
            NEEDS_ACKNOWLEDGING: False,
            TIME_OF_OCCURENCE: '2017-04-20T10:12:22 UTC',
            TIME_OF_FIRST_OCCURENCE: '2017-04-20T09:12:22 UTC'}}
    data['entry3'] = {'uptype1': {
            ALARM_ID: 1,
            PROPERTY: 'choochness',
            DESCRIPTION: 'choochness over 90000',
            ACKNOWLEDGEABLE: False,
            ALARM_TYPE: ALARM_NONE,
            DEVICE_ID: 'Frank',
            NEEDS_ACKNOWLEDGING: False,
            TIME_OF_OCCURENCE: '2017-04-20T10:12:22 UTC',
            TIME_OF_FIRST_OCCURENCE: '2017-04-20T09:12:22 UTC'}}
    ret = utils.extract_alarms_data('dev1', data)
    assert ret['instance_id'] == 'dev1'
    assert ret['update_types'] == ['uptype1', 'uptype1']
    assert ret['alarm_entries'][0].acknowledge == (True, True)
    assert ret['alarm_entries'][1].acknowledge == (False, False)
    assert ret['alarm_entries'][0].showDevice == 'Bobby'
    assert ret['alarm_entries'][1].showDevice == 'Jenny'
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
