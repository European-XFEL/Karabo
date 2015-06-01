# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from karabo.worker import Worker, QueueWorker
from karabo.fsm import KARABO_FSM_NO_TRANSITION_ACTION, KARABO_FSM_EVENT2, KARABO_FSM_EVENT0, KARABO_FSM_INTERRUPT_STATE_EE
from karabo.fsm import KARABO_FSM_STATE_EE, KARABO_FSM_ACTION0, KARABO_FSM_STATE_MACHINE, KARABO_FSM_CREATE_MACHINE
from karabo.fsm import event_instance


class ParallelMachine(object):
    '''
    Example of state machine having hierarchical regions
    '''

    def __init__(self):
        '''
        Constructor
        '''
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)

        #**************************************************************
        #*                        Events                              *
        #**************************************************************

        KARABO_FSM_EVENT2(self, 'ErrorFoundA', 'onExceptionA')
        KARABO_FSM_EVENT0(self, 'EndErrorA', 'endErrorEventA')
        KARABO_FSM_EVENT2(self, 'ErrorFoundB', 'onExceptionB')
        KARABO_FSM_EVENT0(self, 'EndErrorB', 'endErrorEventB')
        KARABO_FSM_EVENT0(self, 'GoTo1', 'goto1Event')
        KARABO_FSM_EVENT0(self, 'GoTo2', 'goto2Event')
        KARABO_FSM_EVENT0(self, 'GoTo3', 'goto3Event')
        KARABO_FSM_EVENT0(self, 'GoTo4', 'goto4Event')
        KARABO_FSM_EVENT0(self, 'GoTo5', 'goto5Event')

        #**************************************************************
        #*                        States                              *
        #**************************************************************

        KARABO_FSM_INTERRUPT_STATE_EE('ErrorA', 'EndErrorA', self.errorStateOnEntry, self.errorStateOnExit)
        KARABO_FSM_INTERRUPT_STATE_EE('ErrorB', 'EndErrorB', self.errorStateOnEntry, self.errorStateOnExit)
            
        KARABO_FSM_STATE_EE('AllOk', self.allOkOnEntry, self.allOkOnExit)

        KARABO_FSM_STATE_EE('InitializeA', self.initializeAOnEntry, self.initializeAOnExit)
        KARABO_FSM_STATE_EE('InitializeB', self.initializeBOnEntry, self.initializeBOnExit)
            
        KARABO_FSM_STATE_EE('State1', self.state1OnEntry, self.state1OnExit)
        KARABO_FSM_STATE_EE('State2', self.state2OnEntry, self.state2OnExit)
        KARABO_FSM_STATE_EE('State3', self.state3OnEntry, self.state3OnExit)
        KARABO_FSM_STATE_EE('State4', self.state4OnEntry, self.state4OnExit)
        KARABO_FSM_STATE_EE('State5', self.state5OnEntry, self.state5OnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************

        KARABO_FSM_ACTION0('InitializeAtoTaskA', self.initializeAtoTaskA)
        KARABO_FSM_ACTION0('InitializeBtoTaskB', self.initializeBtoTaskB)
            
        KARABO_FSM_ACTION0('State1toState2', self.state1toState2)
        KARABO_FSM_ACTION0('State2toState1', self.state2toState1)
        KARABO_FSM_ACTION0('State2toState3', self.state2toState3)
        KARABO_FSM_ACTION0('State3toState2', self.state3toState2)
        KARABO_FSM_ACTION0('State4toState5', self.state4toState5)
        KARABO_FSM_ACTION0('State5toState4', self.state5toState4)

        KARABO_FSM_ACTION0('ErrorFoundAAction', self.errorFoundAAction)
        KARABO_FSM_ACTION0('ErrorFoundBAction', self.errorFoundBAction)
        KARABO_FSM_ACTION0('EndErrorAAction', self.endErrorAAction)
        KARABO_FSM_ACTION0('EndErrorBAction', self.endErrorBAction)
            
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
            
        KARABO_FSM_STATE_MACHINE('TaskA', taskAStt, ('State1','AllOk'))
            
        taskBStt = [
                    ('State4','GoTo5','State5','State4toState5','none'),
                    ('State5','GoTo4','State4','State5toState4','none'),
                    ('AllOk', 'ErrorFoundB','ErrorB','ErrorFoundBAction','none'),
                    ('ErrorB','EndErrorB','AllOk','EndErrorBAction','none')
                   ]
            
        KARABO_FSM_STATE_MACHINE('TaskB', taskBStt, ('State4','AllOk'))
            
        topStt = [
                  ('InitializeA','none','TaskA','InitializeAtoTaskA','none'),
                  ('InitializeB','none','TaskB','InitializeBtoTaskB','none')
                 ]
        
        KARABO_FSM_STATE_MACHINE('TopFsm', topStt, ('InitializeA','InitializeB'))
        
        self.fsm = KARABO_FSM_CREATE_MACHINE('TopFsm')

    def noStateTransition(self): pass

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
    def processEvent(self, event):
        self.fsm.process_event(event)



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
        self.pm.goto2Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State4:AllOk]]", 'Assert failed!')
        self.pm.goto5Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed!')
        self.pm.onExceptionA('Timeout','Hardware not responding')
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.goto4Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State4:AllOk]]", 'Assert failed')
        self.pm.goto5Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.goto3Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.endErrorEventB()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:ErrorA]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.endErrorEventA()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.goto3Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State3:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.goto2Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State2:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        self.pm.goto1Event()
        self.assertEqual(fsm.get_state(), "[TaskA.[State1:AllOk]:TaskB.[State5:AllOk]]", 'Assert failed')
        fsm.stop()


if __name__ == '__main__':
    unittest.main()

