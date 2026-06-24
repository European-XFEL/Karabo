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
import sys
import warnings
from pathlib import Path
from traceback import format_exception, print_exception

from pyqtgraph import setConfigOptions
from qtpy.QtCore import QLocale, Qt
from qtpy.QtGui import QColor, QFont, QFontDatabase, QPalette, QPixmap
from qtpy.QtWidgets import QApplication, QSplashScreen, QStyleFactory

from karabo.common.scenemodel.api import SCENE_FONT_FAMILY, SCENE_FONT_SIZE
from karabogui.background import create_background_timer
from karabogui.const import IS_MAC_SYSTEM
from karabogui.controllers.api import populate_controller_registry
from karabogui.fonts import FONT_FILENAMES, get_font_size_from_dpi
from karabogui.singletons.api import get_manager, get_panel_wrangler
from karabogui.util import get_application_icon, process_qt_events, send_info


def excepthook(exc_type, value, traceback):
    print_exception(exc_type, value, traceback)
    text = "".join(format_exception(exc_type, value, traceback))
    try:
        send_info(type="error", traceback=text)
    except Exception:
        print("Could not send exception to network")


def set_app_info(app, company, url, name):
    """Set the information on the app for QSettings"""
    app.setOrganizationName(company)
    app.setOrganizationDomain(url)
    app.setApplicationName(name)


def _create_light_palette():
    """Create a light application palette independent of the system theme."""
    palette = QPalette()

    window = QColor(248, 248, 248)
    base = QColor(Qt.white)
    alternate_base = QColor(242, 242, 242)
    text = QColor(Qt.black)
    accent = QColor(0, 120, 215)

    for role, color in (
        (QPalette.Window, window),
        (QPalette.Base, base),
        (QPalette.AlternateBase, alternate_base),
        (QPalette.Button, window),
        (QPalette.ToolTipBase, base),
        (QPalette.WindowText, text),
        (QPalette.Text, text),
        (QPalette.ButtonText, text),
        (QPalette.ToolTipText, text),
        (QPalette.Highlight, accent),
        (QPalette.HighlightedText, QColor(Qt.white)),
        (QPalette.Link, QColor(0, 102, 204)),
        (QPalette.LinkVisited, QColor(102, 0, 153)),
    ):
        palette.setColor(role, color)

    disabled_text = QColor(127, 127, 127)
    for role in (QPalette.WindowText, QPalette.Text, QPalette.ButtonText):
        palette.setColor(QPalette.Disabled, role, disabled_text)

    return palette


def create_gui_app(args):
    """Create the QApplication with all necessary fonts and settings"""
    app = QApplication.instance()
    if app is None:
        app = QApplication(args)
    # Set directly the QSettings environment to have access
    set_app_info(app, "XFEL", "xfel.eu", "KaraboGUI")
    create_background_timer()
    style = QStyleFactory.create("Fusion")
    palette = style.standardPalette()
    if IS_MAC_SYSTEM:
        # MacOS do not use the pallet for drawing, it uses system native theme.
        # To avoid showing GUI in dark theme , if the system is dark, we use a
        # custom light palette.
        palette = _create_light_palette()
    app.setStyle(style)
    app.setPalette(palette)

    # Add fonts
    for font_file in FONT_FILENAMES:
        QFontDatabase.addApplicationFont(font_file)

    # Set default application font
    font_size = get_font_size_from_dpi(SCENE_FONT_SIZE)
    font = QFont()
    font.setFamily(SCENE_FONT_FAMILY)
    font.setPointSize(font_size)
    try:
        families = QFontDatabase.families()
    except TypeError:
        # for Qt5 compatibility
        families = QFontDatabase().families()
    if "Ubuntu" in families:
        font.insertSubstitution("Ubuntu", SCENE_FONT_FAMILY)
    if "Sans Serif" in families:
        font.insertSubstitution("Sans Serif", SCENE_FONT_FAMILY)
    app.setFont(font)
    app.setStyleSheet("QPushButton { text-align: left; padding: 5px; }")
    app.setStyleSheet("QToolBar { border: 0px }")
    # Also set the font of the QTreeView and QTableView as it defaults to
    # MS Shell Dlg in Windows for elided text when the QApplication stylesheet
    # has been changed. Any unknown font will do to default to QApp font.
    # [QTBUG-29232]
    app.setStyleSheet(
        f"QTreeView {{ font: {font_size}pt '{SCENE_FONT_FAMILY}'; }} "
        f"QTableView {{ font: {font_size}pt '{SCENE_FONT_FAMILY}'; }}")
    app.setAttribute(Qt.AA_DontShowIconsInMenus, False)

    # set a nice app logo
    icon = get_application_icon()
    app.setWindowIcon(icon)

    QLocale.setDefault(QLocale(QLocale.English, QLocale.UnitedStates))

    # Make sure our applications do not terminate on unhandled python
    # exceptions.
    sys.excepthook = excepthook

    return app


def init_gui(app, use_splash=True):
    """Initialize the GUI.

    Imports are being done inside this function to avoid delaying the display
    of the splash screen. We want the user to know that something is happening
    soon after they launch the GUI.
    """
    # Do some event processing!
    process_qt_events(app)

    if use_splash:
        splash_path = str(Path(__file__).parent / '..' / "icons"
                          / "splash.png")
        splash_img = QPixmap(splash_path)
        splash = QSplashScreen(splash_img, Qt.WindowStaysOnTopHint)
        splash.setMask(splash_img.mask())
        splash.show()
        # NOTE: This is needed to make the splash screen show up...
        splash.showMessage(" ")

    # Do some event processing!
    process_qt_events(app)

    # Start some heavy importing!
    import numpy

    from karabogui import icons

    numpy.set_printoptions(suppress=True, threshold=10)
    setConfigOptions(background=None, foreground="k")

    # Suppress some warnings
    # 1. From scipy.ndimage.zoom
    warnings.filterwarnings('ignore', '.*output shape of zoom.*')
    # 2. From scipy.optimize
    warnings.filterwarnings('ignore', '.*Covariance of the parameters '
                                      'could not be estimated.*')

    # Load the icons
    icons.init()
    # Load the sceneview widget controllers
    populate_controller_registry()
    # Initialize the Manager singleton
    get_manager()
    # Initialize the PanelWrangler and attach the splash screen
    panel_wranger = get_panel_wrangler()
    process_qt_events(app)
    if use_splash:
        panel_wranger.use_splash_screen(splash)
        process_qt_events(app)
