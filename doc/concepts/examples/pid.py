from karabo.middlelayer import (
    AccessMode, Assignment, background, Device, DeviceNode, Float,
    getDevice, locked, Slot, State, String, Unit, waitUntilNew)


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
        defaultValue=None,
        accessMode=AccessMode.RECONFIGURABLE)

    process_device = DeviceNode(
        displayedName="Process Device",
        description="The device of the process variable")

    process_property = String(
        displayedName="Process Variable",
        description="The property of the process variable",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY)

    control_device = DeviceNode(
        displayedName="Control Device",
        description="The device of the control variable")

    control_property = String(
        displayedName="Control Variable",
        description="The property name of the control variable",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY)

    control_command = String(
        displayedName="Control Command",
        description="The property name of the slot to execute on "
                    "the control device",
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.MANDATORY)

    @Slot(displayedName="Start Controlling",
          allowedStates=[State.NORMAL])
    def start(self):
        """Start controlling the process"""
        self.state = State.RUNNING
        self.background = background(self.runner, callback=self.finished)

    def finished(self, future):
        self.state = State.NORMAL

    @Slot(displayedName="Stop Controlling",
          allowedStates=[State.RUNNING])
    def stop(self):
        """Stop controlling the process"""
        self.background.cancel()

    @Slot(displayedName="Update")
    def update(self):
        """Update set point"""
        self.setpoint = self.target_setpoint

    @Slot(displayedName="Reset Integral")
    def reset_integral(self):
        """reset the integral term of the controller"""
        self.current_integral = 0

    def onInitialization(self):
        self.state = State.NORMAL

    def runner(self):
        command = getattr(self.control_device, self.control_command)
        last_error = (getattr(self.process_device, self.process_property) -
                      self.setpoint)
        while True:
            waitUntilNew(getattr(self.process_device, self.process_property))

            error = (getattr(self.process_device, self.process_property) -
                     self.setpoint)
            self.current_integral += error * (error.timestamp -
                                              last_error.timestamp)
            derivative = ((last_error - error) /
                          (last_error.timestamp - error.timestamp))
            control = (self.proportional * error +
                       self.integral * self.current_integral +
                       self.derivative * derivative)
            setattr(self.control_device, self.control_property, control)
            command()

            last_error = error
