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
              "karaboGui",
              "karaboGui.karabo_gui",
              "karaboGui.karabo_gui.attributeediting",
              "karaboGui.karabo_gui.dialogs",
              "karaboGui.karabo_gui.displaywidgets",
              "karaboGui.karabo_gui.editablewidgets",
              "karaboGui.karabo_gui.icons",
              "karaboGui.karabo_gui.panels",
              "karaboGui.karabo_gui.sceneitems",
              "karaboGui.karabo_gui.treewidgetitems",
              "karaboGui.karabo_gui.vacuumwidgets"],
    package_dir=dict(karabo="../../src/pythonKarabo/karabo",
                     karaboGui="../../src/pythonGui"),
    scripts=['scripts/win_post_install.py'],
    package_data = {"karaboGui.karabo_gui.icons": ["*.*"],
                    "karaboGui.karabo_gui.displaywidgets": ["*.ui"],
                    "karaboGui.karabo_gui.dialogs": ["*.ui"]},
)


