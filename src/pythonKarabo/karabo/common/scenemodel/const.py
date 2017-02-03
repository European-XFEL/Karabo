import enum
from xml.etree.ElementTree import register_namespace

# The version number for files which are written
SCENE_FILE_VERSION = 2

# Scene default dimensions
SCENE_MIN_WIDTH = 1024
SCENE_MIN_HEIGHT = 768


@enum.unique
class SceneTargetWindow(enum.Enum):
    MainWindow = 'mainwin'  # The default; a tab in the main window
    Dialog = 'dialog'  # An undocked window


# Define some XML namespaces that we might encounter
NS_INKSCAPE = "{http://www.inkscape.org/namespaces/inkscape}"
NS_KARABO = "{http://karabo.eu/scene}"
NS_SVG = "{http://www.w3.org/2000/svg}"
NS_XLINK = "{http://www.w3.org/1999/xlink}"

# For convenience (and initialization of ElementTree), put them into a dict too
XML_NAMESPACES = {
    'inkscape': NS_INKSCAPE,
    'krb': NS_KARABO,
    'svg': NS_SVG,
    'xlink': NS_XLINK,
}

for prefix, ns in XML_NAMESPACES.items():
    register_namespace(prefix, ns.strip('{}'))
