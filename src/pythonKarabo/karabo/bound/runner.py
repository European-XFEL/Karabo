# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$May 24, 2013 11:36:55 AM$"

import os
import re

from karabind import Hash, loadFromFile

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS

cr_cmt = " # Copyright (C) European XFEL GmbH Schenefeld. All rights reserved."


def removeInitString(args):
    init_pattern = re.compile(r'^\s*init\s*=')
    for token in args:
        if init_pattern.match(token):
            args.remove(token)
            return token


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Runner", "1.0")
class Runner:

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
        configuration = Hash()

        init_pattern = re.compile(r'^\s*init\s*=')
        has_init_string = any(init_pattern.match(token) for token in args)
        if has_init_string:
            json = removeInitString(args).split('=')[1]
            configuration['init'] = json

        firstArg = ""
        if len(args) > 1:
            firstArg = args[1]
        if firstArg[0:2] == "--":
            self.processOption(firstArg[2:], args)
            return False, Hash()
        if firstArg[0:1] == "-":
            self.processOption(firstArg[1:], args)
            return False, Hash()

        pars = self.join_args_in_braces(args)
        self.readTokens('', pars[1:], configuration)
        return True, configuration

    def readTokens(self, prefix, args, configuration):
        for a in args:
            self.readToken(prefix, a, configuration)

    def join_args_in_braces(self, args):
        # fix 'args' to take into account braces
        pars = []
        braces = 0
        arg = ''
        for a in args:
            pos = 0
            while pos < len(a):
                m = re.search("[{}]", a[pos:])
                if m is None:
                    break
                else:
                    pos += m.start()
                    if a[pos] == '{':
                        braces += 1
                    elif a[pos] == '}':
                        braces -= 1
                    pos += 1
                    if braces < 0:
                        raise SyntaxError("CLI Syntax Error: '}' encounters"
                                          " before corresponding '{'")
            if braces == 0:
                pars.append((arg + ' ' + a).strip())
                arg = ''
            else:
                arg += ' ' + a

        if braces > 0:
            raise SyntaxError("CLI Syntax Error: missing {} closing brace(s)"
                              .format(braces))
        elif braces < 0:
            raise SyntaxError("CLI Syntax Error: missing {} opening brace(s)"
                              .format(-braces))
        return pars

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
        print("\n ############################################################"
              "####")
        print(" #                   Karabo Device Server")
        print(" #")
        print(cr_cmt)
        print(" ############################################################"
              "####\n")
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

    def readToken(self, prefix, token, configuration):
        if prefix == '' and os.path.exists(token):
            loadFromFile(configuration, token)
            return
        if token[0] == '{':
            raise SyntaxError(
                f"Key cannot start with opening brace ==> '{token}'")
        pos = token.find("={")
        if pos > 0:
            key = prefix + token[:pos] + '.'
            if token[-1] != '}':
                raise SyntaxError("Missing closing brace '}'")
            value = token[pos + 2:-1].strip()
            args = value.split()
            pars = self.join_args_in_braces(args)
            self.readTokens(key, pars, configuration)
        elif pos == 0:
            raise SyntaxError("Missing the key before '={'")
        else:  # pos == -1 ... not found
            pos = token.find("=")
            if pos > 0:
                key = token[:pos]
                value = token[pos + 1:]
                configuration.set(prefix + key, value)
            elif pos == 0:
                raise SyntaxError("Missing the key before '='")
            else:
                configuration.set(prefix + token, Hash())
