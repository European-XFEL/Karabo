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
from enum import EnumMeta, StrEnum


class _ParentDict:
    """allow enums to reference each other

    This is a wrapper for the namespace of the class body of an enum.
    We abuse the definition of the members of the enum to instead create
    a tree hierarchy. To this end, we save the right side of an assignment
    into a `parents` dict, and tell the underlying dict that the value of
    the member is its name.
    """

    def __init__(self, raw_dict):
        self.raw_dict = raw_dict
        self.parents = {}

    def __getitem__(self, key):
        if key.startswith("_"):
            return self.raw_dict[key]
        else:
            return key

    def __setitem__(self, key, value):
        if key.startswith("_"):
            self.raw_dict[key] = value
        else:
            self.raw_dict[key] = key
            self.parents[key] = value


class ParentEnumMeta(EnumMeta):
    """An Enum which defines a tree hierarchy

    Based on this class one can define hierarchies of enum members.
    The value of each member is just its name, and the value that
    you normally assign it to is the parent, or None if that's a root.

    As an example::

        >>> class Tree(Enum, metaclass=ParentEnumMeta):
        ...    root = None
        ...    some_member = root  # root is the parent of some_member
        >>> Tree.root.value
        'root'
        >>> Tree.some_member.value
        'some_member'
        >>> Tree.root.parent
        >>> Tree.some_member.parent
        <Tree.root: 'root'>
    """

    @classmethod
    def __prepare__(cls, name, bases):
        return _ParentDict(super().__prepare__(name, bases))

    def __new__(cls, name, bases, ns):
        self = super().__new__(cls, name, bases, ns.raw_dict)
        for k, v in ns.parents.items():
            if v is None:
                self(k).parent = None
            else:
                self(k).parent = self(v)
        return self


class StateBase(StrEnum):
    def isDerivedFrom(self, other):
        """return whether `self` is in the ancestry of `other`"""
        while self is not None:
            if self is other:
                return True
            self = self.parent
        return False

    @classmethod
    def fromString(cls, name):
        """return the state with `name`.

        You might prefer using ``State(name)``.
        """
        return cls(name)


class State(StateBase, metaclass=ParentEnumMeta):
    """The State class represents the available states in Karabo

    Only members of this class should be used for indicating
    state. The states defined here form a hierarchy, where

    * UNKNOWN
    * KNOWN
    * INIT

    are at the basis. All other states derive from the KNOWN
    base state, which at the second hierarchy level fans out
    to

    * DISABLED
    * NORMAL
    * ERROR

    Here, INTERLOCK derives from DISABLED and ERROR is
    reserved for hardware errors. Conversely an unknown software
    error should be indicated by transitioning to the UNKNOWN state,
    whenever it is unclear if the device is still following hardware
    state accordingly.

    Finally, the NORMAL base states has the members

    * STATIC -> ACTIVE, PASSIVE
    * CHANGING -> INCREASING, DECREASING

    which each in turn are the basis for the most derived
    and descriptive leaf states.

    Each state knows its parent state by the attribute `parent`.
    """

    UNKNOWN = None
    KNOWN = None
    INIT = None

    ERROR = KNOWN

    # INTERLOCKED is derived from DISABLED, but much more significant
    INTERLOCKED = DISABLED  # noqa

    NORMAL = KNOWN
    STATIC = NORMAL

    INTERLOCK_OK = STATIC

    CHANGING = NORMAL

    DECREASING = CHANGING
    COOLING = DECREASING
    MOVING_LEFT = DECREASING
    MOVING_DOWN = DECREASING
    MOVING_BACK = DECREASING
    ROTATING_CNTCLK = DECREASING
    RAMPING_DOWN = DECREASING
    EXTRACTING = DECREASING
    STOPPING = DECREASING
    EMPTYING = DECREASING
    DISENGAGING = DECREASING
    SWITCHING_OFF = DECREASING

    HOMING = CHANGING
    ROTATING = CHANGING
    MOVING = CHANGING
    SWITCHING = CHANGING
    OPENING = CHANGING
    CLOSING = CHANGING
    SEARCHING = CHANGING

    INCREASING = CHANGING
    HEATING = INCREASING
    MOVING_RIGHT = INCREASING
    MOVING_UP = INCREASING
    MOVING_FORWARD = INCREASING
    ROTATING_CLK = INCREASING
    RAMPING_UP = INCREASING
    INSERTING = INCREASING
    STARTING = INCREASING
    FILLING = INCREASING
    ENGAGING = INCREASING
    SWITCHING_ON = INCREASING

    PAUSED = DISABLED  # noqa

    RUNNING = NORMAL

    ACQUIRING = RUNNING
    PROCESSING = RUNNING

    PASSIVE = STATIC
    WARM = PASSIVE
    COLD = PASSIVE
    PRESSURIZED = PASSIVE
    CLOSED = PASSIVE
    OFF = PASSIVE
    INSERTED = PASSIVE
    STOPPED = PASSIVE
    UNLOCKED = PASSIVE
    DISENGAGED = PASSIVE
    IGNORING = PASSIVE

    ACTIVE = STATIC
    COOLED = ACTIVE
    HEATED = ACTIVE
    EVACUATED = ACTIVE
    OPENED = ACTIVE
    ON = ACTIVE
    EXTRACTED = ACTIVE
    STARTED = ACTIVE
    LOCKED = ACTIVE
    ENGAGED = ACTIVE
    MONITORING = ACTIVE

    DISABLED = KNOWN
    INTERLOCK_BROKEN = DISABLED


