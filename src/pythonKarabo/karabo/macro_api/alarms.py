from collections import OrderedDict, namedtuple

from .middlelayer import call, get_instance, synchronize, Timestamp

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

# ---------------------------

KARABO_ALARMSERVICE_CLASS = "AlarmService"
__ALARM_SERVICE = None


def getAlarmService():
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
def showAlarms():
    """Show the alarm overview in the command line interface
    """
    service = getAlarmService()
    if service is not None:
        reply = yield from call(service, 'slotRequestAlarmDump')

        alarm_hash = reply['alarms']
        alarm_entries = []
        for update_type, hsh in alarm_hash.items():
            # Get data of hash
            for _, alarm_hsh in hsh.items():
                if alarm_hsh.get(ALARM_TYPE) == 'none':
                    # No need to add an entry
                    continue
                params = {k: str(alarm_hsh.get(k))
                          for k in ALARM_DATA.keys() if k in alarm_hsh}
                # Time of first occurence
                params[TIME_OF_FIRST_OCCURENCE] = Timestamp(
                    params[TIME_OF_FIRST_OCCURENCE]).toTimestamp()
                # Time of occurence
                params[TIME_OF_OCCURENCE] = Timestamp(
                    params[TIME_OF_OCCURENCE]).toTimestamp()
                needs_ack = alarm_hsh.get(NEEDS_ACKNOWLEDGING)
                ack = alarm_hsh.get(ACKNOWLEDGEABLE)
                # Create namedtuple
                entry = AlarmEntry(acknowledge="{}/{}".format(needs_ack, ack),
                                   **params)
                alarm_entries.append(entry)

        TEMPLATE = ("|{:^4}|{:^45}|{:^15}|{:^35}|{:^15}|")
        size = len(TEMPLATE.format(*([" "] * (TEMPLATE.count("|") - 1))))

        print("Alarm information requested from Alarm service "
              "device: {}".format(service))
        print('=' * size)
        print(TEMPLATE.format(
            "ID", "DeviceID", "TYPE", "PROPERTY", "LATCH/CAN ACK"))
        print('=' * size)
        for entry in alarm_entries:
            print(TEMPLATE.format(entry.id, entry.deviceId,
                                  entry.type, entry.property,
                                  entry.acknowledge))
        print('=' * size)
