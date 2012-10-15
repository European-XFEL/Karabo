'''
Created on Jul 27, 2012

@author: esenov
'''
import unittest
from fsm import EXFEL_FSM_NO_TRANSITION_ACTION, EXFEL_FSM_EVENT2, EXFEL_FSM_EVENT0, EXFEL_FSM_EVENT1, EXFEL_FSM_STATE
from fsm import EXFEL_FSM_STATE_EE, EXFEL_FSM_ACTION0, EXFEL_FSM_ACTION1, EXFEL_FSM_ACTION2, EXFEL_FSM_GUARD1, EXFEL_FSM_STATE_MACHINE
from fsm import EXFEL_FSM_CREATE_MACHINE, EXFEL_FSM_INTERRUPT_STATE_EE
from fsm import event_instance

class StateMachineTest(object):
    '''
    This state machine was copied from Burkhard's C++ StateMachineTest of core package
    '''


    def __init__(self):
        '''
        Constructor used to describe the state machine
        '''
        
        EXFEL_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
                    
        #**************************************************************
        #*                        Events                              *
        #**************************************************************

        EXFEL_FSM_EVENT2('ErrorFoundEvent', self.onException, str, str)

        EXFEL_FSM_EVENT0('EndErrorEvent', self.endErrorEvent)

        EXFEL_FSM_EVENT0('GoToB', self.goToB)
            
        EXFEL_FSM_EVENT1('GoToA', self.goToA, int)
            
        EXFEL_FSM_EVENT0('GoToD', self.goToD)

        #**************************************************************
        #*                        States                              *
        #**************************************************************

        EXFEL_FSM_INTERRUPT_STATE_EE('Error', 'EndErrorEvent', self.errorStateOnEntry, self.errorStateOnExit)
            
        EXFEL_FSM_STATE('AllOk')

        EXFEL_FSM_STATE_EE('A', self.aOnEntry, self.aOnExit)
            
        #EXFEL_FSM_STATE_EE('B', self.bOnEntry, self.bOnExit)
            
        EXFEL_FSM_STATE_EE('C', self.cOnEntry, self.cOnExit)
            
        EXFEL_FSM_STATE_EE('D', self.dOnEntry, self.dOnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************

        EXFEL_FSM_ACTION0('A2BAction', self.a2BAction)
            
        EXFEL_FSM_ACTION1('B2AAction', self.b2AAction, int)

        EXFEL_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)

        EXFEL_FSM_ACTION0('EndErrorAction', self.endErrorAction)
            
        #**************************************************************
        #*                        Guards                              *
        #**************************************************************

        EXFEL_FSM_GUARD1('GoToAGuard', self.goToAGuard, int)
            
            
        #**************************************************************
        #*                      Sub Machine                           *
        #**************************************************************
        BTransitionTable = [
                             #('C', 'GoToD', 'D', 'none', 'none')
                             ('C', 'none', 'D', 'none', 'none')
                            ]
            
        EXFEL_FSM_STATE_MACHINE('B', BTransitionTable, 'C')
            

        #**************************************************************
        #*                      Top Machine                           *
        #**************************************************************

        TestDeviceMachineTransitionTable = [
             #  Source    Event             Target    Action              Guard
             ('A', 'GoToB', 'B', 'A2BAction', 'none'),
             ('B', 'GoToA', 'A', 'none', 'GoToAGuard'),
             ('AllOk', 'ErrorFoundEvent', 'Error', 'ErrorFoundAction', 'none'),
             ('Error', 'EndErrorEvent', 'AllOk', 'EndErrorAction', 'none')
            ]

        #                            Name            Transition-Table                  Initial-State 
        EXFEL_FSM_STATE_MACHINE('TestDeviceMachine', TestDeviceMachineTransitionTable, ('AllOk', 'A'))
        
        self.fsm = EXFEL_FSM_CREATE_MACHINE('TestDeviceMachine')
    
        
    def noStateTransition(self): pass
    def onException(self): pass
    def endErrorEvent(self): pass
    def goToB(self): pass
    def goToA(self, i): pass
    def goToD(self): pass
    def errorStateOnEntry(self): pass
    def errorStateOnExit(self): pass
    def aOnEntry(self): pass
    def aOnExit(self): pass
    def a2BAction(self): pass
    def b2AAction(self, i): pass
    def errorFoundAction(self, m1, m2): pass
    def endErrorAction(self): pass
    def goToAGuard(self, i): return True
    def cOnEntry(self): pass
    def cOnExit(self): pass
    def dOnEntry(self): pass
    def dOnExit(self): pass
    

