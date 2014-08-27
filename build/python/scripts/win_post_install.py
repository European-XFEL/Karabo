#!python
"""Windows-specific part of the installation"""
from __future__ import print_function
import os, sys
#import karaboGui
from distutils.sysconfig import get_python_lib

def create_shortcut_safe(target, description, link_file, args, workdir):
    create_shortcut(target, description, link_file, args, workdir)
    file_created(link_file)

def install():
    prefix = sys.prefix
    python = prefix + r'\pythonw.exe'
    fn = open(r"C:\test.txt",'w')
    print('Creating Shortcut', file=fn)
    print(python, file=fn)

    # Lookup path to common startmenu ...
    mv_dir = get_special_folder_path('CSIDL_COMMON_PROGRAMS') 
    print(mv_dir, file=fn)
    mv_dir += r'\Karabo'
 
    print(mv_dir, file=fn)
    if not os.path.isdir(mv_dir):
        os.mkdir(mv_dir)
        directory_created(mv_dir)
    print('echo', file=fn)
    print(get_python_lib(), file=fn)
    karaboGuiPath = get_python_lib() + r'\karaboGui'
    print(karaboGuiPath, file=fn)
    guiScript = karaboGuiPath + r'\karabo-gui.py'
    print(guiScript, file=fn)
    workDir=get_special_folder_path('CSIDL_DESKTOPDIRECTORY')
    print(workDir, file=fn)
    # Create program shortcuts ...
    shortcut = mv_dir + r'\Karabo.lnk'
    print('echo2', file=fn)
    create_shortcut_safe(python, 'Karabo', shortcut, guiScript, workdir=workDir)
    print('echo3', file=fn)

def remove():
    pass

# main()
if len(sys.argv) > 1:
    if sys.argv[1] == '-install':
        install()
        print('running install', file=fn)
    elif sys.argv[1] == '-remove':
        remove()
    else:
       # print "Script was called with option %s" % sys.argv[1]
        print("Script was called with option ", file=f)
