import os.path as op
from traceback import print_exception, format_exception
import warnings

from PyQt5.QtCore import QLocale, Qt
from PyQt5.QtGui import QFont, QIcon, QPixmap
from PyQt5.QtWidgets import (
    QApplication, QMessageBox, QSplashScreen, QStyleFactory)
from pyqtgraph import setConfigOptions

from karabo.common.scenemodel.api import SCENE_FONT_SIZE

from karabogui.controllers.api import populate_controller_registry
from karabogui.singletons.api import (
    get_manager, get_network, get_panel_wrangler)


def excepthook(exc_type, value, traceback):
    print_exception(exc_type, value, traceback)
    icon = getattr(value, "icon", QMessageBox.Critical)
    title = getattr(value, "title", exc_type.__name__)
    message = getattr(value, "message",
                      "{}: {}".format(exc_type.__name__, value))
    mb = QMessageBox(icon, title, "{}\n{}\n\n".format(message, " " * 300))
    text = "".join(format_exception(exc_type, value, traceback))
    mb.setDetailedText(text)
    # NOTE: Uncomment to show exceptions in a message box
    # mb.exec_()
    try:
        network = get_network()
        network.onError(text)
    except Exception:
        print("could not send exception to network")


def create_gui_app(args):
    """Create the QApplication with all necessary fonts and settings"""
    app = QApplication(args)
    # Set the style among all operating systems
    style = QStyleFactory.create("Fusion")
    app.setStyle(style)
    app.setPalette(QApplication.style().standardPalette())

    font = QFont()
    font.setFamily("Sans Serif")
    font.setPointSize(SCENE_FONT_SIZE)
    app.setFont(font)

    app.setStyleSheet("QPushButton { text-align: left; padding: 5px; }")
    app.setStyleSheet("QToolBar { border: 0px }")
    # Also set the font of the QTreeView and QTableView as it defaults to
    # MS Shell Dlg in Windows for elided text when the QApplication stylesheet
    # has been changed. Any unknown font will do to default to QApp font.
    # [QTBUG-29232]
    app.setStyleSheet(
        "QTreeView { font: 10pt 'Unknown Font'; } "
        "QTableView { font: 10pt 'Unknown Font'; }")
    app.setAttribute(Qt.AA_DontShowIconsInMenus, False)

    # set a nice app logo
    logo_path = op.join(op.dirname(__file__), '..', "icons", "app_logo.png")
    app.setWindowIcon(QIcon(logo_path))

    # These should be set to simplify QSettings usage
    app.setOrganizationName('XFEL')
    app.setOrganizationDomain('xfel.eu')
    app.setApplicationName('KaraboGUI')

    QLocale.setDefault(QLocale(QLocale.English, QLocale.UnitedStates))

    return app


def init_gui(app, use_splash=True):
    """Initialize the GUI.

    Imports are being done inside this function to avoid delaying the display
    of the splash screen. We want the user to know that something is happening
    soon after they launch the GUI.
    """
    # Do some event processing!
    app.processEvents()

    if use_splash:
        splash_path = op.join(op.dirname(__file__), '..', "icons",
                              "splash.png")
        splash_img = QPixmap(splash_path)
        splash = QSplashScreen(splash_img, Qt.WindowStaysOnTopHint)
        splash.setMask(splash_img.mask())
        splash.show()
        # NOTE: This is needed to make the splash screen show up...
        splash.showMessage(" ")

    # Do some event processing!
    app.processEvents()

    # Start some heavy importing!
    from karabogui import icons
    import numpy

    numpy.set_printoptions(suppress=True, threshold=10)
    setConfigOptions(background=None, foreground="k")

    # Suppress some warnings
    # 1. From scipy.ndimage.zoom
    warnings.filterwarnings('ignore', '.*output shape of zoom.*')
    # 2. From scipy.optimize
    warnings.filterwarnings('ignore', '.*Covariance of the parameters '
                                      'could not be estimated.*')

    # 3. Trendline
    # We might deal with NaN values only, hence we disable all the warnings
    # as Qwt will handle nicely!
    IGNORE_NAN_WARN = [r'Mean of empty slice', r'All-NaN slice encountered',
                       r'All-NaN axis encountered']
    for warn in IGNORE_NAN_WARN:
        numpy.warnings.filterwarnings('ignore', warn)

    # Load the icons
    icons.init()
    # Load the sceneview widget controllers
    populate_controller_registry()

    # Initialize the Manager singleton
    get_manager()
    # Initialize the PanelWrangler and attach the splash screen
    panel_wranger = get_panel_wrangler()
    app.processEvents()
    if use_splash:
        panel_wranger.use_splash_screen(splash)
        app.processEvents()
