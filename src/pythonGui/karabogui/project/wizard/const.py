from pathlib import Path

from karabogui.globals import HIDDEN_KARABO_FOLDER
import karabogui.icons as icons

LOGO_PATH = str(Path(icons.__file__).parent / "splash.png")
LOGO_WIDTH = 50

ICONS_FOLDER = Path(HIDDEN_KARABO_FOLDER) / "icons"
DEFAULT_ICON_PATH = ICONS_FOLDER / Path(LOGO_PATH).name