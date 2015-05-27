__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 26, 2012 16:17:33 PM$"

import copy
import threading
from collections import deque

#======================================= Fsm Macros
NOOP = lambda: None
GNOOP = lambda: True

class Event(object):
    def __init__(self, a):
        self.x = a

    def payload(self):
        return self.x


def event_instance(x, a):
    return type(str(x), (Event,), { })(a)


# global dicts
_events_ = ['none']
_states_ = {'none':None}
_interrupts_ = {}
_actions_ = {'none':(NOOP, ())}
_guards_ = {'none':(GNOOP, ())}
_machines_ = {'none':None}
_state_periodic_actions_ = {'none':(-1, -1, NOOP)}

# events
def KARABO_FSM_EVENT(self, event_name, method_name):
    def inner(self, *args):
        """Drive state machine by processing an event"""
        self.processEvent(event_instance(event_name, args))
    inner.__name__ = str(method_name)
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)

KARABO_FSM_EVENT0 = KARABO_FSM_EVENT1 = KARABO_FSM_EVENT2 = \
    KARABO_FSM_EVENT3 = KARABO_FSM_EVENT4 = KARABO_FSM_EVENT


# states
def KARABO_FSM_STATE(name, target_action='', on_entry=NOOP, on_exit=NOOP):
    _states_[name] = (on_entry, on_exit, target_action)


def KARABO_FSM_STATE_E(name, on_entry):
    _states_[name] = (on_entry, NOOP, '')


def KARABO_FSM_STATE_EE(name, on_entry, on_exit):
    _states_[name] = (on_entry, on_exit, '')


KARABO_FSM_STATE_AEE = KARABO_FSM_STATE_AE = KARABO_FSM_STATE_A = \
    KARABO_FSM_STATE


def KARABO_FSM_INTERRUPT_STATE(name, event, target_action='',
                               on_entry=NOOP, on_exit=NOOP):
    _states_[name] = (on_entry, on_exit, target_action)
    _interrupts_[name] = event


def KARABO_FSM_INTERRUPT_STATE_E(name, event, on_entry):
    _states_[name] = (on_entry, NOOP, '')
    _interrupts_[name] = event


def KARABO_FSM_INTERRUPT_STATE_EE(name, event, on_entry, on_exit):
    _states_[name] = (on_entry, on_exit, '')
    _interrupts_[name] = event


KARABO_FSM_INTERRUPT_STATE_AEE = KARABO_FSM_INTERRUPT_STATE_AE = \
    KARABO_FSM_INTERRUPT_STATE_A = KARABO_FSM_INTERRUPT_STATE


# actions
# associate Action class with action function

def KARABO_FSM_ACTION(name, f, *args):
    _actions_[name] = (f, args)


KARABO_FSM_ACTION0 = KARABO_FSM_ACTION1 = KARABO_FSM_ACTION2 = \
    KARABO_FSM_ACTION3 = KARABO_FSM_ACTION4 = KARABO_FSM_ACTION


def KARABO_FSM_NO_TRANSITION_ACTION(f):
    _actions_["NoTransition"] = (f, ())


# guards

def KARABO_FSM_GUARD(name, f, *args):
    _guards_[name] = (f, args)


KARABO_FSM_GUARD0 = KARABO_FSM_GUARD1 = KARABO_FSM_GUARD2 = \
    KARABO_FSM_GUARD3 = KARABO_FSM_GUARD4 = KARABO_FSM_GUARD


# in state periodic actions

def KARABO_FSM_PERIODIC_ACTION(name, timeout, repetition, f):
    _state_periodic_actions_[name] = (timeout, repetition, f)


# machines


def KARABO_FSM_STATE_MACHINE_E(name, stt, initial, on_entry):
    _machines_[name] = (stt, initial, on_entry, NOOP, '')


def KARABO_FSM_STATE_MACHINE_EE(name, stt, initial, on_entry, on_exit):
    _machines_[name] = (stt, initial, on_entry, on_exit, '')


def KARABO_FSM_STATE_MACHINE(name, stt, initial, target_action='',
                             on_entry=NOOP, on_exit=NOOP):
    _machines_[name] = (stt, initial, on_entry, on_exit, target_action)


KARABO_FSM_STATE_MACHINE_AEE = KARABO_FSM_STATE_MACHINE_AE = \
    KARABO_FSM_STATE_MACHINE_A = KARABO_FSM_STATE_MACHINE


def KARABO_FSM_CREATE_MACHINE(name):
    (_stt, _initial, _on_entry, _on_exit, _in_state) = _machines_[name]
    cls = type(str(name), (StateMachine,), { })
    return cls(None, _stt, _initial, on_entry=_on_entry, on_exit=_on_exit,
               in_state = _in_state)

