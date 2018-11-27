from abc import ABCMeta, abstractmethod

from karathon import (
    OVERWRITE_ELEMENT, SLOT_ELEMENT, IMAGEDATA_ELEMENT, OUTPUT_CHANNEL,
    DOUBLE_ELEMENT, NODE_ELEMENT, INT32_ELEMENT, DaqDataType, Schema, Unit,
    VECTOR_STRING_ELEMENT)
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .no_fsm import NoFsm
from karabo.common.states import State

__author__ = "esenov"
__date__ = "$Apr 22, 2015 4:14:47 PM$"


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("CameraInterface", "1.4")
class CameraInterface(NoFsm, metaclass=ABCMeta):

    @staticmethod
    def expectedParameters(expected):
        (
        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State.INIT, State.UNKNOWN, State.ERROR, State.ACQUIRING, State.CHANGING, State.ON)
                .setNewDefaultValue(State.INIT)
                .commit(),

        SLOT_ELEMENT(expected).key("connectCamera")
                .displayedName("Connect")
                .description("Connects to the hardware")
                .allowedStates(State.UNKNOWN)
                .commit(),

        SLOT_ELEMENT(expected).key("acquire")
                .displayedName("Acquire")
                .description("Instructs camera to go into acquisition state")
                .allowedStates(State.ON)
                .commit(),

        SLOT_ELEMENT(expected).key("trigger")
                .displayedName("Trigger")
                .description("Sends a software trigger to the camera")
                .allowedStates(State.ACQUIRING)
                .commit(),

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Instructs camera to stop current acquisition")
                .allowedStates(State.ACQUIRING)
                .commit(),

        SLOT_ELEMENT(expected).key("resetHardware")
                .displayedName("Reset")
                .description("Resets the camera in case of an error")
                .allowedStates(State.ERROR)
                .commit(),

        VECTOR_STRING_ELEMENT(expected).key("interfaces")
                .displayedName("Interfaces")
                .description("Describes the interfaces of this device")
                .readOnly()
                .initialValue(["Camera"])
                .commit(),
        )

        data = Schema()

        (
        NODE_ELEMENT(data).key("data")
                .displayedName("Data")
                .setDaqDataType(DaqDataType.TRAIN)
                .commit(),

        IMAGEDATA_ELEMENT(data).key("data.image")
                .commit(),

        OUTPUT_CHANNEL(expected).key("output")
                .displayedName("Output")
                .dataSchema(data)
                .commit(),

        DOUBLE_ELEMENT(expected).key("exposureTime")
                .displayedName("Exposure Time")
                .description("The requested exposure time in seconds")
                .unit(Unit.SECOND)
                .assignmentOptional().defaultValue(1.0)
                .minInc(0.02)
                .maxInc(5.0)
                .reconfigurable()
                .commit(),

        INT32_ELEMENT(expected).key("pollInterval")
                .displayedName("Poll Interval")
                .description("The interval with which the camera should be polled")
                .unit(Unit.SECOND)
                .minInc(1)
                .assignmentOptional().defaultValue(10)
                .reconfigurable()
                .allowedStates(State.ON, State.ACQUIRING, State.ERROR)
                .commit(),

        )

    def __init__(self, configuration):
        super(CameraInterface, self).__init__(configuration)
        self.registerInitialFunction(self.initialize)

    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.connectCamera)
        sigslot.registerSlot(self.acquire)
        sigslot.registerSlot(self.trigger)
        sigslot.registerSlot(self.stop)
        sigslot.registerSlot(self.resetHardware)

    @abstractmethod
    def initialize(self):
        """
        This method is called when 'startFsm()' function is called.
        """

    @abstractmethod
    def connectCamera(self):
        """
        The method is called as a result of processing "connect" Event.
        """

    @abstractmethod
    def acquire(self):
        """
        The method is called as a result of processing "acquire" Event.
        """

    @abstractmethod
    def trigger(self):
        """
        The method is called during processing of a "trigger" event
        """

    @abstractmethod
    def stop(self):
        """
        The method is called during processing of a "stop" event.
        """

    @abstractmethod
    def resetHardware(self):
        """
        Reset the camera hardware, i.e. bring it to some safe initial state.
        """
