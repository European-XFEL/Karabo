from numpy import linspace

from karabo.middlelayer import (
    AccessMode, Assignment, background, Device, DeviceNode, Float, getDevice,
    Integer, locked, Slot, State, String, Unit, waitUntil)


class Scan(Device):
    """Scan a device and activate anoter device at each step

    This device orders one device, the *actor*, to move through steps, and at
    each position calls a command on another device, the *sensor*, and waits
    until the latter is done."""

    start_pos = Float(
        displayedName="Start Position",
        defaultValue=None,
        accessMode=AccessMode.RECONFIGURABLE)

    stop_pos = Float(
        displayedName="Stop Position",
        defaultValue=None,
        accessMode=AccessMode.RECONFIGURABLE)

    steps = Integer(
        displayedName="Number of steps",
        defaultValue=None,
        accessMode=AccessMode.RECONFIGURABLE,
        unitSymbol=Unit.COUNT)

    actor_device = DeviceNode(
        displayedName="Actor device",
        description="The device that is scanned")

    actor_property = String(
        displayedName="Actor's property",
        description="The actor's property that is altered as part of the scan",
        displayType="property",
        assignment=Assignment.MANDATORY,
        accessMode=AccessMode.INITONLY)

    actor_command = String(
        displayedName="Actor's command",
        description="the actor's command executed at each step in the scan "
                    "(leave empty if not applicable)",
        displayType="remoteSlot",
        defaultValue="",
        accessMode=AccessMode.INITONLY)

    actor_device = DeviceNode(
        displayedName="Sensor device",
        description="The device that is to be triggered at each scan step")

    sensor_command = String(
        displayedName="Sensor's command",
        description="The sensor triggered at each step in the scan",
        displayType="remoteSlot",
        assignment=Assignment.MANDATORY,
        accessMode=AccessMode.INITONLY)

    def onInitialization(self):
        self.state = State.STOPPED

    @Slot(displayedName="Start Scan",
          allowedStates=[State.STOPPED])
    def start(self):
        """Start a scan"""
        self.background = background(self.runner)

    @Slot(displayedName="Cancel Scan",
          allowedStates=[State.MOVING])
    def cancel(self):
        """Cancel a running scan"""
        self.background.cancel()

    def runner(self):
        if None in (self.start_pos, self.stop_pos, self.steps):
            self.logger.error(
                "start, stop and steps must be set before running")
            return

        self.state = State.MOVING
        try:
            if self.actor_command is None:
                command = lambda: None
            else:
                command = getattr(self.actor_device, self.actor_command)
                sense = getattr(self.sensor_device, self.sensor_command)
            for position in linspace(self.start_pos, self.stop_pos, self.steps):
                setattr(self.actor_device, self.actor_property, position)
                command()
                waitUntil(lambda: getattr(self.actor_device,
                                          self.actor_property).onTarget)
                sense()
                waitUntil(lambda: self.sensor_device.state == State.PASSIVE)
        finally:
            self.state = State.STOPPED
