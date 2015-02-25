import csv
from datetime import datetime

from PyQt4.QtGui import QAction, QInputDialog, QPushButton

from karabo.hashtypes import Simple, String

from util import getSaveFileName
from widget import DisplayWidget


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
        fn = getSaveFileName("Filename for Monitors", suffix="csv",
                             filter="Comma-separated value files (*.csv)")
        if fn:
            self.filename = fn

    def setInterval(self):
        interval, ok = QInputDialog.getDouble(
            self.widget, "Monitor Interval",
            "Enter the interval (in seconds) the monitor should be saved\n"
            "Enter 0 if it should be triggered by first property")
        if ok:
            self.interval = interval

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

    def save(self, e):
        if self.filename is not None:
            e.set("filename", self.filename)
        e.set("interval", str(self.interval))

    def load(self, e):
        self.filename = e.get("filename", None)
        self.interval = float(e.get("interval"))
