""""Scantool macros"""
from asyncio import coroutine

from karabo.middlelayer import Bool, Float, Int32, State, String, waitUntil
from .usermacro import UserMacro
from .genericproxy import Movable, Sensible


class AScan(UserMacro):
    """Absolute scan"""
    _movableId = String(displayedName="Movable")
    _sensibleId = String(displayedName="Sensible")
    _movable = Movable()
    _sensible = Sensible()
    _pos_list = []
    exposure_time = Float(displayedName="Exposure time", defaultValue=0.001)
    steps = Bool(displayedName="Steps", defaultValue=True)
    number_of_steps = Int32(displayedName="Number of steps", defaultValue=0)

    def __init__(self, movable, pos_list, sensible, exposure_time,
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(kwargs)
        self._movable = movable if isinstance(
                                       movable, Movable) else Movable(movable)
        self._movableId = self._movable.deviceId
        self._pos_list = pos_list
        self._sensible = sensible if isinstance(
                                       sensible, Sensible) else Sensible(
                                                                    sensible)
        self._sensibleId = self._sensible.deviceId
        self._exposure_time = exposure_time
        self._steps = steps
        self._number_of_steps = number_of_steps

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""

        self._movable.prepare()
        self._sensible.prepare()

        for i, pos in enumerate(self._pos_list):
            yield from self._movable.moveto(pos)
            yield from waitUntil(lambda: self._movable.state != State.MOVING)
            print("Step {} - Motors at {}"
                  .format(i + 1, self._movable.position))
            yield from self._sensible.acquire()


class AMesh(AScan):
    """Absolute Mesh scan"""


class APathScan(AScan):
    """Absolute Mesh scan"""


class DScan(AScan):
    """Delta scan"""


class TScan(UserMacro):
    """Time scan"""


class DMesh(AScan):
    """Delta Mesh scan"""


class AMove(UserMacro):
    """Absolute move"""


class DMove(UserMacro):
    """Delta move"""
