__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 24, 2013 11:36:55 AM$"

import os
import sys
import traceback
from karabo.configurator import Configurator
from karabo.karathon import Hash, loadFromFile, saveToFile
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Runner", "1.0")
class Runner(object):

    def __init__(self, theclass):
        self.theClass = theclass
        
    def instantiate(self, args):
        instance = None
        try:
            configuration = self.parseCommandLine(args)
            if not configuration.empty():
                instance = self.theClass.create(configuration)
        except:
            print("Exception in user code:")
            print('-'*60)
            traceback.print_exc(file=sys.stdout)
            print('-'*60)
        
        return instance
    
    def parseCommandLine(self, args):
        try:
            if len(args) == 1:
                self.showUsage(args[0])
                return Hash()
            firstArg = args[1]
            if firstArg[0:2] == "--":
                self.processOption(firstArg[2:], args)
                return Hash()
            if firstArg[0:1] == "-":
                processOption(firstArg[1:], args)
                return Hash()
            if firstArg == "help":
                self.processOption(firstArg, args)
                return Hash()
            configuration = Hash()
            for a in args[1:]:
                tmp = Hash()
                self.readToken(a, tmp)
                configuration += tmp
            # auto load configuration "autoload.xml"
            autoLoadFileName = "autoload.xml"
            if os.path.exists(autoLoadFileName):
                tmp = Hash()
                loadFromFile(tmp, autoLoadFileName)
                configuration += tmp
            saveToFile(configuration, "lastConfiguration.xml")
            return configuration
        except Exception as e:
            print(str(e))
            return Hash()
        
    def processOption(self, option, args):
        lowerOption = option.lower()
        if lowerOption == "create-xsd":
            if len(args) > 2:
                classid = args[2]
                pos = classid.find(".")
                if pos > 0:
                    classid = classid[0, pos]
                schema = self.theClass.getSchema(classid)
                print("\nGenerating list of expected parameters. Writing output to file: %s.xsd\n" % classid)
                saveToFile(schema, classid + ".xsd")
            else:
                print("Expecting command line input, telling for whom the xsd file should be generated.")
        elif lowerOption == "help":
            if len(args) > 2:
                self.showUsage(args[1], args[2])
            else:
                self.showUsage(args[0])
        elif lowerOption == "version" or lowerOption == "v":
            if len(args) > 2:
                classid = args[2]
                pos = classid.find(".")
                if pos > 0:
                    classid = classid[0, pos]
                # TODO implement
            else:
                print("Runner-Version: %s" % Runner.__version__);
                print("%s--Version: %s" % (theClass.__name__, theClass.__version__))
        else:
            self.showUsage(args[0])
    
    def showUsage(self, programName, what = ""):
        self.printXfelWelcome()
        print("Usage: %s <configuration>\n" % programName)
        runnableType = self.theClass.__name__
        if what == "":
            print("The <configuration> reflects a set of (hierarchical) key-value types.")
            print("You can supply <configuration> information as xml file or as command-line input or a combination of both.\n")
            print("Example:\nAssume the key \"%s.someThreshold\" and a corresponding value \"4.2\".\nThe corresponding xml file should look like this:\n" % runnableType)
            print("  \"<%s><someThreshold>4.2</someThreshold></%s>\"\n\nIf you saved the file under \"config.xml\" you should then type:\n  '%s config.xml'\n\n" % (runnableType, runnableType, programName))
            print("For the same configuration given as command line arguments you should type:\n  '%s %s.someThreshold=\"4.2\"'\n" % (programName, runnableType))
            print("Following %s <choice>s are available:" % runnableType)
            print(self.theClass.getRegisteredClasses())
            print("\nType: '%s help <choice>' for help on a specific choice" % programName)
            print("Type: '%s --create-xsd <choice>' to generate full description of all parameters (in xml schema format)" % programName)
        else:
            classid = what
            pos = what.find('.')
            if pos >= 0:
                classid = what[0:pos]
                path = what[pos+1:]
            else:
                path = ""
            self.theClass.getSchema(classid).help(path)
        print()
        
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
                if value != "" and value[0] == "{" and value[len(value) - 1] == "}":
                    value = value[1, len(value) - 2].strip()
                    tokens = value.split(" ")
                    config[key] = Hash()
                    for subtok in tokens:
                        subtok = subtok.strip()
                        if subtok != "":
                            self.readToken(subtok, config[key])
                else:
                    config[key] = value
        
    def printXfelWelcome(self):
        runnableType = self.theClass.__name__
        runnableVersion = self.theClass.__version__
        print("\n ##################################################################")
        print(" #             Simple Karabo %s Runner" % runnableType)
        print(" #")
        print(" # Runner-Version: %s" % Runner.__version__)
        print(" # Copyright (C) European XFEL GmbH Hamburg. All rights reserved.")
        print(" ##################################################################\n")
        
        