from karabo.middlelayer import Device, Slot, String


class __CLASS_NAME__(Device):
    greeting = String()

    @Slot()
    def hello(self):
        self.greeting = "Hello world!"
