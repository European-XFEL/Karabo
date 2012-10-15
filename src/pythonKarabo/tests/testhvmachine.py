'''
Created on Jul 27, 2012

@author: esenov
'''
import unittest
from fsm import EXFEL_FSM_EVENT0, EXFEL_FSM_EVENT2, EXFEL_FSM_ACTION0, EXFEL_FSM_ACTION2, EXFEL_FSM_NO_TRANSITION_ACTION
from fsm import EXFEL_FSM_STATE, EXFEL_FSM_STATE_E, EXFEL_FSM_STATE_EE, EXFEL_FSM_STATE_MACHINE_EE, EXFEL_FSM_GUARD0
from fsm import EXFEL_FSM_STATE_MACHINE, EXFEL_FSM_STATE_MACHINE_E, EXFEL_FSM_CREATE_MACHINE
from fsm import event_instance

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
        
        EXFEL_FSM_EVENT2('ErrorFoundEvent', self.errorFound, str, str)  # Possible parameter types: int,float,bool,list,dict,tuple,instance
        EXFEL_FSM_EVENT0('EndErrorEvent', self.endError)
        EXFEL_FSM_EVENT0('SwitchOnEvent', self.slotSwitchOnEvent)
        EXFEL_FSM_EVENT0('SwitchOffEvent', self.slotSwitchOffEvent)
        EXFEL_FSM_EVENT0('VoltChangingEvent', self.slotVoltChangingEvent)
        EXFEL_FSM_EVENT0('LevelReachedEvent', self.slotLevelReachedEvent)
        
        # actions
        EXFEL_FSM_ACTION2('ErrorFoundAction',   self.errorFoundAction, str, str)
        EXFEL_FSM_ACTION0('VoltChangingAction', self.voltChangingAction)
        EXFEL_FSM_ACTION0('LevelReachedAction', self.levelReachedAction)
        
        EXFEL_FSM_NO_TRANSITION_ACTION(self.noTransition)
        
        # states
        EXFEL_FSM_STATE('ErrorState')
        EXFEL_FSM_STATE_E('OffState', self.offStateEntry)
        EXFEL_FSM_STATE_EE('ChangingState', self.changingStateEntry, self.changingStateExit)
        EXFEL_FSM_STATE('StableState')
        # STT
        onStt = [
                    ('StableState',   'VoltChangingEvent', 'ChangingState', 'VoltChangingAction', 'none'),
                    ('ChangingState', 'LevelReachedEvent', 'StableState',   'LevelReachedAction', 'none')
                ]
        # state machine             State     STT    Initial          on_entry      on_exit
        EXFEL_FSM_STATE_MACHINE_EE('OnState', onStt, 'ChangingState', self.onStateEntry, self.onStateExit)
        # guards
        EXFEL_FSM_GUARD0('SwitchOffGuard', self.switchOffGuard)
        EXFEL_FSM_GUARD0('SwitchOnGuard', self.switchOnGuard)
        # STT
        allOkStt = [
                    ('OffState', 'VoltChangingEvent', 'OnState',  'none',  'SwitchOnGuard'),
                    ('OffState', 'SwitchOnEvent',     'OnState',  'none',  'SwitchOnGuard'),
                    ('OnState', 'SwitchOffEvent',     'OffState', 'none',  'SwitchOffGuard')
                   ]
        # state machine
        EXFEL_FSM_STATE_MACHINE('AllOkState', allOkStt, 'OffState')
        # STT
        hvStt = [
                    ('AllOkState', 'ErrorFoundEvent',   'ErrorState', 'ErrorFoundAction',  'none'),
                    ('ErrorState', 'EndErrorEvent',     'AllOkState', 'none',              'none'),
                    ('ErrorState', 'ErrorFoundEvent',   'none',       'none',              'none'),
                    ('ErrorState', 'VoltChangingEvent', 'none',       'none',              'none')
                ]
        EXFEL_FSM_STATE_MACHINE_E('MpodDeviceMachine', hvStt, 'AllOkState', self.initializeHardware)

        self.fsm = EXFEL_FSM_CREATE_MACHINE('MpodDeviceMachine')
     
        
        
    #  default event methods
    def errorFound(self, m1, m2):    pass
    def endError(self):              pass
    def slotSwitchOnEvent(self):     pass
    def slotSwitchOffEvent(self):    pass
    def slotVoltChangingEvent(self): pass
    def slotLevelReachedEvent(self): pass
    
    # default action methods
    def errorFoundAction(self, m1, m2): print 'Error found:', m1, m2
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
        
    

class TestMachine(HvMachine):
    def __init__(self):
        super(TestMachine,self).__init__()
        print 'TestMachine constructor'
    
    # actions
    def voltChangingAction(self): print "******> TestMachine.voltChangingAction()"
    def levelReachedAction(self): print "******> TestMachine.levelReachedAction()"
    def noTransition(self):       print "*> TestMachine.noTransition()"
    
    def offStateEntry(self):      print "------> TestMachine.offStateEntry()"
    def changingStateEntry(self): print "---------> TestMachine.changingStateEntry()"
    def changingStateExit(self):  print "---------> TestMachine.changingStateExit()"
    def onStateEntry(self):       print "------> TestMachine.onStateEntry()"
    def onStateExit(self):        print "------> TestMachine.onStateExit()"
    def initializeHardware(self): print "-> TestMachine.initializeHardware()"

    def test(self):
        self.fsm.start()
        print "1.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('SwitchOnEvent', ()))
        print "2.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('LevelReachedEvent', ()))
        print "3.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('VoltChangingEvent', ()))
        print "4.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('LevelReachedEvent', ()))
        print "5.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('VoltChangingEvent', ()))
        print "6.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('ErrorFoundEvent',('Timeout happened', 'Hardware device not responded')))
        print "7.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('EndErrorEvent', ()))
        print "8.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('VoltChangingEvent', ()))                      
        print "9.  Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('LevelReachedEvent', ()))
        print "10. Current state is", self.fsm.get_state()
        self.fsm.process_event(event_instance('SwitchOffEvent',()))
        print "11. Current state is", self.fsm.get_state()


class Test(unittest.TestCase):

    def testHvMachine(self):
        print "\n================================= testHvMachine =================================\n"
        m = TestMachine()
        m.test()


if __name__ == "__main__":
    #import sys;sys.argv = ['', 'Test.testHvMachine']
    unittest.main()