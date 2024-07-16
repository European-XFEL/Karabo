# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$Jul 26, 2012 16:17:33 PM$"

import copy

from karabo.common.states import State

from .worker import Worker

# ======================================= Fsm Macros
# NOOP = lambda: None
# GNOOP = lambda: True


def NOOP():
    return None


def GNOOP():
    return True


class Event:
    def __init__(self, a):
        self.x = a

    def payload(self):
        return self.x


def event_instance(x, a):
    return type(str(x), (Event,), {})(a)


# global dicts
_events_ = ['none']
_states_ = {None: None}
_interrupts_ = {}
_actions_ = {'none': (NOOP, ())}
_guards_ = {'none': (GNOOP, ())}
_machines_ = {None: None}
_state_periodic_actions_ = {'none': (-1, -1, NOOP)}


# events
def KARABO_FSM_EVENT0(self, event_name, method_name):
    def inner(self):
        """Drive state machine by processing an event"""
        self.processEvent(event_instance(event_name, tuple()))

    inner.__name__ = str(method_name)
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)


def KARABO_FSM_EVENT1(self, event_name, method_name):
    def inner(self, a1):
        """Drive state machine by processing an event"""
        self.processEvent(event_instance(event_name, (a1,)))

    inner.__name__ = str(method_name)
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)


def KARABO_FSM_EVENT2(self, event_name, method_name):
    def inner(self, a1, a2):
        """Drive state machine by processing an event"""
        self.processEvent(event_instance(event_name, (a1, a2,)))

    inner.__name__ = str(method_name)
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)


def KARABO_FSM_EVENT3(self, event_name, method_name):
    def inner(self, a1, a2, a3):
        """Drive state machine by processing an event"""
        self.processEvent(event_instance(event_name, (a1, a2, a3,)))

    inner.__name__ = str(method_name)
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)


def KARABO_FSM_EVENT4(self, event_name, method_name):
    def inner(self, a1, a2, a3, a4):
        """Drive state machine by processing an event"""
        self.processEvent(event_instance(event_name, (a1, a2, a3, a4)))

    inner.__name__ = str(method_name)
    setattr(self.__class__, inner.__name__, inner)
    _events_.append(event_name)


# states
def KARABO_FSM_STATE(state, target_action='', on_entry=NOOP, on_exit=NOOP):
    assert isinstance(state, State)
    _states_[state] = (on_entry, on_exit, target_action)


def KARABO_FSM_STATE_E(state, on_entry):
    assert isinstance(state, State)
    _states_[state] = (on_entry, NOOP, '')


def KARABO_FSM_STATE_EE(state, on_entry, on_exit):
    assert isinstance(state, State)
    _states_[state] = (on_entry, on_exit, '')


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
    global _events_, _states_, _interrupts_, _actions_, _guards_, \
        _machines_, _state_periodic_actions_
    (_stt, _initial, _on_entry, _on_exit, _in_state) = _machines_[name]
    cls = type(str(name), (StateMachine,), {})
    ret = cls(None, _stt, _initial, on_entry=_on_entry, on_exit=_on_exit,
              in_state=_in_state)

    # reset everything
    _events_ = ['none']
    _states_ = {None: None}
    _interrupts_ = {}
    _actions_ = {'none': (NOOP, ())}
    _guards_ = {'none': (GNOOP, ())}
    _machines_ = {None: None}
    _state_periodic_actions_ = {'none': (-1, -1, NOOP)}

    return ret


# ======================================== Base classes...
class _State(dict):
    ismachine = False
    interrupt = None

    def __init__(self, fsm, base, on_entry=NOOP, on_exit=NOOP, in_state=''):
        super().__init__()
        self.fsm = fsm
        self.base = base
        self.on_entry = on_entry
        self.on_exit = on_exit
        self.target_action = in_state
        self.worker = None
        self.timeout, self.repetition, self.hook = (-1, -1, None,)

    def _get_top_fsm(self):
        topfsm = self.fsm
        if topfsm is None:
            return self
        return topfsm._get_top_fsm()

    def entry_action(self):
        if self.target_action != '':
            self.timeout, self.repetition, self.hook = copy.copy(
                _state_periodic_actions_[self.target_action])
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
            ename = o.__class__.__name__  # event (class) name
            ea = o.payload()  # tuple of event arguments
        else:
            ename = 'none'  # anonymous transition
            ea = tuple()

        _target, _action, _guard = dict.__getitem__(self,
                                                    ename)  # unpack STT info

        gf, ga = _guard  # function and tuple of arguments
        # compare number and type of parameters between event args (ea) and
        # guard args (ga)
        if len(ea) < len(ga):
            raise SyntaxError('Event payload has less parameters ("%r") than'
                              ' required by guard ("%r")' % (len(ea), len(ga)))
        if not all(isinstance(e, g) for e, g in zip(ea, ga)):
            raise TypeError('Type mismatch between event payload and '
                            'guard required parameters')
        if not gf(*ea[:len(ga)]):  # check guard
            return self

        if _target is not None:
            self.exit_action()  # exit current state

        # unpack action
        af, aa = _action  # action function and tuple of action arguments
        if len(ea) < len(aa):
            raise SyntaxError("Event payload has less parameters (%r) than "
                              "required by transition action (%r)" %
                              (len(ea), len(aa)))
        if not all(isinstance(e, a) for e, a in zip(ea, aa)):
            raise TypeError("Type mismatch between event payload "
                            "and transition action required parameters")
        af(*ea[:len(aa)])  # call transition action

        if _target is None:
            _target_state = self
        else:
            _target_state = self.fsm.stt[_target]
            if _target_state.ismachine:
                _target_state.start()  # enter target machine
            else:
                _target_state.entry_action()  # enter target state

        # Check for anonymous transition
        if dict.__contains__(_target_state, 'none'):  # event is 'none'
            return _target_state['none']  # anonymous transition
        elif dict.__contains__(_target_state, None):  # event is None
            return _target_state[None]

        return _target_state

    def __setitem__(self, evt, value):
        dict.__setitem__(self, evt, value)


