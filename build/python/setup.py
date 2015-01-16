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
    packages=["karabo", "karaboGui", "karaboGui.dialogs",
              "karaboGui.icons", "karaboGui.treewidgetitems",
              "karaboGui.displaywidgets", "karaboGui.editablewidgets",
              "karaboGui.panels", "karaboGui.vacuumwidgets", "karaboGui.sceneitems"],
    package_dir=dict(karabo="../../src/pythonKarabo/karabo_package",
                     karaboGui="../../src/pythonGui"),
    scripts=['scripts/win_post_install.py'],
    package_data = {"karaboGui.icons": ["*.*"],
                    "karaboGui.displaywidgets": ["*.ui"],
                    "karaboGui.dialogs": ["*.ui"]},
)


