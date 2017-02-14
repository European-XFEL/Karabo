
import os.path

from PyQt4.QtGui import QLabel, QPixmap

from karabo.middlelayer import State, String
from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
from karabo_gui import icons
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget


class LampWidget(DisplayWidget):
    alias = "Generic Lamp"
    category = String
    statePixmapName = {
        State.CHANGING: 'lamp-changing',
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

        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; }}")
        self.widget.setObjectName(objectName)
        self.widget.setScaledContents(True)
        self.setErrorState(False)

    value = None

    @classmethod
    def isCompatible(cls, box, readonly):
        return box.path == ("state",) and super().isCompatible(box, readonly)

    def _setPixmap(self, name):
        p = QPixmap(os.path.join(os.path.dirname(icons.__file__), name))
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())

    def setErrorState(self, isError):
        color = ERROR_COLOR_ALPHA if isError else OK_COLOR
        ss = self._styleSheet.format(color)
        self.widget.setStyleSheet(ss)

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setPixmap(self.statePixmapName[State.CHANGING])
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setPixmap(self.statePixmapName[State.ACTIVE])
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setPixmap(self.statePixmapName[State.PASSIVE])
        elif State(value).isDerivedFrom(State.NORMAL):
            self._setPixmap(self.statePixmapName[State.NORMAL])
        elif State(value) is State.ERROR:
            self._setPixmap(self.statePixmapName[State.ERROR])
        else:
            self._setPixmap(self.statePixmapName[State.UNKNOWN])
