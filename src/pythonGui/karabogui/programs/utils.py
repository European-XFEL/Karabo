from pathlib import Path
import re

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
    path.parent.mkdir(parents=True, exist_ok=True)
    path /= get_valid_filename(name).lower() + ".desktop"

    # Write desktop file
    with open(path, "w") as file:
        contents = LINUX_DESKTOP_FILE_TEMPLATE.format(
            name=name, command=command, icon=icon, scheme=scheme)
        file.write(contents)

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
