from abc import ABCMeta
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.no_fsm import NoFsm

__author__ = "esenov"
__date__ = "$Apr 22, 2015 4:14:47 PM$"


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("MotorInterface", "1.3")
class MotorInterface(NoFsm):

    __metaclass__ = ABCMeta

    @staticmethod
    def expectedParameters(expected):
        (
         OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions("Initializing, HardwareError,Safe,Override,Off,"
                               "Stopped,Idle,Homing,Moving")
                .setNewDefaultValue("Initializing")
                .commit(),

         SLOT_ELEMENT(expected).key("resetHardware")
                .description("Resets the hardware")
                .displayedName("Reset hardware")
                .allowedStates("HardwareError")
                .commit(),

         SLOT_ELEMENT(expected).key("safe")
                .description("Brings device into a safe state (as defined on "
                             "h/w)")
                .displayedName("Safe")
                .commit(),

         SLOT_ELEMENT(expected).key("normal")
                .displayedName("Normal")
                .description("Brings device into normal mode")
                .expertAccess()
                .commit(),

         SLOT_ELEMENT(expected).key("override")
                .displayedName("Override")
                .description("Brings device into override mode (be careful, "
                             "hardware may be broken)")
                .expertAccess()
                .commit(),

         SLOT_ELEMENT(expected).key("off")
                .displayedName("Off")
                .description("Instructs device to switch off")
                .allowedStates("Override Stopped Idle Homing")
                .commit(),

         SLOT_ELEMENT(expected).key("on")
                .displayedName("On")
                .description("Instructs device to switch on")
                .allowedStates("Override Off")
                .commit(),

         SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Instructs the device to switch on and stopped")
                .allowedStates("Override Idle Moving Homing")
                .commit(),

         SLOT_ELEMENT(expected).key("home")
                .displayedName("Home")
                .description("Find home position")
                .allowedStates("Override Stopped")
                .commit(),

         SLOT_ELEMENT(expected).key("move")
                .displayedName("Move")
                .description("Move position")
                .allowedStates("Override Stopped")
                .commit(),

         SLOT_ELEMENT(expected).key("stepUp")
                .displayedName("Step up")
                .description("Step up")
                .allowedStates("Override Idle Stopped")
                .commit(),

         SLOT_ELEMENT(expected).key("stepDown")
                .displayedName("Step down")
                .description("Step down")
                .allowedStates("Override Idle Stopped")
                .commit(),

         FLOAT_ELEMENT(expected).key("encoderPosition")
                .description("Encoder position")
                .displayedName("Encoder position")
                .unit(METER)
                .metricPrefix(MILLI)
                .readOnly()
                .commit(),

         FLOAT_ELEMENT(expected).key("stepCounterPosition")
                .displayedName("Stepcounter position")
                .description("The step counter position describes the motor "
                             "position calculated from counter steps (instead "
                             "of encoder values), and is only valid if "
                             "connected to external encoder")
                .expertAccess()
                .readOnly()
                .commit(),

         FLOAT_ELEMENT(expected).key("targetPosition")
                .description("Target position in position mode")
                .displayedName("Target position")
                .unit(METER)
                .metricPrefix(MILLI)
                .assignmentOptional().noDefaultValue()
                .reconfigurable()
                .allowedStates("Override Stopped Off Idle Moving")
                .commit(),

         INT16_ELEMENT(expected).key("targetVelocity")
                .description("Target velocity in velocity mode")
                .displayedName("Target velocity")
                .assignmentOptional().noDefaultValue()
                .reconfigurable()
                .allowedStates("Override Stopped Off Idle Moving")
                .expertAccess()
                .commit(),
        )

    def __init__(self, configuration):
        super(MotorInterface, self).__init__(configuration)
        self.registerInitialFunction(self.initialize)

    def initFsmSlots(self, sigslot):
        #sigslot.setNumberOfThreads(1)
        sigslot.registerSlot(self.resetHardware)
        sigslot.registerSlot(self.safe)
        sigslot.registerSlot(self.normal)
        sigslot.registerSlot(self.override)
        sigslot.registerSlot(self.off)
        sigslot.registerSlot(self.on)
        sigslot.registerSlot(self.stop)
        sigslot.registerSlot(self.home)
        sigslot.registerSlot(self.move)
        sigslot.registerSlot(self.stepUp)
        sigslot.registerSlot(self.stepDown)

    @abstractmethod
    def resetHardware(self):
        """
        Reset the motor hardware, i.e. bring it to some default initial state.
        """

    @abstractmethod
    def safe(self):
        """Bring the motor to the 'safe' state (predefined in hardware)"""

    @abstractmethod
    def normal(self):
        """Bring the motor to the 'normal' mode."""

    @abstractmethod
    def override(self):
        """Bring the motor to the 'override' mode."""

    @abstractmethod
    def off(self):
        """Switch the motor off (power off)"""

    @abstractmethod
    def on(self):
        """Switch the motor on (power on)"""

    @abstractmethod
    def stop(self):
        """Stop moving the motor or switch on if not"""

    @abstractmethod
    def home(self):
        """Bring the motor to 'home' position"""

    @abstractmethod
    def move(self):
        """Move the motor to reach the target position"""

    @abstractmethod
    def stepUp(self):
        """Move the motor to make one step up"""

    @abstractmethod
    def stepDown(self):
        """Move the motor to make one step down"""

    @abstractmethod
    def initialize(self):
        """Initialize the motor hardware and load motor properties"""