def pstate(fsm):
    print '>>>>>>>>>>>>>>>>', fsm.get_state(), '<<<<<<<<<<<<<<<<'
        
    
class TestStateMachineTest(StateMachineTest):
    
    def __init__(self):
        super(TestStateMachineTest, self).__init__()
        
    def noStateTransition(self):
        print "No transition"
    
    def onException(self):
        print '==============> StateMachineTest.onException()'
    
    def endErrorEvent(self):
        print '==============> StateMachineTest.endErrorEvent'
    
    def goToB(self):
        print '==============> StateMachineTest.goToB()'
    
    def goToA(self, i):
        print '==============> StateMachineTest.goToA()'
    
    def goToD(self):
        print '==============> StateMachineTest.goToD()'
    
    def errorStateOnEntry(self):
        print '==============> StateMachineTest.errorStateOnEntry()'
    
    def errorStateOnExit(self):
        print '==============> StateMachineTest.errorStateOnExit()'
    
    def aOnEntry(self):
        print '==============> StateMachineTest.aOnEntry()'
    
    def aOnExit(self):
        print '==============> StateMachineTest.aOnExit()'
    
    def a2BAction(self):
        print '==============> StateMachineTest.a2BAction()'
    
    def b2AAction(self, i):
        print '==============> StateMachineTest.b2AAction(', i, ')'
    
    def errorFoundAction(self, m1, m2):
        print '**************> Error:', m1, '--', m2
        
    def endErrorAction(self):
        print '==============> StateMachineTest.endErrorAction()'
    
    def goToAGuard(self, i):
        print '==============> StateMachineTest.goToAGuard'
        return True
    def cOnEntry(self):
        print '==============> StateMachineTest.cOnEntry()'
        
    def cOnExit(self):
        print '==============> StateMachineTest.cOnExit()'
        
    def dOnEntry(self):
        print '==============> StateMachineTest.dOnEntry()'
        
    def dOnExit(self):
        print '==============> StateMachineTest.dOnExit()'
        

    def test(self):
        s = self.fsm
        print '-- start top state machine'
        s.start()
        pstate(s)
        print '-- enter "error"'
        s.process_event(event_instance('ErrorFoundEvent', ('Something wrong...', 'Dont worry, this is a test :)')))
        pstate(s)
        print '-- try to enter "go to B" while in error'
        s.process_event(event_instance('GoToB', ()))
        pstate(s)
        print '-- enter "end error"'
        s.process_event(event_instance('EndErrorEvent', ()))
        pstate(s)
        print '-- enter "go to B" again'
        s.process_event(event_instance('GoToB', ()))
        pstate(s)
        #s.process_event(event_instance('GoToD', ()))
        #pstate(s)
        print '-- enter "error"'
        s.process_event(event_instance('ErrorFoundEvent', ('Just', 'a test')))
        pstate(s)
        print '-- enter "go to A"'
        s.process_event(event_instance('GoToA', (42,)))
        pstate(s)
        print '-- enter "end error"'
        s.process_event(event_instance('EndErrorEvent', ()))
        pstate(s)
        print '-- enter "go to A"'
        s.process_event(event_instance('GoToA', (42,)))
        pstate(s)
        print '-- enter "stop"'
        s.stop()
        


class Test(unittest.TestCase):


    def testStateMachineTest(self):
        print "\n================================= testStateMachineTest =================================\n"
        a = TestStateMachineTest()
        a.test()


if __name__ == "__main__":
    #import sys;sys.argv = ['', 'Test.testStateMachineTest']
    unittest.main()