class StateSignifier:
    """Define an order of significance for the states

    :param trumplist: An order of significance can be prescribed by
        creating a `StateSignifier` with this iterable. It contains
        states ordered from low to high significance. It defaults to,
        well, the default order.

        For groups of states having the same parent, only the parent
        needs to be listed, all children will be inserted in the
        default order. States which are not listed at all are
        considered least significant.

        As an example, to get ACTIVE to be more significant than
        PASSIVE, you may write::

            StateSignifier([State.PASSIVE, State.ACTIVE])

        Meaning that ACTIVE is more signifcant than PASSIVE.

    :param staticMoreSignificant: tells whether PASSIVE (the default)
        or ACTIVE should be more significant in the default order.

    :param changingMoreSignificant: tells whether DECREASING (the
        default) or INCREASING should be more significant.
    """

    def __init__(
        self,
        trumplist=(),
        staticMoreSignificant=State.PASSIVE,
        changingMoreSignificant=State.DECREASING,
    ):
        def key(state):
            while state is not None:
                ret = trumpdict.get(state)
                if ret is not None:
                    return ret
                state = state.parent
            return -1

        defaults = {
            (State.PASSIVE, State.INCREASING): self.passive_increase,
            (State.ACTIVE, State.INCREASING): self.active_increase,
            (State.PASSIVE, State.DECREASING): self.passive_decrease,
            (State.ACTIVE, State.DECREASING): self.active_decrease,
        }
        defaultlist = defaults[staticMoreSignificant, changingMoreSignificant]
        trumpdict = {s: i for i, s in enumerate(defaultlist)}
        states = list(State.__members__.values())
        states.sort(key=key)
        trumpdict = {s: i for i, s in enumerate(trumplist)}
        states.sort(key=key, reverse=True)
        self.trumpdict = {s: i for i, s in enumerate(states)}

    def returnMostSignificant(self, iterable):
        """return the most significant state in `iterable`"""
        return min(iterable, key=self.trumpdict.get)

    passive_decrease = []
    active_decrease = [
        State.INTERLOCKED,
        State.CHANGING,
        State.PAUSED,
        State.RUNNING,
        State.NORMAL,
        State.ACTIVE,
        State.PASSIVE,
        State.DISABLED,
    ]
    passive_increase = [
        State.INTERLOCKED,
        State.INCREASING,
        State.CHANGING,
        State.DECREASING,
        State.PAUSED,
        State.RUNNING,
        State.STATIC,
        State.DISABLED,
    ]
    active_increase = [
        State.INTERLOCKED,
        State.INCREASING,
        State.CHANGING,
        State.DECREASING,
        State.PAUSED,
        State.RUNNING,
        State.ACTIVE,
        State.PASSIVE,
        State.DISABLED,
    ]
