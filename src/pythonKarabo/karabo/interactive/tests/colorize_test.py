from unittest import main, TestCase

from ..startkarabo import colorize, COLORS


test_strings = {
    "up": "karabo_alarmServer:  up (pid 15759) 664625 seconds, normally down, running",  # noqa
    "stopping": "karabo_alarmServer:  up (pid 15759) 664625 seconds, normally down, stopping",  # noqa
    "down": "karabo_projectDBServer:  down 2483 seconds, stopped",
    "zero": "middlelayerserver_sqs_powersupplies:  up (pid 17662) 0 seconds, normally down, running",  # noqa
    "one": "middlelayerserver_sqs_powersupplies:  up (pid 17662) 1 seconds, normally down, running",  # noqa
    "failed": "middlelayerserver_sqs_powersupplies:  down 12 seconds, failed",  # noqa
}


def get_color(line):
    for name, color in COLORS.items():
        if line.startswith(color):
            return name


class TestColorize(TestCase):

    def test_color(self):
        test_conditions = {
            "up": "GREEN",
            "stopping": "YELLOW",
            "zero": "YELLOW",
            "one": "YELLOW",
            "down": "RED",
            "failed": "RED",
        }

        for server_state, color in test_conditions.items():
            line = colorize(test_strings[server_state])
            assert color == get_color(line)

    def test_garbage(self):
        assert "garbage" == colorize("garbage")


if __name__ == "__main__":
    main()
