import csv
from datetime import datetime

from PyQt4.QtGui import QAction, QInputDialog, QPushButton

from karabo.middlelayer import Simple, String

from karabo_gui.util import getSaveFileName
from karabo_gui.widget import DisplayWidget


class Monitor(DisplayWidget):
    category = Simple, String
    alias = "Monitor"

    def __init__(self, box, parent):
        super().__init__(None)
        self.widget = QPushButton("Monitor", parent)
        self.widget.setCheckable(True)
        self.widget.toggled.connect(self.toggled)
        self.boxes = [box]
        self.interval = 0

        action = QAction("Set Filename...", self.widget)
        action.triggered.connect(self.setFilename)
        self.widget.addAction(action)
        action = QAction("Set Interval...", self.widget)
        action.triggered.connect(self.setInterval)
        self.widget.addAction(action)
        self.interval = 0
        self.timer = None
        self.filename = None

    def setFilename(self):
        fn = getSaveFileName(
                caption="Filename for Monitors",
                filter="Comma-separated value files (*.csv)",
                suffix="csv")
        if fn:
            self._setFilename(fn)

    def setInterval(self):
        interval, ok = QInputDialog.getDouble(
            self.widget, "Monitor Interval",
            "Enter the interval (in seconds) the monitor should be saved\n"
            "Enter 0 if it should be triggered by first property")
        if ok:
            self._setInterval(interval)

    def addBox(self, box):
        if self.isCompatible(box, True):
            self.boxes.append(box)
            return True
        return False

    def valueChanged(self, box, value, ts):
        if (self.interval == 0 and self.widget.isChecked() and
                box is self.boxes[0]):
            self.timerEvent(ts=ts)

    def timerEvent(self, event=None, ts=None):
        if ts is None:
            ts = datetime.now().isoformat()
        else:
            ts = ts.toLocal()
        self.writer.writerow([ts] + [b.value for b in self.boxes])

    def toggled(self, checked):
        if checked:
            self.file = open(self.filename, "a", newline="\r\n")
            self.writer = csv.writer(self.file)
            self.writer.writerow(["timestamp"] + [b.key() for b in self.boxes])
            if self.interval > 0:
                self.timer = self.startTimer(self.interval * 1000)
        else:
            if self.timer is not None:
                self.killTimer(self.timer)
            self.file.close()

    def _setFilename(self, filename):
        """ Give derived classes a place to respond to changes. """
        self.filename = filename

    def _setInterval(self, interval):
        """ Give derived classes a place to respond to changes. """
        self.interval = interval
