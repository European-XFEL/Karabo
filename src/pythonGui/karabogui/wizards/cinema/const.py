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
from pathlib import Path

import karabogui.icons as icons
from karabogui.const import HIDDEN_KARABO_FOLDER

LOGO_PATH = str(Path(icons.__file__).parent / "splash.png")
LOGO_WIDTH = 50

ICONS_FOLDER = Path(HIDDEN_KARABO_FOLDER) / "icons"
DEFAULT_ICON_PATH = ICONS_FOLDER / Path(LOGO_PATH).name
