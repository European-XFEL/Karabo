# -*- mode: python ; coding: utf-8 -*-
"""
This stores the specifications to  create bundle for karabo gui, alarm panel,
karabo cinema and karabo theatre.

To create bundle,
 - Activate karabogui conda environment
 - Set 'KARABO_GUI_SRC' env variable to your `src/pythonGui/karabogui`
 - Run 'pyinstaller karabo-gui.spec'
"""
import importlib
import os
import shutil
from collections import defaultdict
from pathlib import Path

import pkg_resources

HOOK_FILE_CONTENT = """# hookup file to define gui_extensions entry points.
ep_packages = {}

if ep_packages:
    import pkg_resources
    default_iter_entry_points = pkg_resources.iter_entry_points

    def hook_iter_entry_points(group, name=None):
        if group in ep_packages and ep_packages[group]:
            eps = ep_packages[group]
            for ep in eps:
                parsedEp = pkg_resources.EntryPoint.parse(ep)
                parsedEp.dist = pkg_resources.Distribution()
                yield parsedEp
        else:
            return default_iter_entry_points(group, name)

    pkg_resources.iter_entry_points = hook_iter_entry_points
"""


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
ALARM_GUI_SCRIPT = Path(KARABO_GUI_DIR, 'programs/alarm_runner.py')
KARABO_CINEMA_SCRIPT = Path(KARABO_GUI_DIR, 'programs/cinema.py')
KARABO_THEATRE_SCRIPT = Path(KARABO_GUI_DIR, 'programs/theatre.py')


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
    extensions = (".ui", ".png", ".svg", ".gif")
    for file_name in KARABO_GUI_DIR.rglob("*.*"):
        extension = file_name.suffix
        if extension not in extensions:
            continue
        dest_dir = str(file_name.parent).replace(str(KARABO_GUI_DIR), "")
        dest = Path(f"karabogui{dest_dir}")
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
    Get the modules in the gui_extensions and write a hook file to import them
    in the bundle.

    Return the list of modules in the gui_extensions
    """
    gui_extensions = []
    hook_ep_packages = defaultdict(list)

    ep_package = "karabogui.gui_extensions"

    for ep in pkg_resources.iter_entry_points(ep_package):
        package_entry_point = hook_ep_packages[ep_package]
        package_entry_point.append(f"{ep.name} = {ep.module_name}")
        gui_extensions.append(ep.module_name)
    write_gui_extensions_hook(hook_ep_packages)
    return gui_extensions


def write_gui_extensions_hook(hook_ep_packages):
    """
    Write a hook up file for modules in gui_extension as they are imported
    as entry points.
    """

    hook_file = Path("extensions_hook/pkg_resources_hook.py")
    hook_file.parent.mkdir(exist_ok=True)
    with hook_file.open("w") as f:
        f.write(HOOK_FILE_CONTENT.format(dict(hook_ep_packages)))


# Other modules that are imported on runtime.
hiddenimports = get_controllers_modules() + [
    'karabogui.alarms.api',
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


block_cipher = None

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
    ],
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=["./extensions_hook/pkg_resources_hook.py"],
    excludes=modules_to_exclude,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

alarm_gui = Analysis(
    [ALARM_GUI_SCRIPT],
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
    cipher=block_cipher,
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
    cipher=block_cipher,
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
    cipher=block_cipher,
    noarchive=False,
)
MERGE(
    (karabo_gui, 'gui_runner', 'karabo-gui'),
    (alarm_gui, 'alarm_runner', 'alarm-gui'),
    (karabo_cinema, 'cinema', 'karabo-cinema'),
    (karabo_theatre, 'theatre', 'karabo-theatre'),
)


karabo_gui_pyz = PYZ(karabo_gui.pure,
                     karabo_gui.zipped_data,
                     cipher=block_cipher)

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


alarm_gui_pyz = PYZ(alarm_gui.pure, alarm_gui.zipped_data, cipher=block_cipher)

alarm_gui_exe = EXE(
    alarm_gui_pyz,
    alarm_gui.scripts,
    [],
    exclude_binaries=True,
    name='alarm-gui',
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
alarm_gui_coll = COLLECT(
    alarm_gui_exe,
    alarm_gui.binaries,
    alarm_gui.zipfiles,
    alarm_gui.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='alarm-gui',
)

karabo_cinema_pyz = PYZ(karabo_cinema.pure,
                        karabo_cinema.zipped_data,
                        cipher=block_cipher)

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

karabo_theatre_pyz = PYZ(karabo_theatre.pure,
                         karabo_theatre.zipped_data,
                         cipher=block_cipher)

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
for file_name in ('alarm-gui', 'karabo-cinema', 'karabo-theatre'):

    exec_file = Path('dist', file_name, file_name).resolve()
    if exec_file.is_file():
        destination = Path(karabo_gui_bundle_dir, exec_file.name)
        shutil.copy(exec_file, destination)
        shutil.rmtree(exec_file.parent)
    else:
        raise RuntimeError(f"{exec_file} is not created")
