from karabo.middlelayer import (
    Device, DeviceNode, Float, Slot, State, waitUntilNew)


class DoubleConveyor(Device):
    props = [{"currentSpeed": "speed"}, "targetSpeed"]
    cmds = ["reset", "reset"]
    left_conveyor = DeviceNode(properties=props, commands=cmds)
    right_conveyor = DeviceNode(properties=props, commands=cmds)

    sum_speed = Float(
        displayedName="Sum of speeds",
        description="The sum of the speeds of both conveyors",
        warnHigh=3, alarmHigh=10)

    @Slot(allowedStates={State.STOPPED})
    def start(self):
        self.left_conveyor.start()
        self.right_conveyor.start()
        self.state = State.STARTED

    @Slot(allowedStates={State.STARTED})
    def stop(self):
        self.left_conveyor.stop()
        self.right_conveyor.stop()
        self.state = State.STOPPED

    def onInitialization(self):
        self.monitor_left()
        self.monitor_right()
        self.state = State.STOPPED

    def monitor_left(self, future=None):
        print("left")
        self.sum_speed = (self.left_conveyor.currentSpeed +
                          self.right_conveyor.currentSpeed)
        waitUntilNew(self.left_conveyor.currentSpeed,
                     wait=False).add_done_callback(self.monitor_left)

    def monitor_right(self, future=None):
        print("right")
        self.sum_speed = (self.left_conveyor.currentSpeed +
                          self.right_conveyor.currentSpeed)
        waitUntilNew(self.right_conveyor.currentSpeed,
                     wait=False).add_done_callback(self.monitor_right)
