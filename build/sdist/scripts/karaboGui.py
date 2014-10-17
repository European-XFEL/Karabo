#!/usr/bin/env python
import site, os

sitePaths=site.getsitepackages()
sitePaths.append(site.getusersitepackages())

for path in sitePaths:
    karaboGuiPath = os.path.join(path, 'karaboGui')
    guiScript = os.path.join(karaboGuiPath, 'karabo-gui.py')
    if os.path.exists(guiScript):
        os.system("python " + guiScript)

