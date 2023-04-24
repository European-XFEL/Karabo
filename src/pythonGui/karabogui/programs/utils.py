# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import re
from pathlib import Path

from qtpy.QtWidgets import QApplication

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
