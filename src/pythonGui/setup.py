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

from setuptools import find_packages, setup

CURRENT_FOLDER = os.path.dirname(os.path.realpath(__file__))
VERSION_FILE_PATH = os.path.join(CURRENT_FOLDER, 'karabogui', '_version.py')
ROOT_FOLDER = os.path.dirname(os.path.dirname(CURRENT_FOLDER))

if __name__ == '__main__':

    metadata = {
        'name': 'KaraboGUI',
        'use_scm_version': {
            'root': ROOT_FOLDER, 'write_to': VERSION_FILE_PATH
        },
        'author': 'Karabo Team',
        'author_email': 'opensource@xfel.eu',
        'description': 'This is the Karabo GUI',
        'url': 'http://karabo.eu',
        'license': "GPLv3+",
        'packages': find_packages(),
        'package_data': {
            "karabogui.tests": ["data/*.npy", "data/*.npz"],
            "karabogui.binding.tests": ["data/*.config", "data/*.schema"],
            "karabogui.configurator.dialog": ["*.ui"],
            "karabogui.controllers": ["*.ui", "display/ui/*.ui"],
            "karabogui.controllers.display": ["*.svg", "*.ui"],
            "karabogui.graph.plots.dialogs": ["*.ui"],
            "karabogui.graph.common.dialogs": ["*.ui"],
            "karabogui.graph.image.dialogs": ["*.ui"],
            "karabogui.dialogs": ["ui/*.ui", "tests/*.json"],
            "karabogui.fonts": ["*.*", "*/*"],
            "karabogui.icons": ["*.*", "statefulicons/iconset/*.svg"],
            "karabogui.panels": ["ui/*.ui"],
            "karabogui.programs.tests": ["concert.yml"],
            "karabogui.project.dialog": ["ui/*.ui"],
            "karabogui.sceneview": ["tests/data/*.svg"],
            "karabogui.widgets": ["ui/*.ui"],
            "karabogui.wizards": ["cinema/*.ui", "tips/*.html"],
        }
    }

    setup(
        entry_points={
            'console_scripts': [
                'karabo-gui=karabogui.programs.gui_runner:main',
                'panel-runner=karabogui.programs.panel_runner:main',
                'karabo-cinema=karabogui.programs.cinema:main',
                'karabo-concert=karabogui.programs.concert:main',
                'karabo-theatre=karabogui.programs.theatre:main',
                'karabo-update-extensions=karabogui.dialogs.update_dialog:main', # noqa
                'karabo-register-protocol=karabogui.programs.register_protocol:register_protocol', # noqa
            ]
        },
        # Add an alias for 'build' so we can prepare data for Windows
        **metadata
    )
