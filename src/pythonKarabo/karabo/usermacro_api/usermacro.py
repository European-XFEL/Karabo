"""User Macros implement as-simple-as-possible
   domain-level routines for Light-Source operations
"""
from asyncio import get_event_loop

from karabo.middlelayer import Macro
from .pipeline import OutputChannel


class UserMacro(Macro):
    """"Base class for user macros"""
    outputChannel = OutputChannel()

    def __call__(self):
        self.execute()
        yield from self.slotKillDevice()

    def execute(self):
        """Insert here the code of a UserMacro"""

    def startInstance(self, server=None, *, loop=None):
        """Insert here the code of a UserMacro"""
        self.execute()
        yield from self.slotKillDevice()


def display(image):
    """Display an image into karabo GUI"""
    instance = get_event_loop().instance()
    instance.outputChannel = image
