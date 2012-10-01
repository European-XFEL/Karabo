from cx_Freeze import setup, Executable

#packages = ["displaywidgets", "editablewidgets"]
build_exe_options = {"packages": ["displaywidgets", "editablewidgets"]}

setup(
        name = "karabo-gui",
        version = "0.1",
        description = "The GUI for the KARABO framework.",
        #options = {"build_exe": {"includes": includes}},
        options = {"build_exe": build_exe_options},
        executables = [Executable("karabo-gui.py")]
     )

