import json
import os
import os.path as op
from collections import defaultdict
from functools import partial

from .utils import command_run, conda_run_command

CHANNEL_MAP = {}
EXCLUDED_CHANNELS = [
    "karabo/channel",
    "karabo/channel/mirror/conda-forge",
    "pypi",
    "",
]


class _MirrorChannel:
    """Stores all the information a mirror should have, such as its name,
    original repository, and target mirror name"""

    def __init__(self, name, mirror_root):
        self.name = name
        self.norm_name = name.replace("/", "-")
        self.original_repo = CHANNEL_MAP.get(name, name)
        # not a path join. this is a URL
        self.mirror_channel = "/".join([mirror_root, "mirror", self.norm_name])
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
            packages_on_mirror = command_run(["conda", "search",
                                              "--override-channels", "-c",
                                              self.mirror_channel,
                                              f"*[subdir={platform}]",
                                              "--json", ]
                                             )
            # All these packages are already on the target mirror,
            # so it is not necessary to upload them
            to_exclude = defaultdict(list)
            print(f"Optimize upload for {self.norm_name}-{platform}")
            for name, packages in json.loads(packages_on_mirror).items():
                for package in packages:
                    to_exclude[package["name"]].append(
                        (package["version"], package["build"])
                    )
            return to_exclude
        except json.decoder.JSONDecodeError:
            print("Json decode error, continuing without packages on "
                  f"{self.mirror_channel} for {platform}")
            return {}
        except Exception as e:
            print(f"Failed to find packages in  {self.mirror_channel} for "
                  f"{platform}.\n{e}")
            return {}


class Mirrors:
    """Holds the information of all mirrros and the packages we will need to
    download for each mirror and platform"""

    def __init__(self, reference_environment, mirror_channel):
        self.mirror_channel = mirror_channel
        self._mirrors = {}
        self._needed_packages = defaultdict(partial(defaultdict, list))
        self.reference_environment = reference_environment

    def _generate_needed_packages(self):
        """Returns the packages we will need to download. This excludes
        packages already present in our mirror channels"""
        output = command_run(
            ["conda", "list", "-n", self.reference_environment, "--json"])
        for pkg in json.loads(output):
            if pkg["base_url"].startswith("file://"):
                continue

            # skip local files
            channel_name = pkg["channel"]
            if channel_name in EXCLUDED_CHANNELS:
                continue

            channel = self._mirrors.setdefault(
                channel_name,
                _MirrorChannel(channel_name, mirror_root=self.mirror_channel),
            )

            name = pkg["name"]
            version = pkg["version"]
            build = pkg["build_string"]
            platform = pkg["platform"]

            to_exclude = channel.get_packages(platform)

            if name in to_exclude and (version, build) in to_exclude[name]:
                # this package is already uploaded
                print(f"Skipping upload of {name} to {channel_name}")
                continue

            self._needed_packages[channel_name][platform].append(
                (name, version, build)
            )

    def populate(self, target_dir):
        """Populate the mirror in the given target directory"""
        self._generate_needed_packages()
        for channel_name, platforms in self._needed_packages.items():
            mirror = self._mirrors[channel_name]
            # Create a configuration file for each platform's packages
            print("needing", channel_name, platforms)
            for pkg_platform, needed_packages in platforms.items():
                to_write = "blacklist:\n" '    - name: "*"\n' "whitelist:\n"

                # Whitelist all packages that are not yet mirrored
                for name, version, build in needed_packages:
                    to_write += (
                        f"    - name: {name}\n"
                        f"      version: '{version}'\n"
                        f"      build: '{build}'\n"
                    )
                conf_filename = f"{mirror.norm_name}.yaml"
                conf_file = op.join(target_dir, conf_filename)
                with open(conf_file, "w") as f:
                    f.write(to_write)

                print(
                    f"""
Creating mirror {mirror.name} - {pkg_platform} with the following configuration

{to_write}"""
                )
                self._populate_mirror(
                    mirror, pkg_platform, conf_file, target_dir
                )

    def _populate_mirror(self, mirror, platform, conf_file, target_dir):
        """Calls a conda-mirror process in order to download all the packages
        we need"""

        target_mirror_directory = op.join(target_dir, mirror.norm_name)

        # Create the mirror directory for later
        if not op.isdir(target_mirror_directory):
            os.makedirs(target_mirror_directory)

        print(f"Mirroring channel {mirror.name}...")
        conda_run_command([
            "conda", "mirror", "--upstream-channel", mirror.original_repo,
            "--target-directory", target_mirror_directory,
            "--platform", platform,
            "--config", str(conf_file)],
            env_name="base")
        os.remove(conf_file)
