# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import (
    KARABO_FSM_ACTION0, KARABO_FSM_ACTION2, KARABO_FSM_CREATE_MACHINE,
    KARABO_FSM_EVENT0, KARABO_FSM_EVENT2, KARABO_FSM_GUARD0,
    KARABO_FSM_NO_TRANSITION_ACTION, KARABO_FSM_STATE, KARABO_FSM_STATE_E,
    KARABO_FSM_STATE_EE, KARABO_FSM_STATE_MACHINE, KARABO_FSM_STATE_MACHINE_E,
    KARABO_FSM_STATE_MACHINE_EE, State)


class HvMachine(object):
    '''
    State machine used in HV Mpod projects
    '''

    def __init__(self):
        '''
        Constructor serves as a place for State machine description.
        It is recommended to follow the following order:
        - events
        - actions
        - guards
        - simple states
        - transition table
        - state machine
        - transition table
        - state machine
        ...
        - top state machine
        '''
        # events

        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'EndErrorEvent', 'endError')
        KARABO_FSM_EVENT0(self, 'SwitchOnEvent', 'slotSwitchOnEvent')
        KARABO_FSM_EVENT0(self, 'SwitchOffEvent', 'slotSwitchOffEvent')
        KARABO_FSM_EVENT0(self, 'VoltChangingEvent', 'slotVoltChangingEvent')
        KARABO_FSM_EVENT0(self, 'LevelReachedEvent', 'slotLevelReachedEvent')

        # actions
        KARABO_FSM_ACTION0('initAction', self.initAction)
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('actionInError', self.actionInError)
        KARABO_FSM_ACTION0('VoltChangingAction', self.voltChangingAction)
        KARABO_FSM_ACTION0('LevelReachedAction', self.levelReachedAction)

        KARABO_FSM_NO_TRANSITION_ACTION(self.noTransition)

        # states
        KARABO_FSM_STATE_EE(State.INIT, self.initStateEntry,
                            self.initStateExit)
        KARABO_FSM_STATE_EE(State.ERROR, self.errorStateEntry,
                            self.errorStateExit)
        KARABO_FSM_STATE_E(State.OFF, self.offStateEntry)
        KARABO_FSM_STATE_EE(State.CHANGING, self.changingStateEntry,
                            self.changingStateExit)
        KARABO_FSM_STATE(State.STATIC)

        onStt = [
            (State.STATIC, 'VoltChangingEvent', State.CHANGING,
             'VoltChangingAction', 'none'),
            (State.CHANGING, 'LevelReachedEvent', State.STATIC,
             'LevelReachedAction', 'none')]

        KARABO_FSM_STATE_MACHINE_EE(State.ON, onStt, State.CHANGING,
                                    self.onStateEntry, self.onStateExit)
        # guards
        KARABO_FSM_GUARD0('SwitchOffGuard', self.switchOffGuard)
        KARABO_FSM_GUARD0('SwitchOnGuard', self.switchOnGuard)

        allOkStt = [
            (State.OFF, 'VoltChangingEvent', State.ON,
             'none', 'SwitchOnGuard'),
            (State.OFF, 'SwitchOnEvent', State.ON, 'none', 'SwitchOnGuard'),
            (State.ON, 'SwitchOffEvent', State.OFF, 'none', 'SwitchOffGuard')]

        KARABO_FSM_STATE_MACHINE(State.NORMAL, allOkStt, State.OFF)

        hvStt = [
            (State.INIT, 'none', State.NORMAL, 'initAction', 'none'),
            (State.NORMAL, 'ErrorFoundEvent', State.ERROR,
             'ErrorFoundAction', 'none'),
            (State.ERROR, 'EndErrorEvent', State.NORMAL, 'none', 'none'),
            (State.ERROR, 'ErrorFoundEvent', None, 'actionInError', 'none'),
            (State.ERROR, 'VoltChangingEvent', None, 'none', 'none')]

        KARABO_FSM_STATE_MACHINE_E('MpodDeviceMachine', hvStt, State.INIT,
                                   self.initializeHardware)

        self.fsm = KARABO_FSM_CREATE_MACHINE('MpodDeviceMachine')

    # default action methods
    def initAction(self):
        pass

    def errorFoundAction(self, m1, m2):
        pass

    def actionInError(self):
        pass

    def voltChangingAction(self):
        pass

    def levelReachedAction(self):
        pass

    def noTransition(self):
        pass

    # default guard methods
    def switchOffGuard(self):
        return True

    def switchOnGuard(self):
        return True

    # default state entry/exit methods
    def initStateEntry(self):
        pass

    def initStateExit(self):
        pass

    def offStateEntry(self):
        pass

    def changingStateEntry(self):
        pass

    def changingStateExit(self):
        pass

    def onStateEntry(self):
        pass

    def onStateExit(self):
        pass

    def initializeHardware(self):
        pass

    def errorStateEntry(self):
        pass

    def errorStateExit(self):
        pass

    def processEvent(self, event):
        self.fsm.process_event(event)


class Hvmachine_TestCase(unittest.TestCase):
    def setUp(self):
        self.hv = HvMachine()

    def tearDown(self):
        #    self.foo.dispose()
        self.hv = None

    def test_hvmachine_(self):
        fsm = self.hv.fsm
        fsm.start()
        self.assertIs(fsm.get_state(), State.OFF)
        self.hv.slotSwitchOnEvent()
        self.assertIs(fsm.get_state(), State.CHANGING)
        self.hv.slotLevelReachedEvent()
        self.assertIs(fsm.get_state(), State.STATIC)
        self.hv.slotVoltChangingEvent()
        self.assertIs(fsm.get_state(), State.CHANGING)
        self.hv.slotLevelReachedEvent()
        self.assertIs(fsm.get_state(), State.STATIC)
        self.hv.slotVoltChangingEvent()
        self.assertIs(fsm.get_state(), State.CHANGING)
        self.hv.errorFound('Timeout happened', 'Hardware device not responded')
        self.assertIs(fsm.get_state(), State.ERROR)
        self.hv.errorFound('Exception happened',
                           'Another error while in Error')
        self.assertIs(fsm.get_state(), State.ERROR)
        self.hv.endError()
        self.assertIs(fsm.get_state(), State.OFF)
        self.hv.slotVoltChangingEvent()
        self.assertIs(fsm.get_state(), State.CHANGING)
        self.hv.slotLevelReachedEvent()
        self.assertIs(fsm.get_state(), State.STATIC)
        self.hv.slotSwitchOffEvent()
        self.assertIs(fsm.get_state(), State.OFF)
        # print fsm.get_state()
        # self.fail("TODO: Write test")


if __name__ == '__main__':
    unittest.main()
