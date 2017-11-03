#############################################################################
# Author: <chen.xu@xfel.eu>
# Created on November 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import deque
from itertools import product

import numpy as np
from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import QPushButton, QHBoxLayout, QVBoxLayout, QWidget

from karabo_gui.const import FINE_COLOR, MAXNUMPOINTS
from karabo_gui.mplwidget.mplplotwidgets import MplCurvePlot
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import Bool, Simple

BUTTON_SIZE = (52, 32)


class MultiCurvePlot(DisplayWidget):
    category = Bool, Simple
    alias = "Multi-Curve Plot"

    def __init__(self, box, parent):
        super(MultiCurvePlot, self).__init__(None)
        self.xbox = None
        self.resetbox = None
        self.yboxes = []

        self.xvalues = deque(maxlen=MAXNUMPOINTS)
        self.yvalues = {}  # {box.key(): deque of values}
        self.lastvalues = {}  # used to synchronize x and y values
        self.draw_start = False  # indicate there is at least one x-y pair

        self.widget = QWidget(parent=parent)
        vbox = QVBoxLayout(self.widget)
        self.mplwidget = MplCurvePlot(parent=self.widget, legend=True)
        vbox.addWidget(self.mplwidget)

        hbox = QHBoxLayout()
        hbox.addStretch(1)  # push the button to the right side
        _btn_reset = QPushButton('Reset')
        _btn_reset.setFixedSize(*BUTTON_SIZE)
        _btn_reset.setFocusPolicy(Qt.NoFocus)
        _btn_reset.clicked.connect(self._reset_plot)
        hbox.addWidget(_btn_reset)
        vbox.addLayout(hbox)

        # A closure to change the button sytle once a boolean is linked
        def _changestyle():
            objname = str(id(self))
            _btn_reset.setObjectName(objname)
            sheet = ("QPushButton#{} {{ background-color : rgba{}; }}"
                     "".format(objname, FINE_COLOR))
            _btn_reset.setStyleSheet(sheet)
        self._resetbox_linked = _changestyle

        self.addBox(box)

    def addBox(self, box):
        desc = box.descriptor
        if self.resetbox is None and isinstance(desc, Bool):
            # if the resetbox is set, new boolean box goes to yvalues
            self.resetbox = box
            self._resetbox_linked()
            return True
        if box in self.lastvalues:
            return False
        self.lastvalues[box] = None
        # Don't allow boolean type in x axis
        if self.xbox is None and not isinstance(desc, Bool):
            self.xbox = box
            self.mplwidget.axes_call('set_xlabel', box.axisLabel())
        else:
            self.yboxes.append(box)
            # request a new curve for each new ybox, use box.key() for the
            # curve name as this is unique
            name = box.key()
            self.mplwidget.new_curve(label=name)
            self.yvalues[name] = deque(maxlen=MAXNUMPOINTS)
            self.draw_start = True
        return True

    @property
    def boxes(self):
        boxes = [self.xbox]
        if self.resetbox is not None:
            boxes.append(self.resetbox)
        if self.yboxes:
            boxes.extend(self.yboxes)
        return boxes

    def valueChanged(self, box, value, timestamp=None):
        if box is self.resetbox:
            self._reset_plot(reset=value)
            return
        synchronizer = self.lastvalues
        synchronizer[box] = value
        if self.draw_start and None not in synchronizer.values():
            self._update_curve()

    # -------------------------------------------------------------------
    # private functions

    @pyqtSlot()
    def _reset_plot(self, reset=True):
        """clear the plot"""
        if not reset:
            # when adding a boolean as resetbox, it may send a False value
            return
        sync = self.lastvalues
        self.lastvalues = sync.fromkeys(sync.keys(), None)
        self.xvalues.clear()
        for name, arr in self.yvalues.items():
            arr.clear()
            self.mplwidget.update_curve(self.xvalues, arr, label=name)

    def _update_curve(self):
        """Update all curves, clear lastvalues buffer (set all values to None).
        This is called when xbox and all y boxes produce a not None value.
        """
        lastvalues = self.lastvalues
        for box, value in lastvalues.items():
            if box is self.xbox:
                self.xvalues.append(value)
            else:
                self.yvalues[box.key()].append(value)

        # product pairs xvalues to each ('name', yvalues)
        for xdata, (label, ydata) in product((self.xvalues,),
                                             self.yvalues.items()):
            # later added y may have shorter length
            npx, npy = _convert_data(xdata, ydata)
            self.mplwidget.update_curve(npx, npy, label=label)
        self.lastvalues = lastvalues.fromkeys(lastvalues.keys(), None)


def _convert_data(xdata, ydata):
    """Convert x y deque array to numpy array for plotting.
    """
    # There might be np.nan in the array
    npx = np.array(xdata, dtype=np.float)
    npy = np.array(ydata, dtype=np.float)
    if npx.size != npy.size:
        # trim npx if necessary, len(npx) >= len(npy) always True
        npx = npx[-npy.size:]
    # get a mask to get rid of np.nan in y
    mask = np.isfinite(npy)
    return npx[mask], npy[mask]