class StateMachine(_State):
    ismachine = True

    def __init__(self, fsm, stt, initial, on_entry=NOOP, on_exit=NOOP,
                 in_state=''):
        super().__init__(fsm, None, on_entry=on_entry, on_exit=on_exit,
                         in_state=in_state)
        self.currentStateObject = None
        self.stt = dict()
        self.initial_state = list()
        if not isinstance(initial, State):
            raise TypeError("Initial state should be instance of State class.")
        self._setup(initial)
        self.initial_state.append(self.stt[initial])

        if not self.initial_state:
            raise KeyError('Initial state not set.')

        self.current_state = list()

        for (_source, _event, _target, _action, _guard) in stt:

            if _source is None:
                raise AttributeError("None cannot be a source state")

            if not isinstance(_source, State):
                raise TypeError("Source state should instance of State class")

            if _source in self.stt:
                pass
            else:
                self._setup(_source)

            if not isinstance(_target, (State, type(None))):
                raise TypeError(
                    "Target state should instance of State class or None.")

            if _target is None:
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

            self.stt[_source][_event] = (_target,
                                         copy.copy(_actions_[_action]),
                                         copy.copy(_guards_[_guard]))

        # use global no_transition
        if "NoTransition" in _actions_:
            _f, _a = copy.copy(_actions_['NoTransition'])
            self.no_transition = _f
        else:
            raise IndexError('The "no_transition" action was not defined')

    def _setup(self, sname):
        # Note that the order here is important.
        # Machines should be checked for first.
        if sname in _machines_:
            (_stt, _initial, _entry, _exit, _ta) = copy.copy(_machines_[sname])
            self.stt[sname] = type(str(sname), (StateMachine,), {})(self, _stt,
                                                                    _initial,
                                                                    _entry,
                                                                    _exit, _ta)
        elif sname in _states_:
            (_entry, _exit, _ta) = copy.copy(_states_[sname])
            State = type(sname.name, (_State,), {})
            self.stt[sname] = State(self, sname, on_entry=_entry,
                                    on_exit=_exit, in_state=_ta)
            if sname in _interrupts_:
                self.stt[sname].interrupt = copy.copy(_interrupts_[sname])

        else:
            raise NameError('Undefined name of initial state in State machine '
                            'declaration: {}'.format(sname))

    def start(self):
        # entry current state machine
        if hasattr(self, 'entry_action'):
            self.entry_action()
        self.current_state = copy.copy(
            self.initial_state)  # shallow copy of states
        for i in range(len(self.current_state)):
            if self.current_state[i].ismachine:
                self.current_state[i].start()
            else:
                self.current_state[i].entry_action()
                if None in self.current_state[i]:
                    self.current_state[i] = self.current_state[i][
                        None]  # anonymous transition

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
        # Check if the object (instance of Event subclass) was created by
        # "event_instance" call
        if not isinstance(o, Event):
            raise TypeError(
                'The parameter is not an instance of the "Event" subclass')
        for s in self.current_state:
            if s.interrupt is not None and s.interrupt != o.__class__.__name__:
                return False
        for i, cs in enumerate(self.current_state):
            if o in cs:
                self.current_state[i] = cs[o]  # transition
                return True
        for current in self.current_state:
            if current.ismachine and current.process_event(o):
                return True
        if self.fsm is None:
            self.no_transition(self.get_state(), o.__class__.__name__)
        return False

    def get_state(self):
        if self.current_state[0].ismachine:
            return self.current_state[0].get_state()
        else:
            return self.current_state[0].base

    def get_state_object(self):
        return self.currentStateObject
