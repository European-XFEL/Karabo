__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 26, 2012 16:17:33 PM$"

import copy
import Queue
import threading

#======================================= Fsm Macros
NOOP = lambda: None
GNOOP = lambda: True
class Event(object): pass
def x_ini(self, a): self.x = a
def x_get(self): return self.x
def event_instance(x, a): return type(x, (Event,), {'__init__':x_ini, 'payload':x_get})(a)

# global dicts
_events_ = ['none']
_states_ = {'none':None}
_interrupts_ = {}
_actions_ = {'none':(NOOP, ())}
_guards_ = {'none':(GNOOP, ())}
_machines_ = {'none':None}
_state_periodic_actions_ = {'none':(-1, -1, NOOP)}
# events
def KARABO_FSM_EVENT0(self, event_name, method_name):
    def inner(self):
        self.processEvent(event_instance(event_name, ()))
    inner.__doc__ = "Drive state machine by processing an event without payload"
    inner.__name__ = method_name
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)

def KARABO_FSM_EVENT1(self, event_name, method_name):
    def inner(self, a1):
        self.processEvent(event_instance(event_name, (a1,)))
    inner.__doc__ = "Drive state machine by processing an event with one parameter"
    inner.__name__ = method_name
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)

def KARABO_FSM_EVENT2(self, event_name, method_name):
    def inner(self, a1, a2):
        self.processEvent(event_instance(event_name, (a1,a2,)))
    inner.__doc__ = "Drive state machine by processing an event with two parameters"
    inner.__name__ = method_name
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)

def KARABO_FSM_EVENT3(self, event_name, method_name):
    def inner(self, a1, a2, a3):
        self.processEvent(event_instance(event_name, (a1,a2,a3,)))
    inner.__doc__ = "Drive state machine by processing an event with three parameters"
    inner.__name__ = method_name
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)

def KARABO_FSM_EVENT4(self, event_name, method_name):
    def inner(self, a1, a2, a3, a4):
        self.processEvent(event_instance(event_name, (a1,a2,a3,a4,)))
    inner.__doc__ = "Drive state machine by processing an event with four parameters"
    inner.__name__ = method_name
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)

    
# states
def KARABO_FSM_STATE(name):                                       _states_[name] = (NOOP, NOOP, '')
def KARABO_FSM_STATE_A(name, target_action):                      _states_[name] = (NOOP, NOOP, target_action)
def KARABO_FSM_STATE_E(name, on_entry):                           _states_[name] = (on_entry, NOOP, '')
def KARABO_FSM_STATE_AE(name, target_action, on_entry):           _states_[name] = (on_entry, NOOP, target_action)
def KARABO_FSM_STATE_EE(name, on_entry, on_exit):                 _states_[name] = (on_entry, on_exit, '')
def KARABO_FSM_STATE_AEE(name, target_action, on_entry, on_exit): _states_[name] = (on_entry, on_exit, target_action)

def KARABO_FSM_INTERRUPT_STATE(name, event):
    _states_[name] = (NOOP, NOOP, '')
    _interrupts_[name] = event
    
def KARABO_FSM_INTERRUPT_STATE_A(name, event, target_action):
    _states_[name] = (NOOP, NOOP, target_action)
    _interrupts_[name] = event
    
def KARABO_FSM_INTERRUPT_STATE_E(name, event, on_entry):
    _states_[name] = (on_entry, NOOP, '')
    _interrupts_[name] = event

def KARABO_FSM_INTERRUPT_STATE_AE(name, event, target_action, on_entry):
    _states_[name] = (on_entry, NOOP, target_action)
    _interrupts_[name] = event

def KARABO_FSM_INTERRUPT_STATE_EE(name, event, on_entry, on_exit):
    _states_[name] = (on_entry, on_exit, '')
    _interrupts_[name] = event
    
def KARABO_FSM_INTERRUPT_STATE_AEE(name, event, target_action, on_entry, on_exit):
    _states_[name] = (on_entry, on_exit, target_action)
    _interrupts_[name] = event
    
# actions
# associate Action class with action function

