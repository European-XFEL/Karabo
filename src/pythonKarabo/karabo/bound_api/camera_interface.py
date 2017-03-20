from abc import ABCMeta, abstractmethod

from karathon import (
    OVERWRITE_ELEMENT, SLOT_ELEMENT, IMAGEDATA_ELEMENT, OUTPUT_CHANNEL,
    DOUBLE_ELEMENT, NODE_ELEMENT, BOOL_ELEMENT, PATH_ELEMENT,
    STRING_ELEMENT, INT32_ELEMENT, Schema, SignalSlotable, Unit)
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .no_fsm import NoFsm
from karabo.common.states import State

__author__ = "esenov"
__date__ = "$Apr 22, 2015 4:14:47 PM$"


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("CameraInterface", "1.3")
class CameraInterface(NoFsm, metaclass=ABCMeta):

    @staticmethod
    def expectedParameters(expected):
        (
        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State.INIT, State.UNKNOWN, State.ERROR, State.ACQUIRING, State.CHANGING, State.ACTIVE)
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
                .allowedStates(State.ACTIVE)
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
        )
        
        data = Schema()
        
        (
        IMAGEDATA_ELEMENT(data).key("image")
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

        NODE_ELEMENT(expected).key("imageStorage")
                .displayedName("Local Image Storage")
                .commit(),

        BOOL_ELEMENT(expected).key("imageStorage.enable")
                .displayedName("Enable")
                .description("Save images while acquiring.")
                .assignmentOptional().defaultValue(False)
                .reconfigurable()
                .allowedStates(State.ACTIVE)
                .commit(),

        PATH_ELEMENT(expected).key("imageStorage.filePath")
                .displayedName("File Path")
                .description("The path for saving images to file")
                .isDirectory()
                .assignmentOptional().defaultValue("/tmp")
                .reconfigurable()
                .allowedStates(State.ACTIVE)
                .commit(),

        STRING_ELEMENT(expected).key("imageStorage.fileName")
                .displayedName("File Name")
                .description("The name for saving images to file")
                .assignmentOptional().defaultValue("image")
                .reconfigurable()

                .allowedStates(State.ACTIVE)
                .commit(),

        STRING_ELEMENT(expected).key("imageStorage.fileType")
                .displayedName("File Type")
                .description("The image format to be used for writing to file")
                .assignmentOptional().defaultValue("tif")
                .options("tif jpg png")
                .reconfigurable()
                .allowedStates(State.ACTIVE)
                .commit(),

        STRING_ELEMENT(expected).key("imageStorage.lastSaved")
                .displayedName("Last Saved")
                .description("The name of the last saved image")
                .readOnly()
                .commit(),

        INT32_ELEMENT(expected).key("pollInterval")
                .displayedName("Poll Interval")
                .description("The interval with which the camera should be polled")
                .unit(Unit.SECOND)
                .minInc(1)
                .assignmentOptional().defaultValue(10)
                .reconfigurable()
                .allowedStates(State.ACTIVE, State.ACQUIRING, State.ERROR)
                .commit(),

        )

    def __init__(self, configuration):
        super(CameraInterface, self).__init__(configuration)
        self.registerInitialFunction(self.initialize)

    def initFsmSlots(self, sigslot):
        #sigslot.setNumberOfThreads(1)
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

