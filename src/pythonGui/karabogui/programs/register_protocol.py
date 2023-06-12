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
import os
import shutil
import subprocess
import sys
from pathlib import Path
from platform import system

from qtpy.QtCore import QSettings

import karabogui.icons as icons
from karabogui.const import HIDDEN_KARABO_FOLDER
from karabogui.programs.utils import create_linux_desktop_file

ON_WINDOWS = system() == "Windows"
PYTHON_SHEBANG = f"#!{sys.executable}"

PROTOCOL_HANDLER = "protocol_handler.py"
PROTOCOL_HANDLER_SRC = str(Path(__file__).parent / PROTOCOL_HANDLER)
_dest = Path(HIDDEN_KARABO_FOLDER) / PROTOCOL_HANDLER
if ON_WINDOWS:
    _dest = _dest.with_suffix(".pyw")
PROTOCOL_HANDLER_DEST = str(_dest)

ICON_PATH = str(Path(icons.__file__).parent / "splash.png")


def write_protocol_handler(src, dest):
    # Copy the protocol handler. We use shebangs on Unix systems to easily
    # execute the application
    Path(dest).parent.mkdir(parents=True, exist_ok=True)
    if ON_WINDOWS:
        # Copy the file directly
        shutil.copy(src, dest)
    else:
        # Copy the contents of the file while prepending the shebang
        with open(dest, 'w') as dest_file:
            dest_file.write(PYTHON_SHEBANG + "\n")
            with open(src) as src_file:
                dest_file.writelines(src_file.readlines())
        # Make the file executable
        stat = os.stat(dest)
        os.chmod(dest, stat.st_mode | 0o111)


def register_protocol():
    handler_path = PROTOCOL_HANDLER_DEST
    write_protocol_handler(src=PROTOCOL_HANDLER_SRC,
                           dest=handler_path)
    if system() == "Windows":
        _register_protocol_windows(scheme="karabo", handler=handler_path)
    elif system() == "Linux":
        _register_protocol_linux(scheme="karabo", handler=handler_path)


def _register_protocol_windows(scheme, handler):
    settings = QSettings("HKEY_CURRENT_USER\\Software\\Classes",
                         QSettings.NativeFormat)
    settings.beginGroup(scheme)
    settings.setValue("Default", f"URL:{scheme} Protocol")
    settings.setValue("URL Protocol", "")

    cmd = f'"{sys.executable}" "{handler}" "%1"'
    settings.setValue("shell/open/command/Default", cmd)


def _register_protocol_linux(scheme, handler):
    # Create a desktop file to call the protocol handler
    path = create_linux_desktop_file(
        name="Karabo",
        icon=ICON_PATH,
        command=f"{handler} %u",
        scheme="karabo")
    # Register the scheme on both `gio` and `xdg`
    gio_cmd = ["gio", "mime", f"x-scheme-handler/{scheme}", path.name]
    xdg_cmd = ["xdg-mime", "default", path.name, f"x-scheme-handler/{scheme}"]

    for cmd in (gio_cmd, xdg_cmd):
        try:
            subprocess.call(cmd)
        except (subprocess.CalledProcessError, FileNotFoundError):
            # We are ignoring errors in case one of the two url management
            # is missing.
            RuntimeWarning(f"Scheme registration with {cmd[0]} has failed.")


if __name__ == "__main__":
    register_protocol()
