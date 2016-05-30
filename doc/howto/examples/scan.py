from numpy import linspace

from karabo.middlelayer import (
    AccessMode, background, Device, Float, getDevice, Integer, Slot, State,
    String, Unit, waitUntil)


class Scan(Device):
    """Scan a device and activate anoter device at each step

    This device orders one device, the *actor*, to move through steps, and at
    each position calls a command on another device, the *sensor*, and waits
    until the latter is done."""

    start = Float(
        displayedName="Start Position",
        accessMode=AccessMode.RECONFIGURABLE)

    stop = Float(
        displayedName="Stop Position",
        accessMode=AccessMode.RECONFIGURABLE)

    steps = Integer(
        displayedName="Number of steps",
        accessMode=AccessMode.RECONFIGURABLE,
        unitSymbol=Unit.COUNT)

    actor_property = String(
        displayedName="Actor's property",
        description="the actor's property that is moved in this scan",
        displayType="property",
        accessMode=AccessMode.INITONLY)

    actor_command = String(
        displayedName="Actor's command",
        description="the actor's command that is moved in this scan",
        displayType="remoteSlot",
        accessMode=AccessMode.INITONLY)

    sensor = String(
        displayedName="Sensor's command",
        description="The sensor triggered at each step",
        displayType="remoteSlot",
        accessMode=AccessMode.INITONLY)

    def onInitialization(self):
        self.state = State.STOPPED

    @Slot()
    def start(self):
        self.background = background(self.runner, callback=None)

    @Slot()
    def cancel(self):
        self.background.cancel()

    def runner(self):
        actor_name, actor_prop = self.actor_property.split(".", 2)
        command_name, command_slot = self.actor_command.split(".", 2)
        assert actor_name == command_name
        sensor_name, sensor_slot = self.sensor.split(".", 2)

        self.state = State.MOVING
        try:
            with getDevice(actor_name) as actor, \
                    getDevice(sensor_name) as sensor:
                for position in linspace(self.start, self.stop, self.steps):
                    setattr(actor, self.actor_property, position)
                    getattr(actor, command_slot)()
                    waitUntil(lambda: getattr(actor, self.actor_property).
                              onTarget)
                    getattr(sensor, sensor_slot)()
                    waitUntil(lambda: sensor.state == State.PASSIVE)
        finally:
            self.state = State.STOPPED
