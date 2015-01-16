#!python
"""Windows-specific part of the installation"""
from __future__ import print_function
import os, sys
from distutils.sysconfig import get_python_lib


def create_shortcut_safe(target, description, link_file, args, workdir):
    create_shortcut(target, description, link_file, args, workdir)
    file_created(link_file)

def install():
    prefix = sys.prefix
    python = os.path.join(prefix, 'pythonw.exe')
    title = 'KaraboGUI-VERSION'

    # Lookup path to common startmenu ...
    mv_dir = get_special_folder_path('CSIDL_COMMON_PROGRAMS') 
    mv_dir = os.path.join(mv_dir, title)

    if not os.path.isdir(mv_dir):
        os.mkdir(mv_dir)
        directory_created(mv_dir)

    karaboGuiPath = os.path.join(get_python_lib(), 'karaboGui')
    guiScript = os.path.join(karaboGuiPath, 'karabo-gui.py')
    workDir=get_special_folder_path('CSIDL_DESKTOPDIRECTORY')

    # Create program shortcuts ...
    shortcut = os.path.join(mv_dir, "{}.lnk".format(title))
    create_shortcut_safe(python, title, shortcut, guiScript, workdir=workDir)

def remove():
    pass

if len(sys.argv) > 1:
    if sys.argv[1] == '-install':
        f = open(r"D:\test.txt",'w')
        print('Creating Shortcut', file=f)
        install()
    elif sys.argv[1] == '-remove':
        remove()
    else:
        pass
        #print "Script was called with option %s" % sys.argv[1]"

