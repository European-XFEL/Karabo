""""Scantool macros"""
from asyncio import coroutine

from karabo.middlelayer import State, waitUntil
from .usermacro import UserMacro
from .genericproxy import Movable, Sensible


class AScan(UserMacro):
    """Absolute scan"""
    _movable = Movable()
    _pos_list = []
    _sensible = Sensible()

    def __init__(self, movable, pos_list, sensible, exposure_time, steps=True, number_of_steps=0, **kwargs):
        super().__init__(kwargs)
        self._movable = movable if isinstance(movable, Movable) else Movable(movable)
        self._pos_list = pos_list
        self._sensible = sensible if isinstance(sensible, Sensible) else Sensible(sensible)

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""

        self._movable.prepare()
        #self._sensible.prepare()

        for i, pos in enumerate(self._pos_list):
            print("Step {} - Motors at {}".format(i, self._movable.position))
            yield from self._movable.moveto(pos)
            yield from waitUntil(lambda: self._movable.state == State.STOPPED)
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