def KARABO_FSM_ACTION0(name, f):                 _actions_[name] = (f, ())
def KARABO_FSM_ACTION1(name, f, a1):             _actions_[name] = (f, (a1,))
def KARABO_FSM_ACTION2(name, f, a1, a2):         _actions_[name] = (f, (a1, a2))   
def KARABO_FSM_ACTION3(name, f, a1, a2, a3):     _actions_[name] = (f, (a1, a2, a3))
def KARABO_FSM_ACTION4(name, f, a1, a2, a3, a4): _actions_[name] = (f, (a1, a2, a3, a4))
  
def KARABO_FSM_NO_TRANSITION_ACTION(f):          _actions_["NoTransition"] = (f, ())
    
# guards

def KARABO_FSM_GUARD0(name, f):                 _guards_[name] = (f, ())
def KARABO_FSM_GUARD1(name, f, a1):             _guards_[name] = (f, (a1,))
def KARABO_FSM_GUARD2(name, f, a1, a2):         _guards_[name] = (f, (a1, a2))
def KARABO_FSM_GUARD3(name, f, a1, a2, a3):     _guards_[name] = (f, (a1, a2, a3))
def KARABO_FSM_GUARD4(name, f, a1, a2, a3, a4): _guards_[name] = (f, (a1, a2, a3, a4))

# in state periodic actions

def KARABO_FSM_PERIODIC_ACTION(name, timeout, repetition, f): _state_periodic_actions_[name] = (timeout, repetition, f)

# machines

def KARABO_FSM_STATE_MACHINE(name, stt, initial):
    _machines_[name] = (stt, initial, NOOP, NOOP, '')

def KARABO_FSM_STATE_MACHINE_A(name, stt, initial, target_action):
    _machines_[name] = (stt, initial, NOOP, NOOP, target_action)

def KARABO_FSM_STATE_MACHINE_E(name, stt, initial, on_entry):
    _machines_[name] = (stt, initial, on_entry, NOOP, '')

def KARABO_FSM_STATE_MACHINE_AE(name, stt, initial, target_action, on_entry):
    _machines_[name] = (stt, initial, on_entry, NOOP, target_action)

def KARABO_FSM_STATE_MACHINE_EE(name, stt, initial, on_entry, on_exit):
    _machines_[name] = (stt, initial, on_entry, on_exit, '')
       
def KARABO_FSM_STATE_MACHINE_AEE(name, stt, initial, target_action, on_entry, on_exit):
    _machines_[name] = (stt, initial, on_entry, on_exit, target_action)
       
def KARABO_FSM_CREATE_MACHINE(name):
    (_stt, _initial, _on_entry, _on_exit, _in_state) = _machines_[name]
    cls = type(name, (StateMachine,), {})
    return cls(None, _stt, _initial, on_entry=_on_entry, on_exit=_on_exit, in_state = _in_state)

