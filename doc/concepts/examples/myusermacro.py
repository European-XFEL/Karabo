# ==== Boilerplate: To be hidden in the future ======
from asyncio import coroutine

from karabo.middlelayer import background, String
from karabo.usermacros import AScan, UserMacro


class UserTest(UserMacro):
    name = String(defaultValue="UserTest")
# ========== To be hidden in the future =============

    @coroutine
    def execute(self):
        print("Running {}".format(self.name))

        # Run an AScan and get the data
        ascan = yield from background(
            AScan, "motor1@targetPos*", [1, 10], "cam1", 0.1, True, 5)
        data = yield from ascan()

        # Print the acquired train IDs
        print(data)
