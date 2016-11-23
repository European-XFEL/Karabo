from karabo.common.states import State
from karabo_gui.widget import VacuumWidget


class AgilentIonWidget(VacuumWidget):
    alias = "Agilent Ion Pump"
    statePixmapName = {
        State.CHANGING: 'agilent-pump-changing',
        State.ACTIVE: 'agilent-pump-active',
        State.PASSIVE: 'agilent-pump-passive',
        State.INIT: 'agilent-pump-init',
        State.KNOWN: 'agilent-pump-known',
        State.ERROR: 'agilent-pump-error',
        State.UNKNOWN: 'agilent-pump-unknown',
        State.DISABLED: 'agilent-pump-disabled'
    }
