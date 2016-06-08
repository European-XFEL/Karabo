from numpy import linspace

from karabo.middlelayer import (
    AccessMode, Assignment, background, Device, Float, getDevice, Integer,
    locked, Slot, State, String, Unit, waitUntil)


class Scan(Device):
    """Scan a device and activate anoter device at each step

    This device orders one device, the *actor*, to move through steps, and at
    each position calls a command on another device, the *sensor*, and waits
    until the latter is done."""

    start_pos = Float(
        displayedName="Start Position",
        accessMode=AccessMode.RECONFIGURABLE)

    stop_pos = Float(
        displayedName="Stop Position",
        accessMode=AccessMode.RECONFIGURABLE)

    steps = Integer(
        displayedName="Number of steps",
        accessMode=AccessMode.RECONFIGURABLE,
        unitSymbol=Unit.COUNT)

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

    sensor = String(
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
        self.background = background(self.runner, callback=None)

    @Slot(displayedName="Cancel Scan",
          allowedStates=[State.MOVING])
    def cancel(self):
        """Cancel a running scan"""
        self.background.cancel()

    def runner(self):
        try:
            actor_name, actor_prop = self.actor_property.split(".", 2)
            if self.actor_command:
                command_name, command_slot = self.actor_command.split(".", 2)
                assert actor_name == command_name
            else:
                command_slot = None
            sensor_name, sensor_slot = self.sensor.split(".", 2)
        except:
            self.log.exception("A remote device property seems to be wrong")
            raise

        try:
            self.start_pos, self.stop_pos, self.steps
        except:
            self.log.exception(
                "start, stop and steps must be set before running")
            raise

        self.state = State.MOVING
        try:
            with locked(getDevice(actor_name)) as actor, \
                    locked(getDevice(sensor_name)) as sensor:
                if command_slot is None:
                    command = lambda: None
                else:
                    command = getattr(actor, command_slot)
                sense = getattr(sensor, sensor_slot)
                for position in linspace(self.start_pos, self.stop_pos,
                                         self.steps):
                    setattr(actor, self.actor_property, position)
                    command()
                    waitUntil(lambda:
                              getattr(actor, self.actor_property).onTarget)
                    sense()
                    waitUntil(lambda: sensor.state == State.PASSIVE)
        finally:
            self.state = State.STOPPED
