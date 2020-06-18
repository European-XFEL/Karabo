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
        'author_email': 'karabo@xfel.eu',
        'description': 'This is the Karabo GUI',
        'url': 'http://karabo.eu',
        'packages': find_packages(),
        'package_data': {
            "karabogui.binding.tests": ["data/*.config", "data/*.schema"],
            "karabogui.configurator.dialog": ["*.ui"],
            "karabogui.controllers": ["*.ui"],
            "karabogui.controllers.display": ["*.svg", "*.ui"],
            "karabogui.graph.plots.dialogs": ["*.ui"],
            "karabogui.graph.common.dialogs": ["*.ui"],
            "karabogui.graph.image.dialogs": ["*.ui"],
            "karabogui.dialogs": ["*.ui", 'tests/*.html'],
            "karabogui.icons": ["*.*", "statefulicons/iconset/*.svg"],
            "karabogui.panels": ["*.ui"],
            "karabogui.project.dialog": ["*.ui"],
        }
    }

    setup(
        entry_points={
            'console_scripts': [
                'karabo-gui=karabogui.programs.gui_runner:main',
                'karabo-alarms=karabogui.programs.alarm_runner:main',
                'panel-runner=karabogui.programs.panel_runner:main',
                'karabo-cinema=karabogui.programs.cinema:main',
                'karabo-theatre=karabogui.programs.theatre:main',
                'karabo-update-extensions=karabogui.dialogs.update_dialog:main'
            ]
        },
        # Add an alias for 'build' so we can prepare data for Windows
        **metadata
    )
