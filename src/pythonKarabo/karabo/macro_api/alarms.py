# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from asyncio import TimeoutError, wait_for
from collections import OrderedDict, namedtuple

from karabo.middlelayer_api.device_client import call, get_instance
from karabo.middlelayer_api.eventloop import synchronize
from karabo.native import Timestamp

# Alarm basic definitions
# ---------------------------

ALARM_ID = 'id'
TIME_OF_FIRST_OCCURENCE = 'timeOfFirstOccurrence'
TIME_OF_OCCURENCE = 'timeOfOccurrence'
DEVICE_ID = 'deviceId'
PROPERTY = 'property'
ALARM_TYPE = 'type'
NEEDS_ACKNOWLEDGING = 'needsAcknowledging'
ACKNOWLEDGEABLE = 'acknowledgeable'
ACKNOWLEDGE = 'acknowledge'

ALARM_DATA = OrderedDict()
ALARM_DATA[ALARM_ID] = 'ID'
ALARM_DATA[TIME_OF_FIRST_OCCURENCE] = 'Time of First Occurence'
ALARM_DATA[TIME_OF_OCCURENCE] = 'Time of Occurence'
ALARM_DATA[DEVICE_ID] = 'Device ID'
ALARM_DATA[PROPERTY] = 'Property'
ALARM_DATA[ALARM_TYPE] = 'Type'
ALARM_DATA[ACKNOWLEDGE] = 'Acknowledge'

AlarmEntry = namedtuple('AlarmEntry', [key for key in ALARM_DATA.keys()])

#
# ---------------------------

KARABO_ALARMSERVICE_CLASS = "AlarmService"
__ALARM_SERVICE = None


def _get_alarm_service():
    """Return the deviceId of the alarm service singleton
    """
    global __ALARM_SERVICE
    if __ALARM_SERVICE is not None:
        return __ALARM_SERVICE

    instance = get_instance()
    for k, v, a in instance.systemTopology["device"].iterall():
        if a["classId"] == KARABO_ALARMSERVICE_CLASS:
            __ALARM_SERVICE = k
            return __ALARM_SERVICE
    return None


@synchronize
def _get_service_information(interlock_only=False):
    """Gather the alarm service information
    """
    service = _get_alarm_service()
    if service is not None:
        try:
            reply = yield from wait_for(call(service, 'slotRequestAlarmDump'),
                                        timeout=5)
        except TimeoutError:
            return None

        service_hash = reply['alarms']
        entries = []
        for update_type, hsh in service_hash.items():
            # Get data of hash
            for _, info_hsh in hsh.items():
                _type = info_hsh.get(ALARM_TYPE)
                if (_type == 'none'
                        or (interlock_only and _type != 'interlock')):
                    # No need to add an entry
                    continue
                params = {k: str(info_hsh.get(k))
                          for k in ALARM_DATA.keys() if k in info_hsh}
                # Time of first occurence
                params[TIME_OF_FIRST_OCCURENCE] = Timestamp(
                    params[TIME_OF_FIRST_OCCURENCE]).toLocal()
                # Time of occurence
                params[TIME_OF_OCCURENCE] = Timestamp(
                    params[TIME_OF_OCCURENCE]).toLocal()
                needs_ack = info_hsh.get(NEEDS_ACKNOWLEDGING)
                ack = info_hsh.get(ACKNOWLEDGEABLE)
                acknowledge = needs_ack and ack
                # Create namedtuple
                entry = AlarmEntry(acknowledge="{}".format(acknowledge),
                                   **params)
                entries.append(entry)

        return entries


@synchronize
def showAlarms():
    """Show the alarm overview in the command line interface
    """

    entries = yield from _get_service_information()
    if entries is not None:
        TEMPLATE = ("|{:^4}|{:^45}|{:^15}|{:^35}|{:^15}|")
        size = len(TEMPLATE.format(*([" "] * (TEMPLATE.count("|") - 1))))

        print("Alarm information requested from Alarm service!")
        print('=' * size)
        print(TEMPLATE.format(
            "ID", "DeviceID", "TYPE", "PROPERTY", "CAN ACK"))
        print('=' * size)
        for entry in entries:
            print(TEMPLATE.format(entry.id, entry.deviceId,
                                  entry.type, entry.property,
                                  entry.acknowledge))
        print('=' * size)


@synchronize
def showInterlocks():
    """Show the interlock overview in the command line interface
    """

    entries = yield from _get_service_information(interlock_only=True)
    if entries is not None:
        TEMPLATE = ("|{:^4}|{:^45}|{:^30}|{:^30}|")
        size = len(TEMPLATE.format(*([" "] * (TEMPLATE.count("|") - 1))))

        print("Interlock information requested from Alarm service!")
        print('=' * size)
        print(TEMPLATE.format(
            "ID", "DeviceID", "FIRST OCCURENCE", "OCCURENCE"))
        print('=' * size)
        for entry in entries:
            print(TEMPLATE.format(entry.id, entry.deviceId,
                                  entry.timeOfFirstOccurrence,
                                  entry.timeOfOccurrence))
        print('=' * size)
