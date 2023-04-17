"""
This is a wrapper for the conda CLI install commands, tailored for
Karabo installations/updates on packed environment. This is needed because
`conda` is not included on the pack and not all machines has it installed.

In the future, we could the check packed environment list of packages and
versions and extract the environment to the current folder such that we
will be less dependent on the (mirror) conda-forge."""

import argparse

from conda.cli.python_api import Commands, run_command

KARABO_CHANNEL = "http://exflctrl01.desy.de/karabo/channel"


def main(args, conda_args):
    # Retrieve a list of channels
    channels = [KARABO_CHANNEL]
    if args.channel:
        channels = [c for channel in args.channel for c in channel]

    # Create a list of conda channel arguments (with `-c` for every channel)
    conda_channels = [arg for channel in channels for arg in ('-c', channel)]

    # Run conda install command
    run_command(Commands.INSTALL, *conda_channels, *args.packages, *conda_args)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('packages', type=str, nargs='+')
    parser.add_argument('-c', '--channel', nargs=1,
                        action='append', type=str)

    args, conda_args = parser.parse_known_args()
    main(args, conda_args)
