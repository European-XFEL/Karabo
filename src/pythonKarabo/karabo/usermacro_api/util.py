"""This module contains helper routines"""
import collections
from .middlelayer import (
    _parse_date, get_instance, KaraboError, synchronize)

# imports used for plotLoggedData
import datetime
import matplotlib.pyplot as plt
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


def plotLoggedData(data, begin=None, end=None):
    """
    Plots logged scan data, optionally in a limited time interval

    :param d: an AcquiredFromLog object with loaded data
    :param begin: date and time when the required time interval starts
    :param end: date and time when the required time interval ends
    :return: data formatted in a convenient form to plot with matplotlib, i.e.
            a dictionary which keys are plot names with the form
            'deviceId.property', and values are in turn dictionaries
            with 'value' and 'timestamp' as keys and list of data as values

    If begin is not given scan data from the scan begin are plotted.
    If end is not given scan data up to scan end are plotted.
    begin and end parameters are time string in the form
    "2009-09-01T15:32:12 UTC", but any part of the string can be omitted,
    like "10:32". Only giving the time, where we assume the current day.
    Unless specified otherwise, your local timezone is assumed.
    If matplotlib backend is set properly (e.g. %matplotlib qt) charts are
    drawn in a default plot.
    
    usage example of returned dictionary:
    pd = plotLoggedData(acquired_data)
    x = pd['deviceId.property']['timestamp']
    y = pd['deviceId.property']['value']
    aPlotFunction(x,y)

    """

    if get_ipython():
        get_ipython().run_line_magic('matplotlib', 'qt4')

    if begin:
        begin = datetime.datetime.strptime(_parse_date(begin), DATE_FORMAT)
    if end:
        end = datetime.datetime.strptime(_parse_date(end), DATE_FORMAT)

    data4plots = {}

    for datum in data:
        ts = datetime.datetime.fromtimestamp(datum.get('timestamp'))

        if (begin and ts < begin) or (end and ts > end):
            continue

        did = datum.get('deviceId')
        propname = datum.getKeys()[3]

        plotname = "{}.{}".format(did, propname)

        plotdata = data4plots.setdefault(plotname, {'timestamp': [], 'value': []})
        plotdata['timestamp'].append(ts)
        plotdata['value'].append(datum.get(propname))

    # draw default plot
    ax = None
    fig = plt.figure()
    ax = fig.add_axes([0.1, 0.25, 0.8, 0.7])

    for plotname in data4plots.keys():
        ax.plot_date(data4plots[plotname]['timestamp'],
                     data4plots[plotname]['value'], label=plotname)

        ax.xaxis.set_major_locator(mdates.AutoDateLocator())
        ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y/%m/%d %H:%M:%S"))
        for l in ax.get_xticklabels():
            l.set_rotation(45)
        ax.legend()
        plt.grid(b=True, which='major', color='b', linestyle=':')
        plt.title(data.experimentId)

    # return easy to plot data
    return data4plots
