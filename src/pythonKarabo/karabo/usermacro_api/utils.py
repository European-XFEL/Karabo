from karabo.middlelayer_api.device_client import (
    _parse_date, get_instance, KaraboError
)

from karabo.middlelayer_api.eventloop import synchronize


@synchronize
def _getConfigurationFromPast(deviceId, timepoint):
    instance = get_instance()
    did = "DataLogger-{}".format(deviceId)
    print(did)
    if did not in instance.loggerMap:
        instance.loggerMap = yield from instance.call(
            "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
        if did not in instance.loggerMap:
            raise KaraboError('no logger for device "{}"'.
                              format(deviceId))
    reader = "DataLogReader0-{}".format(instance.loggerMap[did])
    print(reader)

    h, s = yield from get_instance().call(
        reader, "slotGetConfigurationFromPast", deviceId, timepoint)

    return h, s


def getConfigurationFromPast(deviceId, timepoint):
    """Calls slotGetConfigurationFromPast on dataLoglReader"""
    timepoint = _parse_date(timepoint)
    return _getConfigurationFromPast(deviceId, timepoint)
