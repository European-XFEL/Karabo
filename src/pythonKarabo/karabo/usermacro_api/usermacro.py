"""User Macros implement as-simple-as-possible
   domain-level routines for Light-Source operations
"""
from asyncio import coroutine, get_event_loop, set_event_loop
from contextlib import closing
import os
import socket

from karabo.middlelayer import Device, Macro, Slot, State
from karabo.middlelayer_api.eventloop import EventLoop, NoEventLoop
from .pipeline import OutputChannel


class UserMacro(Macro):
    """"Base class for user macros"""
    position_epsilon = 1e-2
    outputChannel = OutputChannel()

    @Slot(displayedName="Start", allowedStates={State.PASSIVE})
    def start(self):
        """Start the user macro"""
        self.__call__()

    @coroutine
    def _run(self, **kwargs):
        """Skip the expert macro RemoteDevice functionality"""
        self.state = State.PASSIVE
        # yield from Device._run(self, **kwargs)

    @staticmethod
    def run_in_event_loop(macro):
        """ To run macro, reuse the running event loop if any"""
        loop = get_event_loop()
        if isinstance(loop, NoEventLoop):
            if EventLoop.global_loop is not None:
                loop = EventLoop.global_loop
            else:
                loop = EventLoop()
                set_event_loop(loop)

        @coroutine
        def __run():
            macro.state = State.ACTIVE
            yield from macro.execute()
            macro.state = State.PASSIVE
            yield from macro.slotKillDevice()

        if loop.is_running():
            loop.call_soon_threadsafe(
                loop.create_task, __run())
        else:
            with closing(loop):
                loop.run_until_complete(__run())

    def __call__(self):
        type(self).run_in_event_loop(self)

    @coroutine
    def execute(self):
        """Insert here the code of a UserMacro"""

    @classmethod
    def main(cls, **kwargs):

        if "deviceId" in kwargs:
            kwargs["_deviceId_"] = kwargs["deviceId"]
        else:
            bareHostName = socket.gethostname().partition('.')[0]
            kwargs["_deviceId_"] = "{}_{}_{}".format(
                cls.__name__, bareHostName, os.getpid())

        cls.run_in_event_loop(cls(kwargs))


def display(image):
    """Display an image into karabo GUI"""
    instance = get_event_loop().instance()
    instance.outputChannel = image
