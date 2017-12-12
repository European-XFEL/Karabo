import os.path

from PyQt4.QtGui import QLabel, QPixmap

from karabo.middlelayer import State, String
from karabo_gui import icons
from karabo_gui.widget import DisplayWidget


class LampWidget(DisplayWidget):
    alias = "Generic Lamp"
    category = String
    statePixmapName = {
        State.CHANGING: 'lamp-changing',
        State.RUNNING: 'lamp-running',
        State.ACTIVE: 'lamp-active',
        State.PASSIVE: 'lamp-passive',
        State.STATIC: 'lamp-static',
        State.INIT: 'lamp-init',
        State.NORMAL: 'lamp-known',
        State.KNOWN: 'lamp-known',
        State.ERROR: 'lamp-error',
        State.UNKNOWN: 'lamp-unknown',
        State.DISABLED: 'lamp-disabled'
    }

    def __init__(self, box, parent):
        DisplayWidget.__init__(self, box)

        self.widget = QLabel(parent)
        self.widget.setScaledContents(True)

    value = None

    @classmethod
    def isCompatible(cls, box, readonly):
        return box.path == ("state",) and super().isCompatible(box, readonly)

    def _setPixmap(self, name):
        p = QPixmap(os.path.join(os.path.dirname(icons.__file__), name))
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setPixmap(self.statePixmapName[State.CHANGING])
        elif State(value).isDerivedFrom(State.RUNNING):
            self._setPixmap(self.statePixmapName[State.RUNNING])
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setPixmap(self.statePixmapName[State.ACTIVE])
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setPixmap(self.statePixmapName[State.PASSIVE])
        elif State(value).isDerivedFrom(State.DISABLED):
            self._setPixmap(self.statePixmapName[State.DISABLED])
        elif State(value) is State.STATIC:
            self._setPixmap(self.statePixmapName[State.STATIC])
        elif State(value) is State.NORMAL:
            self._setPixmap(self.statePixmapName[State.NORMAL])
        elif State(value) is State.ERROR:
            self._setPixmap(self.statePixmapName[State.ERROR])
        elif State(value) is State.INIT:
            self._setPixmap(self.statePixmapName[State.INIT])
        else:
            self._setPixmap(self.statePixmapName[State.UNKNOWN])