#======================================== Worker
class Worker(threading.Thread):
    
    def __init__(self, callback = None, timeout = -1, repetition = -1):
        threading.Thread.__init__(self)
        self.callback = callback
        self.timeout  = timeout
        self.repetition = repetition
        self.running = False
        self.aborted = False
        self.suspended = False
        self.counter = -1
        self.cv = threading.Condition()  # cv = condition variable
        self.dq = deque()
    
    def set(self, callback, timeout = -1, repetition = -1):
        self.callback = callback
        self.timeout  = timeout
        self.repetition = repetition

    def setTimeout(self, timeout = -1):
        self.timeout = timeout

    def setRepetition(self, repetition = -1):
        self.repetition = repetition

    def is_running(self):
        return self.running
    
    def push(self,o):
        if self.running:
            with self.cv:
                self.dq.append(o)
                self.cv.notify()
            
    def isRepetitionCounterExpired(self):
        return self.counter == 0
    
    def run(self):
        self.running = True
        self.aborted = False
        self.suspended = False
        self.counter = self.repetition
        while not self.aborted:
            t = None
            if self.counter == 0:
                break
            if not self.running:
                break
            try:
                if self.suspended:
                    with self.cv:
                        while self.suspended:
                            self.cv.wait()
                        if self.aborted or not self.running:
                            break
                    continue
                if self.timeout < 0:
                    with self.cv:
                        while len(self.dq) == 0:
                            self.cv.wait()
                        if not self.suspended:
                            t = self.dq.popleft()
                elif self.timeout > 0:
                    with self.cv:
                        if len(self.dq) == 0:
                            self.cv.wait(float(self.timeout) / 1000)   # self.timeout in milliseconds
                        if len(self.dq) != 0 and not self.suspended:
                            t = self.dq.popleft()
                else:
                    with self.cv:
                        if len(self.dq) != 0 and not self.suspended:
                            t = self.dq.popleft()
                if self.suspended:
                    continue
                if t is not None:
                    if stopCondition(t):
                        break
            except RuntimeError as e:
                t = None
            if self.counter > 0:
                self.counter -= 1
            if self.running:
                self.callback(self.counter == 0)      # self.callback(self.counter == 0)
        if self.running:
            self.running = False
            
    def stopCondition(self, obj):
        return False
    
    def start(self):
        if not self.running:
            self.suspended = False
            super(Worker, self).start()
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()
        return self
    
    def stop(self):
        if self.running:
            with self.cv:
                self.running = False
                self.suspended = False
                self.cv.notify()
        return self
    
    def abort(self):
        self.aborted = True
        self.running = False
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()        
        if len(self.dq) != 0:
            with self.cv:
                self.dq.clear()
        return self
    
    def pause(self):
        if not self.suspended:
            with self.cv:
                self.suspended = True
                self.cv.notify()
            
#======================================== Base classes...    
class State(dict):
    ismachine = False
    interrupt = None
    
    def __init__(self, fsm, on_entry=NOOP, on_exit=NOOP, in_state=''):
        dict.__init__(self)
        self.fsm = fsm
        self.on_entry = on_entry
        self.on_exit = on_exit
        self.target_action = in_state
        self.worker = None
        self.timeout, self.repetition, self.hook = (-1,-1,None,)
    
    def _get_top_fsm(self):
        topfsm = self.fsm
        if topfsm is None:
            return self
        return topfsm._get_top_fsm()
    
    def entry_action(self):
        if self.target_action != '':
            self.timeout, self.repetition, self.hook = _state_periodic_actions_[self.target_action]
        topfsm = self._get_top_fsm()
        topfsm.currentStateObject = self
        if self.on_entry != NOOP:
            self.on_entry()
        if self.target_action != '':
            self.worker = Worker(self.hook, self.timeout, self.repetition)
            self.worker.start()
        
    def exit_action(self):
        if self.worker is not None:
            if self.worker.is_running():
                self.worker.stop()
            self.worker.join()
            self.worker = None
        self.on_exit()
        
    def getWorker(self):
        return self.worker
    
    def __contains__(self, o):
        if o:
            return dict.__contains__(self, o.__class__.__name__)
        return dict.__contains__(self, 'none')
        
    
    def __getitem__(self, o):
        # use event name for searching in State dictionary
        if o:
            ename = o.__class__.__name__   # event (class) name
            ea = o.payload()               # tuple of event arguments
        else:
            ename = 'none'                 # anonymous transition
            ea = tuple()
        
        _target, _action, _guard = dict.__getitem__(self, ename)     # unpack STT info
        
        gf, ga = _guard                     # function and tuple of arguments
        # compare number and type of parameters between event args (ea) and guard args (ga)
        if len(ea) < len(ga):
            raise SyntaxError('Event payload has less parameters ("%r") '
                              'than required by guard ("%r")' %
                              (len(ea), len(ga)))
        if not all(isinstance(e, g) for e, g in zip(ea, ga)):
            raise TypeError('Type mismatch between event payload and '
                            'guard required parameters')
        if not gf(*ea[:len(ga)]):           # check guard
            return self
        
        if _target != "none":
            self.exit_action()              # exit current state
        
        # unpack action
        af, aa = _action                    # action function and tuple of action arguments
        if len(ea) < len(aa):
            raise SyntaxError("Event payload has less parameters (%r) than "
                              "required by transition action (%r)" %
                              (len(ea), len(aa)))
        if not all(isinstance(e, a) for e, a in zip(ea, aa)):
            raise TypeError("Type mismatch between event payload "
                            "and transition action required parameters")
        af(*ea[:len(aa)])                   # call transition action
        
        if _target == "none":
            _target_state = self
        else:
            _target_state = self.fsm.stt[_target]
            if _target_state.ismachine:
                _target_state.start()           # enter target machine
            else:
                _target_state.entry_action()    # enter target state
            
        if dict.__contains__(_target_state, 'none'):
            return _target_state[None]      # anonymous transition
        
        return _target_state
        
    def __setitem__(self, evt, value):
        dict.__setitem__(self, evt, value)
        
