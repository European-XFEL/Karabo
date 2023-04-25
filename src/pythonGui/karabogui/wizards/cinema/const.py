# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pathlib import Path

import karabogui.icons as icons
from karabogui.const import HIDDEN_KARABO_FOLDER

LOGO_PATH = str(Path(icons.__file__).parent / "splash.png")
LOGO_WIDTH = 50

ICONS_FOLDER = Path(HIDDEN_KARABO_FOLDER) / "icons"
DEFAULT_ICON_PATH = ICONS_FOLDER / Path(LOGO_PATH).name
