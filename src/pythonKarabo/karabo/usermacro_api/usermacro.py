"""User Macros implement as-simple-as-possible
   domain-level routines for Light-Source operations
"""
import argparse
from asyncio import (
    coroutine, get_event_loop, set_event_loop)
import socket
import uuid

from karabo.middlelayer import (
    AccessLevel, AccessMode, Bool, Device, DeviceClientBase,
    Float, Macro, MetricPrefix, Slot, State, Unit)
from karabo.middlelayer_api.eventloop import EventLoop, NoEventLoop
from karabo.middlelayer_api.macro import EventThread
from karabo.usermacro_api.pipeline import OutputChannel


def run_in_event_loop(macro, *args, **kwargs):
    """ Runs a macro"""
    class DeviceHelper(Macro, DeviceClientBase):
        pass
    loop = get_event_loop()

    eventThread = None

    if not isinstance(loop, EventLoop):
        if EventLoop.global_loop is None:
            # The user macro is started from command line for e.g.
            # In this case, start a device helper in a new event thread
            # to provide services like connectDevice() on __init__.
            eventThread = EventThread()
            eventThread.start()
            helper = DeviceHelper(_deviceId_=uuid.uuid4())
            set_event_loop(NoEventLoop(helper))
        loop = EventLoop.global_loop

        if isinstance(macro, type):
            # macro is a class, instantiate
            macro = macro(*args, **kwargs)

    @coroutine
    def __run():
        macro.state = State.ACTIVE
        macro.currentSlot = "start"
        yield from macro.execute()
        macro.currentSlot = ""
        macro.state = State.PASSIVE
        yield from macro.slotKillDevice()

        if eventThread:
            eventThread.stop()

    loop.call_soon_threadsafe(loop.create_task, __run())
    # Must wait here due to the scope EventThread
    if eventThread:
            eventThread.join()

class UserMacro(Macro):
    """"Base class for user macros"""
    position_epsilon = Float(
        displayedName="PositionEpsilon",
        defaultValue=1e-1,
        unitSymbol=Unit.METER,
        metricPrefixSymbol=MetricPrefix.MILLI,
        accessMode=AccessMode.INITONLY)

    time_epsilon = Float(
        displayedName="TimeEpsilon",
        defaultValue=1e-1,
        unitSymbol=Unit.SECOND,
        accessMode=AccessMode.INITONLY)

    cancelled = Bool(
        displayedName="Cancelled",
        defaultValue=False,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    outputChannel = OutputChannel()

    def __init__(self, **kwargs):
        bareHostName = socket.gethostname().partition('.')[0]
        deviceId = "{}_{}_{}".format(
            type(self).__name__, bareHostName, str(uuid.uuid4()))
        if "deviceId" in kwargs:
            kwargs["_deviceId_"] = kwargs["deviceId"]
        else:
            kwargs["_deviceId_"] = kwargs["deviceId"] = deviceId
        super().__init__(kwargs)

    @Slot(displayedName="Start", allowedStates={State.PASSIVE})
    def start(self):
        """Start the user macro"""
        self.__call__()

    @Slot(displayedName="Cancel", allowedStates={State.ACTIVE})
    def cancel(self):
        """Duplicated here due to a bug in the Macro class"""
        self.cancelled = True

    @coroutine
    def _run(self, **kwargs):
        """Skip the expert macro RemoteDevice functionality"""
        self.state = State.PASSIVE
        yield from Device._run(self, **kwargs)

    def __call__(self):
        run_in_event_loop(self)

    @coroutine
    def execute(self):
        """Insert here the code of a UserMacro"""

    @classmethod
    def main(cls):
        """Parse from command line"""
        kwargs = {}
        parser = argparse.ArgumentParser(
            description="Run a {}.".format(cls.__name__))
        parser.add_argument(
            "--id", nargs="?",
            metavar="deviceId", type=str, help="The macro DeviceId")
        parser.add_argument(
            "arg", nargs="*", type=str, help="Argument")
        kwargs = vars(parser.parse_args())
        args = kwargs["arg"]
        if args:
            del kwargs["arg"]

        run_in_event_loop(cls, *args, **kwargs)


def display(image):
    """Display an image into karabo GUI"""
    instance = get_event_loop().instance()
    instance.outputChannel = image


if __name__ == "__main__":
    UserMacro.main()
