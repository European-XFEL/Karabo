import argparse
import json
import os
import os.path as op
import pathlib
import shutil
import subprocess
import sys
from collections import defaultdict
from functools import partial

CHANNEL_MAP = {'pkgs/main': 'https://repo.anaconda.com/pkgs/main/'}
KARABO_CHANNEL = 'http://exflserv05.desy.de/karabo/channel/'


class _MirrorChannel:
    """Stores all the information a mirror should have, such as its name,
    original repository, and target mirror name"""

    def __init__(self, name):
        self.name = name
        self.conda_exe = os.environ.get('CONDA_EXE', 'conda')
        self.norm_name = name.replace('/', '-')
        self.original_repo = CHANNEL_MAP.get(name, name)
        self.mirror_channel = op.join(KARABO_CHANNEL, 'mirror', self.norm_name)

        # Dictionary containing packages for each platform of this mirror
        self._packages = {}

    def get_packages(self, platform):
        if platform not in self._packages:
            packages = self._obtain_packages_on_mirror(platform)
            self._packages[platform] = packages
        return self._packages[platform]

    def _obtain_packages_on_mirror(self, platform):
        """Returns the packages that are already on our mirror channel"""
        try:
            packages_on_mirror= subprocess.check_output(
                [self.conda_exe, 'search', '--override-channels', '-c',
                 self.mirror_channel, f'*[subdir={platform}]', '--json'])

            # All these packages are already on exflserv05, so we exclude them
            # from the mirroring
            to_exclude = defaultdict(list)
            for name, packages in json.loads(packages_on_mirror).items():
                for package in packages:
                    to_exclude[package['name']].append((package['version'],
                                                        package['build']))
            return to_exclude
        except subprocess.CalledProcessError:
            return {}


class Mirrors:
    """Holds the information of all mirrros and the packages we will need to
    download for each mirror and platform"""

    def __init__(self, reference_environment):
        self.conda_exe = os.environ.get('CONDA_EXE', 'conda')
        self._mirrors = {}
        self._needed_packages = defaultdict(partial(defaultdict, list))

        self._generate_needed_packages(reference_environment)

    def _generate_needed_packages(self, reference_environment):
        """Returns the packages we will need to download. This excludes packages
        already present in our mirror channels"""
        cmd = [self.conda_exe, 'list', '-n', reference_environment, '--json']
        output = subprocess.check_output(cmd)

        for pkg in json.loads(output):

            channel_name = pkg['channel']
            if channel_name in ['karabo/channel', 'pypi']:
                continue

            if channel_name in self._mirrors:
                channel = self._mirrors[channel_name]
            else:
                channel = _MirrorChannel(channel_name)
                self._mirrors[channel_name] = channel

            name = pkg['name']
            version = pkg['version']
            build = pkg['build_string']
            platform = pkg['platform']

            to_exclude = channel.get_packages(platform)

            if name in to_exclude and (version, build) in to_exclude[name]:
                continue

            self._needed_packages[channel_name][platform].append(
                (name, version, build))

    def populate(self, target_dir):
        """Populate the mirror in the given target directory"""
        for channel_name, platforms in self._needed_packages.items():
            mirror = self._mirrors[channel_name]

            # Create a configuration file for each platform's packages
            for platform, needed_packages in platforms.items():
                to_write = 'blacklist:\n' \
                           '    - name: "*"\n' \
                           'whitelist:\n'

                # We will whitelist all packages that are not yet downloaded
                for name, version, build in needed_packages:
                    to_write += f'    - name: {name}\n' \
                                f'      version: \'{version}\'\n' \
                                f'      build: \'{build}\'\n'

                conf_file = pathlib.Path(target_dir).joinpath(
                    f'{mirror.norm_name}.yaml')
                conf_file.write_text(to_write)

                self._populate_mirror(mirror, platform, conf_file, target_dir)

    def _populate_mirror(self, mirror, platform, conf_file, target_dir):
        """Calls a conda-mirror process in order to download all the packages
        we need"""
        print("Creating mirror {} - {} with the following configuration\n{}\n"
              .format(mirror.name, platform, conf_file.read_text()))
        target_mirror_directory = op.join(target_dir, mirror.norm_name)

        # Create the mirror directory for later
        if not os.path.isdir(target_mirror_directory):
            os.makedirs(target_mirror_directory)

        # Bug found on windows, can't mirror using threads due to weird
        # logging error
        num_threads = 1 if sys.platform.startswith('win') else 0

        print(f'Mirroring channel {mirror.name}...')
        cmd = [self.conda_exe, 'mirror',
               '--upstream-channel', mirror.original_repo,
               '--target-directory', target_mirror_directory,
               '--platform', platform,
               '--config', str(conf_file),
               '--num-threads', str(num_threads)]

        print(' '.join(cmd))
        subprocess.run(cmd, check=True)


def main(args):
    target_dir = args.target_dir

    if os.path.isdir(target_dir):
        shutil.rmtree(target_dir)
    os.makedirs(target_dir)

    mirrors = Mirrors(args.env)
    mirrors.populate(target_dir)


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description='Generate Mirror Channel')
    ap.add_argument(
        '--target_dir', type=str, required=True,
        help='Target directory on which the files will be downloaded')
    ap.add_argument(
        '--env', type=str, default='karabogui',
        help='Environment for reference'
    )

    main(ap.parse_args())
