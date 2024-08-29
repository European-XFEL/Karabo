# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import re
from pathlib import Path

import yaml
from qtpy.QtCore import QProcess
from qtpy.QtWidgets import QApplication

from karabogui.singletons.api import get_config, get_network
from karabogui.util import process_qt_events

LINUX_DESKTOP_FILE_TEMPLATE = """\
[Desktop Entry]
Name={name}
Exec={command}
Icon={icon}
Terminal=false
Type=Application
MimeType=x-scheme-handler/{scheme};
"""


def create_linux_desktop_file(name, command, icon, scheme):
    # Resolve path
    path = Path.home() / ".local" / "share" / "applications"
    path.mkdir(parents=True, exist_ok=True)

    path /= get_valid_filename(name).lower() + ".desktop"
    # Write desktop file
    with path.open("w") as f:
        contents = LINUX_DESKTOP_FILE_TEMPLATE.format(
            name=name, command=command, icon=icon, scheme=scheme)
        f.write(contents)

    return path


def get_valid_filename(s):
    """
    Return the given string converted to a string that can be used for a clean
    filename. Remove leading and trailing spaces; convert other spaces to
    underscores; and remove anything that is not an alphanumeric, dash,
    underscore, or dot.
    >>> get_valid_filename("john's portrait in 2004.jpg")
    'johns_portrait_in_2004.jpg'

    From Django:
    https://github.com/django/django/blob/master/django/utils/text.py
    """
    s = str(s).strip().replace(' ', '_')
    return re.sub(r'(?u)[^-\w.]', '', s)


def close_app():
    app = QApplication.instance()
    if app is not None:
        process_qt_events(app, timeout=1000)
        app.quit()


def save_concert_file(file_name, scene_data):
    """ Write the scene information to a yaml file, for karabo-concert.
    """
    domain = get_config()["domain"]
    network = get_network()
    hostname = network.hostname
    port = network.port

    scenes = []
    for uuid, properties in scene_data.items():
        x = properties["x"]
        y = properties["y"]
        scenes.append({"uuid": uuid, "x": x, "y": y})

    concert = {"domain": domain, "host": hostname,
               "port": port, "scenes": scenes}
    with open(file_name, "w") as yaml_file:
        yaml.dump(concert, yaml_file)


def run_concert(file_name):
    """Start karabo-concert as a separate process"""
    QProcess.startDetached(f"karabo-concert {file_name}")
