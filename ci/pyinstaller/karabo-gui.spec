# -*- mode: python ; coding: utf-8 -*-
"""
This stores the specifications to  create bundle for karabo gui, karabo-cinema,
karabo-concert and karabo-theatre.

To create bundle,
 - Activate karabogui conda environment
 - Set 'KARABO_GUI_SRC' env variable to your `Framework/src/pythonGui/karabogui`
 - Set 'KARABO_GUI_EXTENSIONS_SRC' env variable to your `guiextensions/src/extensions`
 - Run 'pyinstaller karabo-gui.spec'
"""
import importlib
import os
import shutil
from pathlib import Path

from PyInstaller.utils.hooks import copy_metadata


def get_karabo_gui_dir():
    """
    Get the path to 'src/pythonGui/karabogui' directory from 'KARABO_GUI_SRC'
    environment variable
    """
    karabo_gui_src = os.environ.get('KARABO_GUI_SRC')
    if not karabo_gui_src:
        raise RuntimeError("Define path to 'karabogui' directory as "
                           "KARABO_GUI_SRC environment variable")
    karabo_gui_dir = Path(karabo_gui_src).resolve()
    if not karabo_gui_dir.is_dir():
        raise RuntimeError(
            f"{karabo_gui_dir} doesn't exist or not a valid directory")
    return karabo_gui_src


KARABO_GUI_DIR = Path(get_karabo_gui_dir())

KARABO_GUI_SCRIPT = Path(KARABO_GUI_DIR, 'programs/gui_runner.py')
KARABO_CINEMA_SCRIPT = Path(KARABO_GUI_DIR, 'programs/cinema.py')
KARABO_CONCERT_SCRIPT = Path(KARABO_GUI_DIR, 'programs/concert.py')
KARABO_THEATRE_SCRIPT = Path(KARABO_GUI_DIR, 'programs/theatre.py')

gui_extensions_path = os.environ.get('KARABO_GUI_EXTENSIONS_SRC')
KARABO_GUI_EXTENSIONS_DIR = (Path(gui_extensions_path).resolve() if
                           gui_extensions_path else None)
SOURCES = {"karabogui": KARABO_GUI_DIR,
           "extensions": KARABO_GUI_EXTENSIONS_DIR}


def get_ui_icon_data():
    """
    Find all the ui files and icons files from karabogui and store
    them as tuple of source file and destination directory
    for eg:
    'karabogui/widgets/ui/find_widget.ui' will be stored as
    ( "../src/pythonGui/karabogui/widgets/ui/find_widget.ui",
      "karabogui/widgets/ui/")
    """
    data = []
    file_extensions = (".ui", ".png", ".svg", ".gif")
    for name, source_dir in SOURCES.items():
        if source_dir is None:
            continue
        for file_name in source_dir.rglob("*.*"):
            file_extension = file_name.suffix
            if file_extension not in file_extensions:
                continue
            dest_dir = str(file_name.parent).replace(str(source_dir), "")
            dest = Path(f"{name}{dest_dir}")
            data.append((f"{file_name}", f"{dest}"))
    return data


def get_controllers_modules():
    """
    Get a list of all the python modules in 'display' and 'edit' directories.

    These are imported dynamically on runtime while opening the Karabo gui.
    See 'karabogui.controls.utils.populate_controller_registry' function.
    """
    controllers_modules = []
    ROOT_PACKAGE = 'karabogui.controllers'
    SUBPACKAGES = ('display', 'edit')
    for pkg in SUBPACKAGES:
        pkg = ROOT_PACKAGE + '.' + pkg
        module = importlib.import_module(pkg)
        parent = Path(module.__file__).parent
        mod_files = [pkg + "." + f.stem for f in parent.iterdir() if
                     f.suffix == '.py']
        controllers_modules.extend(mod_files)
    return controllers_modules


def get_gui_extensions():
    """
    Get the modules in the gui_extensions.

    Return the list of modules in the gui_extensions
    """
    entry_point_group = "karabogui.gui_extensions"
    gui_extensions = [ep.value for ep in
                      importlib.metadata.entry_points(group=entry_point_group)]
    return gui_extensions


# Other modules that are imported on runtime.
hiddenimports = get_controllers_modules() + [
    'karabogui.singletons.db_connection',
    'karabogui.singletons.manager',
    'karabogui.singletons.configuration',
    'karabogui.singletons.mediator',
    'karabogui.singletons.network',
    'karabogui.singletons.panel_wrangler',
    'karabogui.singletons.selection_tracker',
    'karabogui.singletons.project_model',
    'karabogui.topology.api',
] + get_gui_extensions()

