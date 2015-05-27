from distutils.core import setup

import os


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
    package_dir=dict(karabo="pythonKarabo/karabo",
                     karaboGui="pythonGui"),
    scripts=['scripts/karaboGui.py'],
    package_data = {"karaboGui.icons": ["*.*"],
                    "karaboGui.displaywidgets": ["*.ui"],
                    "karaboGui.dialogs": ["*.ui"]},
)



