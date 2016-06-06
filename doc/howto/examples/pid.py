from karabo.middlelayer import (
    AccessMode, Assignment, background, Device, Float,
    getDevice, Slot, State, String, Unit, waitUntilNew)


class PID(Device):
    """A generic PID controller

    A PID controller tries to minimize the difference between the *process
    variable* of a device and a user defined *setpoint* by acting on a *control
    variable*, often on a different device.

    The time evolution of this difference, the *error*, is fed back to the
    control variable using the error itself, its integral and its derivative.
    """

    proportional = Float(
        displayedName="Proportional Term",
        description="Feed back the error proportionally with this factor",
        accessMode=AccessMode.RECONFIGURABLE,
        assignment=Assignment.MANDATORY,
        unitSymbol=Unit.NUMBER)

    integral = Float(
        displayedName="Integral Term",
        description="Feed back the integral of the error with this factor",
        accessMode=AccessMode.RECONFIGURABLE,
        assignment=Assignment.MANDATORY,
        unitSymbol=Unit.HERTZ)

    current_integral = Float(
        displayedName="Current Integral Value",
        description="The current value of the error integral",
        accessMode=AccessMode.READONLY,
        unitSymbol=Unit.SECOND)

    derivative = Float(
        displayedName="Derivative Term",
        description="Feed back the derivative of the error with this factor",
        accessMode=AccessMode.RECONFIGURABLE,
        assignment=Assignment.MANDATORY,
        unitSymbol=Unit.SECOND)

    setpoint = Float(
        displayedName="Current Setpoint",
        description="The currently active set point",
        accessMode=AccessMode.READONLY)

    target_setpoint = Float(
        displayedName="Target Setpoint",
        description="The target set point",
        accessMode=AccessMode.RECONFIGURABLE)

    process = String(
        displayedName="Process Variable",
        description="The device and property name of the process variable",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY,
        displayType="property")

    control = String(
        displayedName="Control Variable",
        description="The device and property name of the control variable",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY,
        displayType="property")

    command = String(
        displayedName="Control Command",
        description="The device and property name of the slot to execute on "
                    "the control device",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY,
        displayType="remoteSlot")

    @Slot(displayedName="Go")
    def go(self):
        """set the setpoint to the target setpoint"""
        self.setpoint = self.target_setpoint

    @Slot(displayedName="Reset Integral")
    def reset_integral(self):
        """reset the integral term of the controller"""
        self.current_integral = 0

    def onInitialization(self):
        background(self.runner, callback=None)
        self.state = State.NORMAL

    def runner(self):
        process_name, process_property = self.process.split(".", 2)
        control_name, control_property = self.control.split(".", 2)
        command_name, command_slot = self.command.split(".", 2)
        assert command_name == control_name

        with getDevice(control_name) as control_device, \
                getDevice(process_name) as process_device:
            command = getattr(control_device, command_slot)
            last_error = getattr(process_device, process_name) - self.setpoint
            while True:
                waitUntilNew(getattr(process_device, process_name))

                error = getattr(process_device, process_name) - self.setpoint
                self.current_integral += error * (error.timestamp -
                                                  last_error.timestamp)
                derivative = ((last_error - error) /
                              (last_error.timestamp - error.timestamp))
                control = (self.proportional * error +
                           self.integral * self.current_integral +
                           self.derivative * derivative)
                setattr(control_device, control_name, control)
                command()

                last_error = error
