# ==== Boilerplate: To be hidden in the future ======
from asyncio import coroutine, get_event_loop

from karabo.middlelayer import String
from karabo.usermacros import AScan, UserMacro


class UserTest(UserMacro):
    name = String(defaultValue="UserTest")

    @coroutine
    def execute(self):
# ========== To be hidden in the future =============
        print("Running {}".format(self.name))
        loop = get_event_loop()
        ascan = yield from loop.run_coroutine_or_thread(
            AScan, "motor1", [1, 10], "cam1", 0.1, True, 5)
        yield from ascan()
