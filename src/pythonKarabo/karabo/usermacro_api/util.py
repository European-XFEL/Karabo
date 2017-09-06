"""This module contains helper routines"""
import collections

# imports used for plotLoggedData
import datetime
import itertools
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np
from IPython import get_ipython
DATE_FORMAT = "%Y-%m-%dT%H:%M:%S"

from .middlelayer import (
    _parse_date, get_instance, KaraboError, synchronize)


def splitTrajectory(pos_list, number_of_steps):
    """Generates a segmented trajectory

    :param pos_list: the trajectory to be segmented.
      It is a list whose elements are cartesian coordinates
      to be passed to the moveto(pos) method
      of a Movable.
      For example, the trajectory might be:

      [1,10] if the movable is a simple motor
      with a single degree of freedom

      or [(1,1,1), (2,2,10), (10,10,10)]
       in case of a (virtual) compound movable
        with three degrees of freedom.

    :param number_steps: the number of segments
      to split the trajectory into.

    This function generates breaking points as pos, boolean
    pairs that are either replicating the vectices in pos_list
    or new points the scan must pause on. The boolean if True
    indicates such a mandatory pause (i.e. a step marker).
    """
    fepsilon = 1e-3
    if number_of_steps == 0:
        # Path Scan case
        itpos = iter(pos_list)
        while True:
            # Instruct to pause at every point
            yield next(itpos), True
    else:
        # Step-scan case
        len_traj = 0
        itpos1, itpos2 = itertools.tee(iter(pos_list))
        pos = next(itpos1)

        # Compute the trajectory length
        for nextpos in itpos1:
            v = np.subtract(nextpos, pos)
            nv = np.linalg.norm(v)
            pos = nextpos
            len_traj += nv

        len_step = len_traj / number_of_steps

        # Yield segment edges and pause points
        pos = next(itpos2)
        nextpos = next(itpos2)
        dl = len_step
        yield pos, True

        while True:
            v = np.subtract(nextpos, pos)
            nv = np.linalg.norm(v)
            if abs(dl - nv) < fepsilon * dl:
                # Acquire at the edge of this segment
                p = nextpos
                dl = len_step
                yield p, True
                nextpos = next(itpos2)
            elif dl < nv:
                # Acquire in this segment
                v1 = v / nv
                p = pos + v1 * dl
                dl = len_step
                yield p, True
            else:
                # The step spans to the next segment
                p = nextpos
                if nv > fepsilon * dl:
                    # Only command significant motions
                    dl -= nv
                    yield p, False
                nextpos = next(itpos2)
            pos = p


def meshTrajectory(pos_list1, pos_list2):
    """Generates a mesh trajectory

       :param pos_list1: The first motion axis
       :param pos_list2: The second motion axis
       The resulting 2D trajectory looks like:
        *--*---*
               |
        *--*---*
        |
        *--*---*
        This differs from a cartesian product, which gives:
        *--*---*
        |
        *--*---*
        |
        *--*---*
        and mandatorily and uncessarily leads to a return
        the row start.
    """
    reverse = False
    for pos1 in pos_list1:
        for pos2 in reversed(pos_list2) if reverse else pos_list2:
            yield pos1, pos2
        reverse = not reverse


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


def plot(data, begin=None, end=None):
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
    pd = plot(acquired_data)
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

        plotdata = data4plots.setdefault(
            plotname, {'timestamp': [], 'value': []})
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
