# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from fsm import EXFEL_FSM_NO_TRANSITION_ACTION, EXFEL_FSM_EVENT2, EXFEL_FSM_EVENT0, EXFEL_FSM_INTERRUPT_STATE_EE
from fsm import EXFEL_FSM_STATE_EE, EXFEL_FSM_ACTION0, EXFEL_FSM_STATE_MACHINE, EXFEL_FSM_CREATE_MACHINE
from fsm import event_instance


class ParallelMachine(object):
    '''
    Example of state machine having hierarchical regions
    '''

    def __init__(self):
        '''
        Constructor
        '''
        EXFEL_FSM_NO_TRANSITION_ACTION(self.noStateTransition)

        #**************************************************************
        #*                        Events                              *
        #**************************************************************

        EXFEL_FSM_EVENT2('ErrorFoundA', self.onException, str, str)
        EXFEL_FSM_EVENT0('EndErrorA', self.endErrorEvent)
        EXFEL_FSM_EVENT2('ErrorFoundB', self.onException, str, str)
        EXFEL_FSM_EVENT0('EndErrorB', self.endErrorEvent)
        EXFEL_FSM_EVENT0('GoTo1', self.goto1Event)
        EXFEL_FSM_EVENT0('GoTo2', self.goto2Event)
        EXFEL_FSM_EVENT0('GoTo3', self.goto3Event)
        EXFEL_FSM_EVENT0('GoTo4', self.goto4Event)
        EXFEL_FSM_EVENT0('GoTo5', self.goto5Event)

        #**************************************************************
        #*                        States                              *
        #**************************************************************

        EXFEL_FSM_INTERRUPT_STATE_EE('ErrorA', 'EndErrorA', self.errorStateOnEntry, self.errorStateOnExit)
        EXFEL_FSM_INTERRUPT_STATE_EE('ErrorB', 'EndErrorB', self.errorStateOnEntry, self.errorStateOnExit)
            
        EXFEL_FSM_STATE_EE('AllOk', self.allOkOnEntry, self.allOkOnExit)

        EXFEL_FSM_STATE_EE('InitializeA', self.initializeAOnEntry, self.initializeAOnExit)
        EXFEL_FSM_STATE_EE('InitializeB', self.initializeBOnEntry, self.initializeBOnExit)
            
        EXFEL_FSM_STATE_EE('State1', self.state1OnEntry, self.state1OnExit)
        EXFEL_FSM_STATE_EE('State2', self.state2OnEntry, self.state2OnExit)
        EXFEL_FSM_STATE_EE('State3', self.state3OnEntry, self.state3OnExit)
        EXFEL_FSM_STATE_EE('State4', self.state4OnEntry, self.state4OnExit)
        EXFEL_FSM_STATE_EE('State5', self.state5OnEntry, self.state5OnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************

        EXFEL_FSM_ACTION0('InitializeAtoTaskA', self.initializeAtoTaskA)
        EXFEL_FSM_ACTION0('InitializeBtoTaskB', self.initializeBtoTaskB)
            
        EXFEL_FSM_ACTION0('State1toState2', self.state1toState2)
        EXFEL_FSM_ACTION0('State2toState1', self.state2toState1)
        EXFEL_FSM_ACTION0('State2toState3', self.state2toState3)
        EXFEL_FSM_ACTION0('State3toState2', self.state3toState2)
        EXFEL_FSM_ACTION0('State4toState5', self.state4toState5)
        EXFEL_FSM_ACTION0('State5toState4', self.state5toState4)

        EXFEL_FSM_ACTION0('ErrorFoundAAction', self.errorFoundAAction)
        EXFEL_FSM_ACTION0('ErrorFoundBAction', self.errorFoundBAction)
        EXFEL_FSM_ACTION0('EndErrorAAction', self.endErrorAAction)
        EXFEL_FSM_ACTION0('EndErrorBAction', self.endErrorBAction)
            
        #**************************************************************
        #*                        Guards                              *
        #**************************************************************
            
        #**************************************************************
        #*                      Sub Machine                           *
        #**************************************************************
        taskAStt = [
                    ('State1','GoTo2','State2','State1toState2','none'),
                    ('State2','GoTo1','State1','State2toState1','none'),
                    ('State2','GoTo3','State3','State2toState3','none'),
                    ('State3','GoTo2','State2','State3toState2','none'),
                    ('AllOk', 'ErrorFoundA','ErrorA','ErrorFoundAAction','none'),
                    ('ErrorA','EndErrorA','AllOk','EndErrorAAction','none')
                   ]
            
        EXFEL_FSM_STATE_MACHINE('TaskA', taskAStt, ('State1','AllOk'))
            
        taskBStt = [
                    ('State4','GoTo5','State5','State4toState5','none'),
                    ('State5','GoTo4','State4','State5toState4','none'),
                    ('AllOk', 'ErrorFoundB','ErrorB','ErrorFoundBAction','none'),
                    ('ErrorB','EndErrorB','AllOk','EndErrorBAction','none')
                   ]
            
        EXFEL_FSM_STATE_MACHINE('TaskB', taskBStt, ('State4','AllOk'))
            
        topStt = [
                  ('InitializeA','none','TaskA','InitializeAtoTaskA','none'),
                  ('InitializeB','none','TaskB','InitializeBtoTaskB','none')
                 ]
        
        EXFEL_FSM_STATE_MACHINE('TopFsm', topStt, ('InitializeA','InitializeB'))
        
        self.fsm = EXFEL_FSM_CREATE_MACHINE('TopFsm')

    def noStateTransition(self): pass
    def onException(self,msg1,msg2): pass
    def endErrorEvent(self): pass
    def goto1Event(self): pass
    def goto2Event(self): pass
    def goto3Event(self): pass
    def goto4Event(self): pass
    def goto5Event(self): pass
    def errorStateOnEntry(self): pass
    def errorStateOnExit(self): pass
    def allOkOnEntry(self): pass
    def allOkOnExit(self): pass
    def initializeAOnEntry(self): pass
    def initializeAOnExit(self): pass
    def initializeBOnEntry(self): pass
    def initializeBOnExit(self): pass
    def state1OnEntry(self): pass
    def state1OnExit(self): pass
    def state2OnEntry(self): pass
    def state2OnExit(self): pass
    def state3OnEntry(self): pass
    def state3OnExit(self): pass
    def state4OnEntry(self): pass
    def state4OnExit(self): pass
    def state5OnEntry(self): pass
    def state5OnExit(self): pass
    def initializeAtoTaskA(self): pass
    def initializeBtoTaskB(self): pass
    def state1toState2(self): pass
    def state2toState1(self): pass
    def state2toState3(self): pass
    def state3toState2(self): pass
    def state4toState5(self): pass
    def state5toState4(self): pass
    def errorFoundAAction(self): pass
    def errorFoundBAction(self): pass
    def endErrorAAction(self): pass
    def endErrorBAction(self): pass


class  Parallel_machine_TestCase(unittest.TestCase):
    def setUp(self):
        self.pm = ParallelMachine()
    #

    def tearDown(self):
        self.pm = None

    def event(self, event_name, event_arg):
        self.pm.fsm.process_event(event_instance(event_name,event_arg))
        
    def test_parallel_machine_(self):
        fsm = self.pm.fsm
        fsm.start()
        self.assertEqual(fsm.get_state(), "[TaskA.[State1:AllOk]:TaskB.[State4:AllOk]]", 'Assert failed!')
        self.event('GoTo2',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State4:AllOk]]", 'Assert failed!')
        self.event('GoTo5',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed!')
        self.event('ErrorFoundA',('Timeout','Hardware not responding'))
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('GoTo4',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State4:AllOk]]", 'Assert failed')
        self.event('GoTo5',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('GoTo3',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('EndErrorB',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('EndErrorA',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('GoTo3',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State3:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('GoTo2',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.event('GoTo1',())
        self.assertEqual(fsm.get_state(), "[TaskA.[State1:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        fsm.stop()


if __name__ == '__main__':
    unittest.main()