modules_to_exclude = ['pytest', 'pytest-cov', 'pytest-mock', 'pytest-qt',
                      'pytest-subtests', 'conda-package-handling', 'conda',
                      'flake8', 'isort', 'pre-commit']

karabo_gui = Analysis(
    [KARABO_GUI_SCRIPT],
    pathex=[],
    binaries=[],
    datas=get_ui_icon_data() + [
        # TODO : Copying 'display' and 'edit' can be avoided after
        # 'karabogui.controls.utils.populate_controller_registry' is modified.
        (f'{KARABO_GUI_DIR}/controllers/display',
         'karabogui/controllers/display'),
        (f'{KARABO_GUI_DIR}/controllers/edit', 'karabogui/controllers/edit'),
        (f'{KARABO_GUI_DIR}/fonts', 'karabogui/fonts'),
    ] + copy_metadata('GUIExtensions'),
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    excludes=modules_to_exclude,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    noarchive=False,
)

karabo_cinema = Analysis(
    [KARABO_CINEMA_SCRIPT],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=modules_to_exclude,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    noarchive=False,
)

karabo_concert = Analysis(
    [KARABO_CONCERT_SCRIPT],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=modules_to_exclude,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    noarchive=False,
    )

karabo_theatre = Analysis(
    [KARABO_THEATRE_SCRIPT],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=modules_to_exclude,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    noarchive=False,
)
MERGE(
    (karabo_gui, 'gui_runner', 'karabo-gui'),
    (karabo_cinema, 'cinema', 'karabo-cinema'),
    (karabo_concert, 'concert', 'karabo-concert'),
    (karabo_theatre, 'theatre', 'karabo-theatre'),
)


karabo_gui_pyz = PYZ(karabo_gui.pure,
                     karabo_gui.zipped_data,)

karabo_gui_exe = EXE(
    karabo_gui_pyz,
    karabo_gui.scripts,
    [],
    exclude_binaries=True,
    name='karabo-gui',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
karabo_gui_coll = COLLECT(
    karabo_gui_exe,
    karabo_gui.binaries,
    karabo_gui.zipfiles,
    karabo_gui.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='karabo-gui',
)

karabo_cinema_pyz = PYZ(karabo_cinema.pure,
                        karabo_cinema.zipped_data,)

karabo_cinema_exe = EXE(
    karabo_cinema_pyz,
    karabo_cinema.scripts,
    [],
    exclude_binaries=True,
    name='karabo-cinema',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
karabo_cinema_coll = COLLECT(
    karabo_cinema_exe,
    karabo_cinema.binaries,
    karabo_cinema.zipfiles,
    karabo_cinema.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='karabo-cinema',
)

karabo_concert_pyz = PYZ(karabo_concert.pure,
                         karabo_concert.zipped_data,)

karabo_concert_exe = EXE(
    karabo_concert_pyz,
    karabo_concert.scripts,
    [],
    exclude_binaries=True,
    name='karabo-concert',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
karabo_concert_coll = COLLECT(
    karabo_concert_exe,
    karabo_concert.binaries,
    karabo_concert.zipfiles,
    karabo_concert.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='karabo-concert',
)

karabo_theatre_pyz = PYZ(karabo_theatre.pure,
                         karabo_theatre.zipped_data,)

karabo_theatre_exe = EXE(
    karabo_theatre_pyz,
    karabo_theatre.scripts,
    [],
    exclude_binaries=True,
    name='karabo-theatre',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
karabo_theatre_coll = COLLECT(
    karabo_theatre_exe,
    karabo_theatre.binaries,
    karabo_theatre.zipfiles,
    karabo_theatre.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='karabo-theatre',
)


karabo_gui_bundle_dir = Path('dist', 'karabo-gui').resolve()

for file_name in ('karabo-cinema', 'karabo-concert', 'karabo-theatre'):
    exec_file = Path('dist', file_name, file_name).resolve()
    if exec_file.is_file():
        destination = Path(karabo_gui_bundle_dir, exec_file.name)
        shutil.copy(exec_file, destination)
        shutil.rmtree(exec_file.parent)
    else:
        raise RuntimeError(f"{exec_file} is not created")
