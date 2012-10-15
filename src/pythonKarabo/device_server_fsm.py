'''
Created on Jul 26, 2012

@author: esenov
'''

from abc import ABCMeta, abstractmethod
from libkarabo import Hash
from fsm import EXFEL_FSM_EVENT0, EXFEL_FSM_EVENT1, EXFEL_FSM_EVENT2, EXFEL_FSM_STATE, EXFEL_FSM_STATE_E
from fsm import EXFEL_FSM_ACTION0, EXFEL_FSM_ACTION1, EXFEL_FSM_ACTION2, EXFEL_FSM_STATE_MACHINE
from fsm import EXFEL_FSM_NO_TRANSITION_ACTION, EXFEL_FSM_CREATE_MACHINE

class DeviceServerFsm(object):
    '''
    Base Class describing State machine for DeviceServer
    '''
    __metaclass__ = ABCMeta

    def __init__(self):
        '''
        Description of state machine
        '''
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        
        EXFEL_FSM_EVENT2('ErrorFoundEvent', self.errorFound, str, str)
        EXFEL_FSM_EVENT0('EndErrorEvent', self.endError)
        EXFEL_FSM_EVENT0('NewPluginAvailableEvent', self.newPluginAvailable)
        EXFEL_FSM_EVENT0('InbuildDevicesAvailableEvent', self.inbuildDevicesAvailable)
        EXFEL_FSM_EVENT1('StartDeviceEvent', self.slotStartDevice, Hash)
        EXFEL_FSM_EVENT1('RegistrationOkEvent', self.slotRegistrationOk, str)
        EXFEL_FSM_EVENT1('RegistrationFailedEvent', self.slotRegistrationFailed, str)

        #**************************************************************
        #*                        States                              *
        #**************************************************************

        EXFEL_FSM_STATE_E('RegistrationState', self.registrationStateOnEntry)
        EXFEL_FSM_STATE('ErrorState')
        EXFEL_FSM_STATE_E('IdleState', self.idleStateOnEntry)
        EXFEL_FSM_STATE('ServingState')

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************

        EXFEL_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        EXFEL_FSM_ACTION0('EndErrorAction', self.endErrorAction)
        EXFEL_FSM_ACTION0('NotifyNewDeviceAction', self.notifyNewDeviceAction)
        EXFEL_FSM_ACTION1('StartDeviceAction', self.startDeviceAction, Hash)
        EXFEL_FSM_ACTION1('RegistrationFailedAction', self.registrationFailed, str)
        EXFEL_FSM_ACTION1('RegistrationOkAction', self.registrationOk, str)
        
        EXFEL_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
            
        #**************************************************************
        #*                      AllOk Machine                         *
        #**************************************************************

        AllOkSTT = [
                    ('RegistrationState', 'RegistrationOkEvent',     'IdleState',  'RegistrationOkAction',     'none'),
                    ('RegistrationState', 'RegistrationFailedEvent', 'ErrorState', 'RegistrationFailedAction', 'none'),
                    ('IdleState',         'NewPluginAvailableEvent', 'none',       'NotifyNewDeviceAction',    'none'),
                    ('IdleState',    'InbuildDevicesAvailableEvent', 'none',       'NotifyNewDeviceAction',    'none'),
                    ('IdleState',         'StartDeviceEvent',      'ServingState', 'StartDeviceAction',        'none'),
                    ('ServingState',      'StartDeviceEvent',        'none',       'StartDeviceAction',        'none')
                   ]
        
        EXFEL_FSM_STATE_MACHINE('AllOkState', AllOkSTT, 'RegistrationState')
        
        DeviceServerMachineSTT=[
                                ('AllOkState', 'ErrorFoundEvent', 'ErrorState', 'ErrorFoundAction', 'none'),
                                ('ErrorState', 'EndErrorEvent',   'AllOkState', 'EndErrorAction',   'none')
                               ]
        
        EXFEL_FSM_STATE_MACHINE('DeviceServerMachine', DeviceServerMachineSTT, 'AllOkState')
        
        self.fsm = EXFEL_FSM_CREATE_MACHINE('DeviceServerMachine')
    
    @abstractmethod   
    def errorFound(self, m1, m2): pass
    @abstractmethod   
    def endError(self): pass
    @abstractmethod   
    def newPluginAvailable(self): pass
    @abstractmethod   
    def inbuildDevicesAvailable(self): pass
    @abstractmethod   
    def slotStartDevice(self, h): pass
    @abstractmethod   
    def slotRegistrationOk(self, a1): pass
    @abstractmethod   
    def slotRegistrationFailed(self, a1): pass
    @abstractmethod   
    def registrationStateOnEntry(self): pass
    @abstractmethod   
    def idleStateOnEntry(self): pass
    @abstractmethod   
    def errorFoundAction(self, m1, m2): pass
    @abstractmethod   
    def endErrorAction(self): pass
    @abstractmethod   
    def notifyNewDeviceAction(self): pass
    @abstractmethod   
    def startDeviceAction(self, h): pass
    @abstractmethod   
    def registrationFailed(self, a1): pass
    @abstractmethod   
    def registrationOk(self, a1): pass
    @abstractmethod   
    def noStateTransition(self): pass
        