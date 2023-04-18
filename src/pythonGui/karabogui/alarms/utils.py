#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 10, 2017
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import QDateTime

from karabo.native import Timestamp

from .const import (
    ACKNOWLEDGEABLE, ALARM_DATA, ALARM_NONE, ALARM_TYPE, NEEDS_ACKNOWLEDGING,
    TIME_OF_FIRST_OCCURENCE, TIME_OF_OCCURENCE, AlarmEntry)


def extract_alarms_data(instanceId, rows):
    """Extract important data from an alarm service device message.

    :param instanceId: The instance id of an `AlarmService` device
    :param rows: A hash including the alarm specific entries

    :return A dictionary which contains all the important alarm data
    """
    update_types = []
    alarm_entries = []
    for entry_id, hsh in rows.items():
        # Get data of hash
        for update_type, alarm_hsh in hsh.items():
            if alarm_hsh.get(ALARM_TYPE) == ALARM_NONE:
                # No need to add an entry
                continue
            update_types.append(update_type)
            # XXX: TODO use proper UTC to local time lib
            params = {k: str(alarm_hsh.get(k))
                      for k in ALARM_DATA.keys() if k in alarm_hsh}
            # Time of first occurence
            params[TIME_OF_FIRST_OCCURENCE] = Timestamp(
                params[TIME_OF_FIRST_OCCURENCE]).toTimestamp()
            params[TIME_OF_FIRST_OCCURENCE] = QDateTime.fromSecsSinceEpoch(
                int(params[TIME_OF_FIRST_OCCURENCE]))
            # Time of occurence
            params[TIME_OF_OCCURENCE] = Timestamp(
                params[TIME_OF_OCCURENCE]).toTimestamp()
            params[TIME_OF_OCCURENCE] = QDateTime.fromSecsSinceEpoch(
                int(params[TIME_OF_OCCURENCE]))
            needs_ack = alarm_hsh.get(NEEDS_ACKNOWLEDGING)
            ack = alarm_hsh.get(ACKNOWLEDGEABLE)
            # Create namedtuple
            entry = AlarmEntry(acknowledge=(needs_ack, ack),
                               **params)
            alarm_entries.append(entry)

    return {'instance_id': instanceId,
            'update_types': update_types,
            'alarm_entries': alarm_entries}
