from distutils.core import setup

import os

# Work around mbcs bug in distutils.
# http://bugs.python.org/issue10945
import codecs
try:
    codecs.lookup('mbcs')
except LookupError:
    ascii = codecs.lookup('ascii')
    func = lambda name, enc=ascii: {True: enc}.get(name=='mbcs')
    codecs.register(func)


def getVersion():
    try:
        with open("VERSION", 'r') as file:
            return file.readline().rstrip()
    except IOError:
        return ""


setup(name="KaraboGUI",
    version=getVersion(),
    description="The GUI of the Karabo framework",
    author="Kerstin Weger, Burkhard Heisen, Martin Teichmann",
    packages=["karabo", "karabo.api2", "karabo.api2._project",
              "karabo.interactive", "karabo.packaging", "karabo.testing",
              "karabo_gui",
              "karabo_gui.attributeediting",
              "karabo_gui.dialogs",
              "karabo_gui.displaywidgets",
              "karabo_gui.editablewidgets",
              "karabo_gui.icons",
              "karabo_gui.panels",
              "karabo_gui.sceneitems",
              "karabo_gui.treewidgetitems",
              "karabo_gui.vacuumwidgets"],
    package_dir=dict(karabo="../../src/pythonKarabo/karabo",
                     karabo_gui="../../src/pythonGui/karabo_gui"),
    scripts=['scripts/win_post_install.py'],
    package_data = {"karabo_gui.icons": ["*.*"],
                    "karabo_gui.displaywidgets": ["*.ui"],
                    "karabo_gui.dialogs": ["*.ui"]},
)


