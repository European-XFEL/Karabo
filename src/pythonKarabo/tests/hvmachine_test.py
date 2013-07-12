# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from karabo.fsm import KARABO_FSM_EVENT0, KARABO_FSM_EVENT2, KARABO_FSM_ACTION0, KARABO_FSM_ACTION2, KARABO_FSM_NO_TRANSITION_ACTION
from karabo.fsm import KARABO_FSM_STATE, KARABO_FSM_STATE_E, KARABO_FSM_STATE_EE, KARABO_FSM_STATE_MACHINE_EE, KARABO_FSM_GUARD0
from karabo.fsm import KARABO_FSM_STATE_MACHINE, KARABO_FSM_STATE_MACHINE_E, KARABO_FSM_CREATE_MACHINE
from karabo.fsm import event_instance

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
        
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent',   'errorFound')
        KARABO_FSM_EVENT0(self, 'EndErrorEvent',     'endError')
        KARABO_FSM_EVENT0(self, 'SwitchOnEvent',     'slotSwitchOnEvent')
        KARABO_FSM_EVENT0(self, 'SwitchOffEvent',    'slotSwitchOffEvent')
        KARABO_FSM_EVENT0(self, 'VoltChangingEvent', 'slotVoltChangingEvent')
        KARABO_FSM_EVENT0(self, 'LevelReachedEvent', 'slotLevelReachedEvent')
        
        # actions
        KARABO_FSM_ACTION2('ErrorFoundAction',   self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('VoltChangingAction', self.voltChangingAction)
        KARABO_FSM_ACTION0('LevelReachedAction', self.levelReachedAction)
        
        KARABO_FSM_NO_TRANSITION_ACTION(self.noTransition)
        
        # states
        KARABO_FSM_STATE('ErrorState')
        KARABO_FSM_STATE_E('OffState', self.offStateEntry)
        KARABO_FSM_STATE_EE('ChangingState', self.changingStateEntry, self.changingStateExit)
        KARABO_FSM_STATE('StableState')
        # STT
        onStt = [
                    ('StableState',   'VoltChangingEvent', 'ChangingState', 'VoltChangingAction', 'none'),
                    ('ChangingState', 'LevelReachedEvent', 'StableState',   'LevelReachedAction', 'none')
                ]
        # state machine             State     STT    Initial          on_entry      on_exit
        KARABO_FSM_STATE_MACHINE_EE('OnState', onStt, 'ChangingState', self.onStateEntry, self.onStateExit)
        # guards
        KARABO_FSM_GUARD0('SwitchOffGuard', self.switchOffGuard)
        KARABO_FSM_GUARD0('SwitchOnGuard', self.switchOnGuard)
        # STT
        allOkStt = [
                    ('OffState', 'VoltChangingEvent', 'OnState',  'none',  'SwitchOnGuard'),
                    ('OffState', 'SwitchOnEvent',     'OnState',  'none',  'SwitchOnGuard'),
                    ('OnState', 'SwitchOffEvent',     'OffState', 'none',  'SwitchOffGuard')
                   ]
        # state machine
        KARABO_FSM_STATE_MACHINE('AllOkState', allOkStt, 'OffState')
        # STT
        hvStt = [
                    ('AllOkState', 'ErrorFoundEvent',   'ErrorState', 'ErrorFoundAction',  'none'),
                    ('ErrorState', 'EndErrorEvent',     'AllOkState', 'none',              'none'),
                    ('ErrorState', 'ErrorFoundEvent',   'none',       'none',              'none'),
                    ('ErrorState', 'VoltChangingEvent', 'none',       'none',              'none')
                ]
        KARABO_FSM_STATE_MACHINE_E('MpodDeviceMachine', hvStt, 'AllOkState', self.initializeHardware)

        self.fsm = KARABO_FSM_CREATE_MACHINE('MpodDeviceMachine')
     
    
    # default action methods
    def errorFoundAction(self, m1, m2): pass
    def voltChangingAction(self): pass
    def levelReachedAction(self): pass

    def noTransition(self):       pass
    
    # default guard methods
    def switchOffGuard(self):     return True
    def switchOnGuard(self):      return True
    
    # default state entry/exit methods
    def offStateEntry(self):      pass
    def changingStateEntry(self): pass
    def changingStateExit(self):  pass
    def onStateEntry(self):       pass
    def onStateExit(self):        pass
    def initializeHardware(self): pass
        
    def processEvent(self, event):
        self.fsm.process_event(event)

class  Hvmachine_TestCase(unittest.TestCase):
    def setUp(self):
        self.hv = HvMachine()

    def tearDown(self):
    #    self.foo.dispose()
        self.hv = None

    def test_hvmachine_(self):
        fsm = self.hv.fsm
        fsm.start()
        self.assertEqual(fsm.get_state(), "AllOkState.OffState", "failure -- not 'AllOkState.OffState'")
        self.hv.slotSwitchOnEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.ChangingState", "failure -- not 'AllOkState.OnState.ChangingState'")
        self.hv.slotLevelReachedEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.StableState", "failure -- not 'AllOkState.OnState.StableState'")
        self.hv.slotVoltChangingEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.ChangingState", "failure -- not 'AllOkState.OnState.ChangingState'")
        self.hv.slotLevelReachedEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.StableState", "failure -- not 'AllOkState.OnState.StableState'")
        self.hv.slotVoltChangingEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.ChangingState", "failure -- not 'AllOkState.OnState.ChangingState'")
        self.hv.errorFound('Timeout happened', 'Hardware device not responded')
        self.assertEqual(fsm.get_state(), "ErrorState", "failure -- not 'ErrorState'")
        self.hv.endError()
        self.assertEqual(fsm.get_state(), "AllOkState.OffState", "failure -- not 'AllOkState.OffState'")
        self.hv.slotVoltChangingEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.ChangingState", "failure -- not 'AllOkState.OnState.ChangingState'")
        self.hv.slotLevelReachedEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OnState.StableState", "failure -- not 'AllOkState.OnState.StableState'")
        self.hv.slotSwitchOffEvent()
        self.assertEqual(fsm.get_state(), "AllOkState.OffState", "failure -- not 'AllOkState.OffState'")
        #print fsm.get_state()
        #self.fail("TODO: Write test")

if __name__ == '__main__':
    unittest.main()

