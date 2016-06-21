from numpy import arctan2, cos, sin, sqrt

from karabo.middlelayer import (AccessMode, background, Device, DeviceNode,
                                Float, Slot, Unit, wait_for_all, waitUntilNew)


class CircleMotor(Device):
    xaxis = DeviceNode(
        displayedName="X Axis",
        properties=["targetValue", "position", "hardwareState"],
        commands=[("ramp_ch2", "ramp"), "reset"])

    yaxis = DeviceNode(
        displayedName="Y Axis",
        properties=["targetValue", "position", "hardwareState"],
        commands=["reset"])

    targetRadius = Float(
        displayedName="Radius of the circle to move on",
        unitSymbol=Unit.METER)

    targetAngle = Float(
        displayedName="Angle on the circle to move to",
        unitSymbol=Unit.DEGREE)

    radius = Float(
        displayedName="Radius of the circle the motor is on",
        accessMode=AccessMode.READONLY,
        unitSymbol=Unit.METER)

    angle = Float(
        displayedName="Angle on the circle the motor is at",
        accessMode=AccessMode.READONLY,
        unitSymbol=Unit.DEGREE)

    def onInitialization(self):
        background(self.watch, self.xaxis)
        background(self.watch, self.yaxis)

    @Slot(
        displayedName="Move to target position")
    def move(self):
        self.xaxis.targetValue = self.targetRadius * sin(self.targetAngle)
        self.yaxis.targetValue = self.targetRadius * cos(self.targetAngle)
        xfuture = background(self.xaxis.move)
        yfuture = background(self.yaxis.move)
        wait_for_all(xfuture, yfuture)

    def watch(self, device):
        while True:
            waitUntilNew(device)
            self.angle = arctan2(self.xaxis.position, self.yaxis.position)
            self.radius = sqrt(self.xaxis.position ** 2 +
                               self.yaxis.position ** 2)
