__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$May 24, 2013 11:36:55 AM$"

import os
import re

from karathon import Hash, loadFromFile
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Runner", "1.0")
class Runner(object):

    def __init__(self, deviceServer):
        self.deviceServer = deviceServer

    def instantiate(self, args):
        classId = 'DeviceServer'
        try:
            ret = self.parseCommandLine(args)
            if ret[0]:
                config = ret[1]
                if not config.empty():
                    if config.has(classId):
                        return self.deviceServer.create(config)
                    else:
                        return self.deviceServer.create(classId, config)
                else:
                    return self.deviceServer.create(classId, config)
            else:
                return None
        except Exception as e:
            print(str(e))
            return None

    def parseCommandLine(self, args):
        try:
            firstArg = ""
            if len(args) > 1:
                firstArg = args[1]
            if firstArg[0:2] == "--":
                self.processOption(firstArg[2:], args)
                return False, Hash()
            if firstArg[0:1] == "-":
                self.processOption(firstArg[1:], args)
                return False, Hash()

            # fix 'args' to take into account braces
            pars = []
            braces = 0
            arg = ''
            for a in args:
                pos = 0
                while True:
                    m = re.search('[{}]', a[pos:])
                    if m is None:
                        break
                    else:
                        pos = m.start()
                        if a[pos] == '{':
                            braces += 1
                        elif a[pos] == '}':
                            braces -= 1
                        pos = m.end()
                        if braces < 0:
                            raise SyntaxError("CLI Syntax Error: '}' encounters before corresponding '{'")
                if braces == 0:
                    pars.append((arg + ' ' + a).strip())
                    arg = ''
                else:
                    arg += ' ' + a
                    
            if braces > 0:
                raise SyntaxError("CLI Syntax Error: missing {} closing brace(s)".format(braces))
            elif braces < 0:
                raise SyntaxError("CLI Syntax Error: missing {} opening brace(s)".format(-braces))
                
            configuration = Hash()

            for a in pars[1:]:
                tmp = Hash()
                self.readToken(a, tmp)
                configuration += tmp
            return True, configuration

        except Exception as e:
            print(str(e))
            return False, Hash()

    def processOption(self, option, args):
        lowerOption = option.lower()
        if lowerOption == "help" or lowerOption == "h":
            if len(args) > 2:
                self.showUsage(args[1], args[2])
            else:
                self.showUsage(args[0])
        # TODO implement -v, --version option
        else:
            self.showUsage(args[0])

    def showUsage(self, name, what=""):
        print("\n ################################################################")
        print(" #                   Karabo Device Server")
        print(" #")
        print(" # Copyright (C) European XFEL GmbH Hamburg. All rights reserved.")
        print(" ################################################################\n")
        if not what:
            print("Usage: {} <configuration>\n".format(name.split('/')[-1]))
            print("Positional arguments:\n")
            print("<configuration> A set of (hierarchical) <key>=<value> "
                  "pairs")
            print("                Use: --help [key] to list available keys "
                  "or sub-keys, respectively")
            self.deviceServer.getSchema('DeviceServer').help()
        else:
            self.deviceServer.getSchema('DeviceServer').help(what)
        print("")

    def readToken(self, token, config):
        if os.path.exists(token):
            loadFromFile(config, token)
        else:
            pos = token.find("=")
            if pos == -1:
                config[token] = Hash()
            else:
                key = token[0:pos].strip()
                value = token[pos+1:].strip()
                if value != "" and value[0] == "{" and value[-1] == "}":
                    value = value[1:-1].strip()
                    tokens = value.split(" ")
                    config.set(key, Hash())
                    for subtok in tokens:
                        subtok = subtok.strip()
                        if subtok != "":
                            if "[" in key and "]" in key:
                                keyToks = key.split("[")
                                skey = keyToks[0]
                                index = int(keyToks[1][:-1])
                                self.readToken(subtok, config[skey][index])
                            else:
                                self.readToken(subtok, config[key])
                else:
                    config.set(key, value)