class StateMachine(State):
    ismachine = True
    
    def __init__(self, fsm, stt, initial, on_entry=NOOP, on_exit=NOOP, in_state=''):
        super(StateMachine, self).__init__(fsm, on_entry=on_entry, on_exit=on_exit, in_state=in_state)
        self.currentStateObject=None
        self.stt = dict()
        self.initial_state = list()
        if isinstance(initial, tuple):
            for sname in initial:
                self._setup(sname)
                self.initial_state.append(self.stt[sname])
        elif isinstance(initial, str):
            self._setup(initial)
            self.initial_state.append(self.stt[initial])
        else:
            raise TypeError(
                "Unsupported type {} for representing initial state. "
                "Required str or tuple of str".format(type(initial)))

        if not self.initial_state:
            raise KeyError('Initial state not set.')
        
        self.current_state = list()
        
        for (_source, _event, _target, _action, _guard) in stt:
            
            if _source == 'none':
                raise AttributeError("'none' cannot be a source state")
            elif _source in self.stt:
                pass
            else:
                self._setup(_source)
                
            if _target == 'none':
                pass
            elif _target in self.stt:
                pass
            else:
                self._setup(_target)
                
            if _event not in _events_:
                raise NameError('Undefined name in StateTransitionTable: {}'.
                                format(_event))
            if _action not in _actions_:
                raise NameError('Undefined name in StateTransitionTable: {}'.
                                format(_action))
            if _guard not in _guards_:
                raise NameError('Undefined name in StateTransitionTable: {}'.
                                format(_guard))

            self.stt[_source][_event] = (_target, _actions_[_action], _guards_[_guard])    
        
        # use global no_transition
        if "NoTransition" in _actions_:
            _f, _a = _actions_['NoTransition']
            self.no_transition = _f
        else:
            raise IndexError('The "no_transition" action was not defined')

    def _setup(self, sname):
        if sname in _states_:
            (_entry, _exit, _ta) = _states_[sname]
            self.stt[sname] = type(str(sname), (State,), {})(self, on_entry=_entry, on_exit=_exit, in_state=_ta)
            if sname in _interrupts_:
                self.stt[sname].interrupt = _interrupts_[sname]
        elif sname in _machines_:
            (_stt, _initial, _entry, _exit, _ta) = _machines_[sname]
            self.stt[sname] = type(str(sname), (StateMachine,), {})(self, _stt, _initial, _entry, _exit, _ta)
        else:
            raise NameError('Undefined name of initial state in State machine '
                            'declaration: {}'.format(sname))

    def start(self):
        # entry current state machine
        if hasattr(self, 'entry_action'):
            self.entry_action()
        self.current_state = copy.copy(self.initial_state)        # shallow copy of states
        for i in range(len(self.current_state)):
            if self.current_state[i].ismachine:
                self.current_state[i].start()
            else:
                self.current_state[i].entry_action()
                if None in self.current_state[i]:
                    self.current_state[i] = self.current_state[i][None]  # anonymous transition
        
    def stop(self):
        for state_object in self.current_state:
            if state_object.ismachine:
                state_object.stop()
            else:
                state_object.exit_action()
        # exit current state machine
        if hasattr(self, 'exit_action'):
            self.exit_action()
        self.current_state = None

    def process_event(self, o):
        # check if the object (instance of Event subclass) was created by "event_instance" call
        if not isinstance(o, Event):
            raise TypeError(
                'The parameter is not an instance of the "Event" subclass')
        for s in self.current_state:
            if not s.interrupt is None and s.interrupt != o.__class__.__name__:
                return False
        for i, cs in enumerate(self.current_state):
            if o in cs:
                self.current_state[i] = cs[o]  # transition
                return True
        for current in self.current_state:
            if current.ismachine and current.process_event(o):
                return True
        if self.fsm is None:
            self.no_transition()
        return False

    def get_state(self):
        result = [x.__class__.__name__ + "." + x.get_state()
                  if x.ismachine else x.__class__.__name__
                  for x in self.current_state]
        if len(result) == 1:
            return str(result[0])
        return '[' + ':'.join(result) + ']'

    def get_state_object(self):
        return self.currentStateObject
