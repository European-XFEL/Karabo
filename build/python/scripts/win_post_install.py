"""Windows-specific part of the installation"""

import os, sys
import karaboGui

def create_shortcut_safe(target, description, link_file, args, workdir):
    create_shortcut(target, description, link_file, args, workdir)
    file_created(link_file)

def install():
    prefix = sys.prefix
    python = prefix + r'\pythonw.exe'

    # Lookup path to common startmenu ...
    mv_dir = get_special_folder_path('CSIDL_COMMON_PROGRAMS') + r'\Karabo'
    if not os.path.isdir(mv_dir):
        os.mkdir(mv_dir)
        directory_created(mv_dir)

    # Create program shortcuts ...
    f = mv_dir + r'\Karabo.lnk'
    create_shortcut_safe(python, 'Karabo', mv_dir + r'\Karabo.lnk',
            karaboGui.__path__[0] + r"\karabo-gui.py",
            workdir=get_special_folder_path('CSIDL_DESKTOPDIRECTORY'))

def remove():
    pass

# main()
if len(sys.argv) > 1:
    if sys.argv[1] == '-install':
        install()
    elif sys.argv[1] == '-remove':
        remove()
    else:
        print "Script was called with option %s" % sys.argv[1]
