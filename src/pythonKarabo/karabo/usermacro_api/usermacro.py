"""User Macros implement as-simple-as-possible
   domain-level routines for Light-Source operations
"""
import argparse
from asyncio import (
    async, coroutine, Future, get_event_loop, set_event_loop)
import tempfile
import socket
import uuid
import sys

from karabo.usermacro_api.middlelayer import (
    AccessLevel, AccessMode, Bool, Device, DeviceClientBase,
    EventThread, EventLoop, Float, Macro, MetricPrefix,
    NoEventLoop, Slot, State, synchronize, Unit)
from karabo.usermacro_api.pipeline import OutputChannel


@coroutine
def run_usermacro(macro, eventThread=None):
    """Run and terminate a User Macro"""
    macro.state = State.ACTIVE
    macro.currentSlot = "start"
    data = yield from macro.execute()
    macro.currentSlot = ""
    macro.state = State.PASSIVE
    yield from macro.slotKillDevice()

    if eventThread:
        eventThread.stop()
    return data


def run_in_event_loop(macro, *args, **kwargs):
    """ Runs a macro"""

    class DeviceHelper(Macro, DeviceClientBase):
        """Provide the device client machinery"""

    loop = get_event_loop()
    eventThread = None
    helper_uuid = None

    if not isinstance(loop, EventLoop):
        if EventLoop.global_loop is None:
            # The user macro is started from command line for e.g.
            # In this case, start the unique event loop in a separated thread,
            # and provide a device helper in the current thread
            # to provide services like connectDevice() on __init__.
            eventThread = EventThread()
            eventThread.start()

            bareHostName = socket.gethostname().partition('.')[0]
            kwargs["uuid"] = uuid.uuid4()
            helper = DeviceHelper(_deviceId_="DeviceHelper_{}_{}"
                                  .format(bareHostName, helper_uuid))
            set_event_loop(NoEventLoop(helper))

        loop = EventLoop.global_loop
        # Mute the event loop
        loop.set_debug(False)

        if isinstance(macro, type):
            # macro is a class, instantiate
            macro = macro(*args, **kwargs)
    try:
        task = loop.create_task(run_usermacro(macro, eventThread))
        loop.call_soon_threadsafe(async, task)
        # Must wait here due to the scope EventThread
        if eventThread:
            eventThread.join()
        return task

    except KeyboardInterrupt:
        macro.cancelled = True
        print("{} cancelled.".format(macro.deviceId))


class UserMacro(Macro):
    """"Base class for user macros"""
    position_epsilon = Float(
        displayedName="PositionEpsilon",
        defaultValue=1e-1,
        unitSymbol=Unit.METER,
        metricPrefixSymbol=MetricPrefix.MILLI)

    time_epsilon = Float(
        displayedName="TimeEpsilon",
        defaultValue=1e-4,
        unitSymbol=Unit.SECOND)

    cancelled = Bool(
        displayedName="Cancelled",
        defaultValue=False,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    outputChannel = OutputChannel()

    @classmethod
    def register(cls, name, dic):
        """Do not register UserMacro API classes
        in Macro but in Device

        The reason is that middlelayer MetaMacro
        tries to instantiate registered Macro subclasses
        """
        if "karabo.usermacro" not in cls.__module__:
            super().register(name, dic)
        else:
            Device.register.__func__(cls, name, dic)

    def __new__(cls, *args, **kwargs):
        """Return a dummy object instantiated"""
        if cls is UserMacro:
            f = Future()
            f.set_result(None)
            return (
                type("NoUserMacro", (object,),
                     dict(startInstance=lambda _: f)))
        else:
            return super().__new__(cls)

    def __init__(self, *args, **kwargs):
        if args and isinstance(args[0], dict):
            kwargs.update(args[0])

        bareHostName = socket.gethostname().partition('.')[0]
        deviceId = "{}_{}_{}".format(
            type(self).__name__, bareHostName,
            kwargs.get("uuid", str(uuid.uuid4())))

        if "_deviceId_" not in kwargs:
            if "deviceId" in kwargs:
                kwargs["_deviceId_"] = kwargs["deviceId"]
            else:
                kwargs["_deviceId_"] = kwargs["deviceId"] = deviceId

        super().__init__(kwargs)

    def __del__(self):
        # Silencing this method
        sys.stderr = tempfile.NamedTemporaryFile()
        super().__del__()

    def _initInfo(self):
        info = super()._initInfo()
        # Needed for being seen in the topology and being logged
        info["type"] = "device"
        return info

    @Slot(displayedName="Start", allowedStates={State.PASSIVE})
    @synchronize
    def start(self):
        """Start the user macro"""
        data = yield from self.__call__()
        return data

    @Slot(displayedName="Cancel", allowedStates={State.ACTIVE})
    def cancel(self):
        """Duplicated here due to a bug in the Macro class"""
        self.cancelled = True

    @coroutine
    def _run(self, **kwargs):
        """Skip the expert macro RemoteDevice functionality"""
        self.state = State.PASSIVE
        yield from Device._run(self, **kwargs)

    @synchronize
    def __call__(self):
        data = yield from run_usermacro(self)
        return data

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

        if kwargs.get("id"):
            kwargs["deviceId"] = kwargs["id"]
            del kwargs["id"]

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
