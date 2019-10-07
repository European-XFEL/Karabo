import os.path as op
from traceback import print_exception, format_exception
import warnings

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (
    QApplication, QFont, QIcon, QMessageBox, QStyleFactory)
from pyqtgraph import setConfigOptions

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
    app.setStyle(QStyleFactory.create("Cleanlooks"))
    app.setPalette(QApplication.style().standardPalette())

    font = QFont()
    font.setFamily("Sans Serif")
    font.setPointSize(10)
    app.setFont(font)

    app.setStyleSheet("QPushButton { text-align: left; padding: 5px; }")
    app.setStyleSheet("QToolBar { border: 0px }")
    app.setAttribute(Qt.AA_DontShowIconsInMenus, False)

    # set a nice app logo
    logo_path = op.join(op.dirname(__file__), '..', "icons", "app_logo.png")
    app.setWindowIcon(QIcon(logo_path))

    # These should be set to simplify QSettings usage
    app.setOrganizationName('XFEL')
    app.setOrganizationDomain('xfel.eu')
    app.setApplicationName('KaraboGUI')

    return app


def init_gui(app, splash=None):
    """Initialize the GUI.

    Imports are being done inside this function to avoid delaying the display
    of the splash screen. We want the user to know that something is happening
    soon after they launch the GUI.
    """
    from karabogui import icons
    import numpy

    numpy.set_printoptions(suppress=True, threshold=10)
    setConfigOptions(background=None, foreground="k")

    # Suppress some warnings
    # 1. From scipy.ndimage.zoom
    warnings.filterwarnings('ignore', '.*output shape of zoom.*')

    # Load the icons
    icons.init()
    # Load the sceneview widget controllers
    populate_controller_registry()

    # Initialize the Manager singleton
    get_manager()
    # Initialize the PanelWrangler and attach the splash screen
    panel_wranger = get_panel_wrangler()
    if splash is not None:
        panel_wranger.use_splash_screen(splash)
