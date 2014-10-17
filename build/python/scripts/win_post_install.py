"""Windows-specific part of the installation"""
from __future__ import print_function
import os, sys
from distutils.sysconfig import get_python_lib


def create_shortcut_safe(target, description, link_file, args, workdir):
    create_shortcut(target, description, link_file, args, workdir)
    file_created(link_file)

def install():
    prefix = sys.prefix
    python = prefix + r'\pythonw.exe'

    # Lookup path to common startmenu ...
    mv_dir = get_special_folder_path('CSIDL_COMMON_PROGRAMS') 
    mv_dir += r'\Karabo'

    if not os.path.isdir(mv_dir):
        os.mkdir(mv_dir)
        directory_created(mv_dir)

    karaboGuiPath = get_python_lib() + r'\karaboGui'
    guiScript = karaboGuiPath + r'\karabo-gui.py'
    workDir=get_special_folder_path('CSIDL_DESKTOPDIRECTORY')

    # Create program shortcuts ...
    shortcut = mv_dir + r'\Karabo.lnk'
    create_shortcut_safe(python, 'Karabo', shortcut, guiScript, workdir=workDir)

def remove():
    pass

if len(sys.argv) > 1:
    if sys.argv[1] == '-install':
        install()
    elif sys.argv[1] == '-remove':
        remove()
    else:
        pass
        #print "Script was called with option %s" % sys.argv[1]"
