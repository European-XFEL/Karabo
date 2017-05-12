#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QDateTime

from karabo.middlelayer import Timestamp
from karabo_gui.events import broadcast_event
from .const import (ACKNOWLEDGEABLE, ALARM_DATA, ALARM_NONE, ALARM_TYPE,
                    DEVICE_ID, NEEDS_ACKNOWLEDGING, TIME_OF_FIRST_OCCURENCE,
                    TIME_OF_OCCURENCE, AlarmEntry)


def broadcast_about_alarm(event_type, instanceId, rows):
    """An alarm related broadcast needs to be prepared and then send

    :param event_type: AlarmServiceInit or AlarmServiceUpdate
    :param instanceId: The instance id of an `AlarmService` device
    :param rows: A hash including the alarm specific entries
    """
    updateTypes = []
    alarmEntries = []
    for entry_id, hsh in rows.items():
        # Get data of hash
        for update_type, aHash in hsh.items():
            if aHash.get(ALARM_TYPE) == ALARM_NONE:
                # No need to add an entry
                continue
            updateTypes.append(update_type)
            # XXX: TODO use proper UTC to local time lib
            params = {k: str(aHash.get(k))
                      for k in ALARM_DATA.keys() if k in aHash}
            # Time of first occurence
            params[TIME_OF_FIRST_OCCURENCE] = Timestamp(
                params[TIME_OF_FIRST_OCCURENCE]).toTimestamp()
            params[TIME_OF_FIRST_OCCURENCE] = QDateTime.fromMSecsSinceEpoch(
                params[TIME_OF_FIRST_OCCURENCE] * 1000)
            # Time of occurence
            params[TIME_OF_OCCURENCE] = Timestamp(
                params[TIME_OF_OCCURENCE]).toTimestamp()
            params[TIME_OF_OCCURENCE] = QDateTime.fromMSecsSinceEpoch(
                params[TIME_OF_OCCURENCE] * 1000)
            needsAck = aHash.get(NEEDS_ACKNOWLEDGING)
            ack = aHash.get(ACKNOWLEDGEABLE)
            # Create namedtuple
            entry = AlarmEntry(acknowledge=(needsAck, ack),
                               showDevice=params[DEVICE_ID],
                               **params)
            alarmEntries.append(entry)

    # Create KaraboBroadcastEvent
    broadcast_event(event_type, {'instanceId': instanceId,
                                 'updateTypes': updateTypes,
                                 'alarmEntries': alarmEntries})