#======================================== Worker
class Worker(threading.Thread):
    
    def __init__(self, callback = None, timeout = -1, repetition = -1):
        threading.Thread.__init__(self)
        self.callback = callback
        self.timeout  = timeout
        self.repetition = repetition
        self.running = False
        self.aborted = False
        self.queue = Queue.Queue()
        self.counter = -1
    
    def set(self, callback, timeout = -1, repetition = -1):
        self.callback = callback
        self.timeout  = timeout
        self.repetition = repetition

    def cond(self, s):
        if type(s) is bool:
            return s
        if s is None:
            return False
        return True

    def is_running(self):
        return self.running
    
    def push(self, o):
        if self.running:
            self.queue.put(o)
            
    def isRepetitionCounterExpired(self):
        return self.counter == 1
    
    def run(self):
        self.running = True
        self.aborted = False
        self.counter = self.repetition
        while not self.aborted:
            if self.counter == 0:
                break
            if not self.running and self.queue.empty():
                break
            try:
                if self.timeout < 0:
                    t = self.queue.get(True)
                elif self.timeout > 0:
                    t = self.queue.get(True, float(self.timeout) / 1000)   # self.timeout in milliseconds
                else:
                    t = self.queue.get(False)
                self.queue.task_done()
            except Queue.Empty, e:
                t = None
            if self.counter > 0:
                self.counter -= 1
            if self.running or not self.cond(t):
                self.callback(self.counter == 0)      # self.callback(self.counter == 0)
        if self.running:
            self.running = False
            
    def start(self):
        super(Worker, self).start()
        return self
    
    def stop(self):
        if self.running:
            self.running = False
        return self
    
    def abort(self):
        self.aborted = True
        self.running = False
        while not self.queue.empty():
            t = self.queue.get(False)
            self.queue.task_done()
        return self
    

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
        try:
            if sum([type(ea[i]) == ga[i] for i in range(len(ga))]) != len(ga):
                raise TypeError,'Type mismatch between event payload and guard required parameters'
        except IndexError:
            raise SyntaxError,'Event payload has less parameters ("%r") than required by guard ("%r")' % (len(ea), len(ga))
        if not gf(*ea[:len(ga)]):           # check guard
            return self
        
        if _target != "none":
            self.exit_action()              # exit current state
        
        # unpack action
        af, aa = _action                    # action function and tuple of action arguments
        try:
            if sum([type(ea[i]) == aa[i] for i in range(len(aa))]) != len(aa):
                raise TypeError,"Type mismatch between event payload and transition action required parameters"
        except IndexError:
            raise SyntaxError,"Event payload has less parameters (%r) than required by transition action (%r)" % (len(ea), len(aa))
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
        if type(initial) is tuple:
            for sname in initial:
                self._setup(sname)
                self.initial_state.append(self.stt[sname])
        elif type(initial) is str:
            self._setup(initial)
            self.initial_state.append(self.stt[initial])
        else:
            raise TypeError,"Unsupported type %r for representing initial state. Required str or tuple of str" % type(initial)

        if len(self.initial_state) == 0:
            raise KeyError, 'Initial state not set.'
        
        self.current_state = list()
        
        for (_source, _event, _target, _action, _guard) in stt:
            
            if _source == 'none':
                raise AttributeError,"'none' cannot be a source state"
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
                raise NameError,'Undefined name in StateTransitionTable: %r' % _event
            if _action not in _actions_:
                raise NameError,'Undefined name in StateTransitionTable: %r' % _action
            if _guard not in _guards_:
                raise NameError,'Undefined name in StateTransitionTable: %r' % _guard
            
            self.stt[_source][_event] = (_target, _actions_[_action], _guards_[_guard])    
        
        # use global no_transition
        if "NoTransition" in _actions_:
            _f, _a = _actions_['NoTransition']
            self.no_transition = _f
        else:
            raise IndexError,'The "no_transition" action was not defined'
    
    def _setup(self, sname):
        if sname in _states_:
            (_entry, _exit, _ta) = _states_[sname]
            self.stt[sname] = type(sname, (State,), {})(self, on_entry=_entry, on_exit=_exit, in_state=_ta)
            if sname in _interrupts_:
                self.stt[sname].interrupt = _interrupts_[sname]
        elif sname in _machines_:
            (_stt, _initial, _entry, _exit, _ta) = _machines_[sname]
            self.stt[sname] = type(sname, (StateMachine,), {})(self, _stt, _initial, _entry, _exit, _ta)
        else:
            raise NameError,'Undefined name of initial state in State machine declaration: %r' % sname
        
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
        is_hit = False
        if not isinstance(o, Event):
            raise TypeError,'The parameter is not an instance of the "Event" subclass'
        for s in self.current_state:
            if not s.interrupt is None and s.interrupt != o.__class__.__name__:
                return False
        for i in range(len(self.current_state)):
            if o in self.current_state[i]:
                self.current_state[i] = self.current_state[i][o]  # transition
                is_hit = True
                return is_hit
        for current in self.current_state:
            if current.ismachine:
                is_hit = current.process_event(o)
                if is_hit:
                    return True
        if not is_hit and self.fsm is None:
            self.no_transition()
        return is_hit
    
    def get_state(self):
        f = lambda x: x.__class__.__name__ + "." + x.get_state() if x.ismachine else x.__class__.__name__ 
        result = [ f(self.current_state[i]) for i in range(len(self.current_state)) ]
        if len(result) == 1:
            return str(result[0])
        ret = '['
        for x in result:
            if ret == '[':
                ret += x
            else:
                ret += ':' + x
        ret += ']'
        return ret
    
    def get_state_object(self):
        return self.currentStateObject
