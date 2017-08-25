# ==== Boilerplate: To be hidden in the future ======
from asyncio import coroutine

from karabo.middlelayer import String
from karabo.usermacros import AMove, UserMacro


class UserMacroExample(UserMacro):
    name = String(defaultValue="UserMacroExample")
# ========== To be hidden in the future =============

    @coroutine
    def execute(self):
        print("Running {}".format(self.name))
        AMove("motor1", 0)()
