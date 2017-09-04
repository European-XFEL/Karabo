"""This module contains helper routines"""
import collections
from karabo.middlelayer_api.device_client import (
    _parse_date, get_instance, KaraboError
)

from karabo.middlelayer_api.eventloop import synchronize

# imports used for plotLoggedData
import datetime
import matplotlib.pyplot as pl
import matplotlib.dates as mdates
from IPython import get_ipython
DATE_FORMAT = "%Y-%m-%dT%H:%M:%S"


def flatten(lis):
    """ Flattens a list that may contain sublists"""
    if isinstance(lis, (str, bytes)):
        yield lis
    else:
        for e in lis:
            if (isinstance(e, collections.Iterable) and
                    not isinstance(e, (str, bytes))):
                for ee in flatten(e):
                    yield ee
            else:
                yield e


@synchronize
def _getConfigurationFromPast(deviceId, timepoint):
    instance = get_instance()
    did = "DataLogger-{}".format(deviceId)
    if did not in instance.loggerMap:
        instance.loggerMap = yield from instance.call(
            "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
        if did not in instance.loggerMap:
            raise KaraboError('no logger for device "{}"'.
                              format(deviceId))
    reader = "DataLogReader0-{}".format(instance.loggerMap[did])

    h, s = yield from get_instance().call(
        reader, "slotGetConfigurationFromPast", deviceId, timepoint)

    return h, s


def getConfigurationFromPast(deviceId, timepoint):
    """Calls slotGetConfigurationFromPast on dataLoglReader"""
    timepoint = _parse_date(timepoint)
    return _getConfigurationFromPast(deviceId, timepoint)


def plotLoggedData(data, begin, end):
    """
    Plots acquired from log data in a given time interval

    :param d: an AcquiredFromLog object with loaded data
    :param begin: date and time when the required time interval starts
    :param end: date and time when the required time interval ends
    :return: data formatted in a conveinient form to plot with matplotlib, i.e.
            a dictionary which keys are plot names with the form
            'deviceId.property', and values are in turn dictionaries
            with 'value' and 'timestamp' as keys and list of data as values

    begin and end parameters are time string in the form
    "2009-09-01T15:32:12 UTC", but any part of the string can be omitted,
    like "10:32". Only giving the time, where we assume the current day.
    Unless specified otherwise, your local timezone is assumed.
    If matplotlib backend is set properly (e.g. %matplotlib qt) charts are
    drown in a default plot.

    """

    if get_ipython():
        get_ipython().run_line_magic('matplotlib', 'qt4')

    begin = datetime.datetime.strptime(_parse_date(begin), DATE_FORMAT)
    end = datetime.datetime.strptime(_parse_date(end), DATE_FORMAT)

    data4plots = {}

    for d in data:
        ts = datetime.datetime.fromtimestamp(d.get('timestamp'))
        if ts < begin or ts > end:
            print(ts, "out of range", begin, end)
            continue

        did = d.get('deviceId')
        propname = d.getKeys()[3]

        plotname = "{}.{}".format(did, propname)
        if plotname not in data4plots.keys():
            data4plots[plotname] = {'timestamp': [], 'value': []}

        data4plots[plotname]['timestamp'].append(ts)
        data4plots[plotname]['value'].append(d.get(propname))

    # draw default plot
    ax = None
    fig = pl.figure(figsize=(8, 6))
    ax = fig.add_axes([0.1, 0.1, 0.8, 0.8])

    for plotname in data4plots.keys():
        ax.plot_date(data4plots[plotname]['timestamp'],
                     data4plots[plotname]['value'], label=plotname)

        ax.xaxis.set_major_locator(mdates.AutoDateLocator())
        ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y/%m/%d %H:%M"))
        for l in ax.get_xticklabels():
            l.set_rotation(45)
        ax.legend()
        pl.grid(b=True, which='major', color='b', linestyle=':')
        pl.title(data.experimentId)

    # return easy to plot data
    return data4plots
