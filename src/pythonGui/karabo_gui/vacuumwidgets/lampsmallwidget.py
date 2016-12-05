from karabo.common.states import State
from karabo_gui.widget import VacuumWidget


class LampSmallWidget(VacuumWidget):
    alias = "Generic LampSmall"
    statePixmapName = {
        State.CHANGING: 'lampsmall-changing',
        State.ACTIVE: 'lampsmall-active',
        State.PASSIVE: 'lampsmall-passive',
        State.INIT: 'lampsmall-init',
        State.KNOWN: 'lampsmall-known',
        State.ERROR: 'lampsmall-error',
        State.UNKNOWN: 'lampsmall-unknown',
        State.DISABLED: 'lampsmall-disabled'
    }
