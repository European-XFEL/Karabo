from karabo.common.states import State
from karabo_gui.widget import VacuumWidget


class LampWidget(VacuumWidget):
    alias = "Generic Lamp"
    statePixmapName = {
        State.CHANGING: 'lamp-changing',
        State.ACTIVE: 'lamp-active',
        State.PASSIVE: 'lamp-passive',
        State.STATIC: 'lamp-static',
        State.INIT: 'lamp-init',
        State.KNOWN: 'lamp-known',
        State.ERROR: 'lamp-error',
        State.UNKNOWN: 'lamp-unknown',
        State.DISABLED: 'lamp-disabled'
    }
