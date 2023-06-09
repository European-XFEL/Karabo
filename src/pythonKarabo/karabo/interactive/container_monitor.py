# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import signal
import subprocess
from argparse import ArgumentParser
from time import sleep


class TermCatcher:
    def __init__(self):
        signal.signal(signal.SIGINT, self.exit)
        signal.signal(signal.SIGTERM, self.exit)
        self.alive = True

    def exit(self, *args):
        self.alive = False

    def __bool__(self):
        return self.alive


def main():
    """karabo-check-container

    karabo-check-container container_name start_command stop_command
    """

    parser = ArgumentParser()
    parser.add_argument('container_name')
    parser.add_argument('start_command')
    parser.add_argument('stop_command')
    parser.add_argument('--nap_time',
                        help='time to sleep every each check',
                        default=1)
    args = parser.parse_args()
    container_name = args.container_name
    nap_time = args.nap_time
    b_container_name = container_name.encode()
    print(f'Monitoring the container {container_name} every {nap_time}')
    catcher = TermCatcher()
    while catcher:
        cmd = f"docker ps --filter name={container_name}"
        output = subprocess.check_output(
            cmd, shell=True, stderr=subprocess.STDOUT)
        lines = [line
                 for line in output.split(b'\n')
                 if b_container_name in line]
        # if the container name is found, just sleep a bit
        if lines:
            sleep(nap_time)
        else:
            print(f"starting container {container_name}")
            output = subprocess.check_output(
                args.start_command,
                shell=True, stderr=subprocess.STDOUT)
            print(output.decode())
    else:
        print(f"stopping container {container_name} as requested")
        output = subprocess.check_output(
            args.stop_command,
            shell=True, stderr=subprocess.STDOUT)


if __name__ == "__main__":
    main()
