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
import sys
import warnings
from pathlib import Path
from platform import system
from traceback import format_exception, print_exception

from pyqtgraph import setConfigOptions
from qtpy.QtCore import QLocale, Qt
from qtpy.QtGui import QFont, QFontDatabase, QIcon, QPixmap
from qtpy.QtWidgets import QApplication, QSplashScreen, QStyleFactory

from karabo.common.scenemodel.api import (
    SCENE_DEFAULT_DPI, SCENE_FONT_FAMILY, SCENE_FONT_SIZE)
from karabogui.background import create_background_timer
from karabogui.controllers.api import populate_controller_registry
from karabogui.fonts import FONT_FILENAMES, get_font_size_from_dpi
from karabogui.singletons.api import (
    get_config, get_manager, get_panel_wrangler)
from karabogui.util import process_qt_events, send_info


def excepthook(exc_type, value, traceback):
    print_exception(exc_type, value, traceback)
    text = "".join(format_exception(exc_type, value, traceback))
    try:
        send_info(type="error", traceback=text)
    except Exception:
        print("Could not send exception to network")


def create_gui_app(args):
    """Create the QApplication with all necessary fonts and settings"""
    if system() == 'Darwin' and 'QT_MAC_WANTS_LAYER' not in os.environ:
        os.environ['QT_MAC_WANTS_LAYER'] = '1'
        # PyQt 5.12 onwards provides problems for MacOS11.
        # https://github.com/conda-forge/pyqt-feedstock/issues/98
    app = QApplication.instance()
    if app is None:
        app = QApplication(args)
    # Set directly the QSettings environment to have access
    app.setOrganizationName('XFEL')
    app.setOrganizationDomain('xfel.eu')
    app.setApplicationName('KaraboGUI')
    # We check our `KARABO_TEST_GUI` variable before due to Squish cracks!
    if get_config()['highDPI'] and not os.environ.get("KARABO_TEST_GUI"):
        # Create a preliminary QApplication to check system/screen properties.
        # This is needed as setting QApplication attributes should be done
        # before the instantiation (Qt bug as of 5.9).
        # Note: Must take the int of dpi! This is fixed in Qt 5.15 and always
        # active in Qt 6!
        dpi = int(app.primaryScreen().logicalDotsPerInch())
        app.quit()
        del app
        # Set the QApplication attributes before its instantiation.
        # Only apply high DPI scaling when the logical DPI is greater than
        # the default DPI (96). This is usually observed on scaled desktops
        # (e.g., 150% scaling on Windows)
        if dpi > SCENE_DEFAULT_DPI:
            QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
            QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)

        app = QApplication(args)
        # Again set the QSettings environment
        app.setOrganizationName('XFEL')
        app.setOrganizationDomain('xfel.eu')
        app.setApplicationName('KaraboGUI')

    create_background_timer()
    style = QStyleFactory.create("Fusion")
    palette = style.standardPalette()
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
    logo_path = str(Path(__file__).parent / '..' / "icons" / "app_logo.png")
    app.setWindowIcon(QIcon(logo_path))

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
