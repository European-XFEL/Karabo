/*
 * $Id: Runner.hh 4649 2011-11-04 17:46:36Z heisenb@DESY.DE $
 *
 * File:   Runner.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Modified by: <krzysztof.wrona@xfel.eu>
 *
 * Created on December 1, 2011, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_RUNNER_HH
#define	KARABO_CORE_RUNNER_HH

#include <string>
#include <vector>

#include <krb_log4cpp/Category.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Hash.hh>
#include <karabo/io/Output.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/Version.hh>

#include "DeviceServer.hh"
#include <boost/range/algorithm/count.hpp>

/**
 * The main karabo namespace
 */
namespace karabo {

    namespace core {

        /**
         * The Runner class.
         */
        template <class T>
        class Runner {

        public:

            KARABO_CLASSINFO(Runner, "Runner", karabo::util::Version::getVersion())

            KARABO_CONFIGURATION_BASE_CLASS;

            static typename T::Pointer instantiate(int argc, char** argv) {

                typename T::Pointer instancePointer;

                try {

                    karabo::util::Hash configuration = parseCommandLine(argc, argv);

                    if (!configuration.empty())
                        instancePointer = T::create(configuration);

                    return instancePointer;

                } catch (const karabo::util::Exception& e) {
                    std::cout << e.userFriendlyMsg() << std::endl;
                    return instancePointer;
                }
            }

        private:

            static karabo::util::Hash parseCommandLine(int argc, char** argv) {
                using namespace std;
                using namespace karabo::util;
                try {

                    bool autoload = false;
                    string autoLoadFileName("autoload.xml");
                    if (boost::filesystem::exists(autoLoadFileName)) {
                        autoload = true;
                    }

                    if (argc == 1 && autoload == false) {
                        showUsage(std::string(argv[0]));
                        return Hash();
                    }

                    std::string firstArg;
                    if (argc > 1) {
                        firstArg = argv[1];
                    }

                    if (firstArg.substr(0, 2) == "--") {
                        processOption(firstArg.substr(2), argc, argv);
                        return Hash();
                    }

                    if (firstArg.substr(0, 1) == "-") {
                        processOption(firstArg.substr(1), argc, argv);
                        return Hash();
                    }

                    if (firstArg == "help") {
                        processOption(firstArg, argc, argv);
                        return Hash();
                    }

                    karabo::util::Hash configuration, flatConfiguration;
                    if (autoload) {
                        // Auto load configuration
                        string autoLoadFileName("autoload.xml");
                        if (boost::filesystem::exists(autoLoadFileName)) {
                            karabo::io::loadFromFile<Hash>(configuration, autoLoadFileName);
                        }
                        configuration.flatten(flatConfiguration);
                        configuration.clear();
                    }


                    vector<string> args, resolved;
                    for (int i = 1; i < argc; ++i) {
                        args.push_back(argv[i]);
                    }

                    resolveTokens(args, resolved);

                    std::vector<std::string> tokenList;
                    for (size_t i = 0; i < resolved.size(); ++i) {
                        parseToken("", resolved[i], tokenList);
                    }


                    Hash tmp;
                    for (size_t i = 0; i < tokenList.size(); ++i) {
                        readToken(tokenList[i], tmp);
                        for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                            std::string key = it->getKey();
                            if (flatConfiguration.has(key, ';')) {
                                flatConfiguration.erase(key, ';');
                            }

                            const Hash::Node tmpNode = tmp.getNode(key, ';');
                            if (tmpNode.getType() != Types::VECTOR_HASH) {
                                flatConfiguration.set(tmpNode.getKey(), tmpNode.getValueAsAny(), ';');
                            } else {
                                flatConfiguration.unflatten(configuration);
                                if (configuration.has(tmpNode.getKey())) {
                                    configuration.erase(tmpNode.getKey());
                                }
                                flatConfiguration.clear();
                                configuration.flatten(flatConfiguration);
                                configuration.clear();
                            }
                        }
                    }

                    flatConfiguration.unflatten(configuration);

                    karabo::io::saveToFile<Hash>(configuration, "lastConfiguration.xml");
                    return configuration;

                } catch (const Exception& e) {
                    cout << endl << e.userFriendlyMsg() << endl;
                    return Hash();
                }
            }

            static void parseToken(const std::string& prefix, const std::string& token, std::vector<std::string>& tokenList) {
                using namespace std;
                using namespace karabo::util;
                size_t pos = token.find_first_of("=");
                string key, value;
                if (pos == std::string::npos) {

                    key = token;
                    boost::trim(key);
                    if (!prefix.empty())
                        key = prefix + "." + key;
                    tokenList.push_back(key);

                } else {

                    key = token.substr(0, pos);
                    boost::trim(key);
                    if (!prefix.empty())
                        key = prefix + "." + key;
                    value = token.substr(pos + 1);
                    boost::trim(value);
                    if (!value.empty() && (*value.begin()) == '{' && (*value.rbegin()) == '}') {

                        string line = value.substr(1, value.length() - 2);
                        boost::trim(line);


                        vector<string> args, resolved;
                        boost::split(args, line, boost::is_any_of(" "));
                        resolveTokens(args, resolved);
                        for (size_t i = 0; i < resolved.size(); ++i) {
                            parseToken(key, resolved[i], tokenList);
                        }
                    } else {
                        string finalToken = key + "=" + value;
                        tokenList.push_back(finalToken);
                    }

                }

            }

            static void resolveTokens(const std::vector<std::string>& argv, std::vector<std::string>& args) {
                // this function converts arguments to tokens
                // Arguments are separated by spaces. Tokens may contain spaces
                // Examples of some supported tokens:
                // a=23
                // b = 45
                // k.a = "abc"
                // c = { d=5 a.v = {m=4 } }
                // DeviceServer
                // DeviceServer.Logger.appenders[]

                using namespace std;
                using namespace karabo::util;

                int argc = argv.size();

                string token;
                for (int i = 0; i < argc;) {
                    i = buildToken(argv, i, token);
                    args.push_back(token);
                    token = "";
                }
            }

            static void processOption(const std::string& option, int argc, char** argv) {
                std::string lowerOption(boost::to_lower_copy(option));
                if (argc > 2) {
                    std::string possibleFileName(argv[2]);
                    if (boost::filesystem::exists(possibleFileName)) {
                        argc--;
                    }
                }

                if (lowerOption == "create-xsd") {
                    if (argc > 2) {
                        std::string classId = std::string(argv[2]);
                        classId.substr(0, classId.find_first_of("."));
                        karabo::util::Schema schema = T::getSchema(classId, karabo::util::Schema::AssemblyRules(karabo::util::READ | karabo::util::WRITE | karabo::util::INIT));
                        std::cout << std::endl << "Generating list of expected parameters. Writing output to file: " << classId << ".xsd " << std::endl << std::endl;
                        karabo::io::saveToFile<karabo::util::Schema>(schema, classId + ".xsd");
                    } else {
                        std::cout << "Expecting command line input, telling for whom the xsd file should be generated." << std::endl;
                    }
                } else if (lowerOption == "help") {
                    if (argc > 2) {
                        std::string classId = std::string(argv[2]);
                        classId.substr(0, classId.find_first_of("."));
                        showUsage(std::string(argv[1]), classId);
                    } else {
                        showUsage(std::string(argv[0]));
                    }
                } else if (lowerOption == "version" || lowerOption == "v") {

                    std::cout << "Karabo-Version: " << karabo::util::Version::getVersion() << std::endl;

                } else {
                    showUsage(std::string(argv[0]));
                }
            }

            static void showUsage(const std::string& programName, const std::string& what = "") {
                printXfelWelcome();
                std::cout << "Usage: " << programName << " <configuration>\n" << std::endl;
                std::string runnableType = T::classInfo().getClassName();
                if (what.empty()) {
                    std::cout << "The <configuration> reflects a set of (hierarchical) key-value types." << std::endl;
                    std::cout << "You can supply <configuration> information as xml file or as command-line input or a combination of both.\n" << std::endl;
                    std::cout << "Example:\nAssume the key \"" << runnableType << ".someThreshold\" and a corresponding value \"4.2\".\nThe corresponding xml file should look like this:\n" << std::endl;
                    std::cout << "  \"<" << runnableType << "><someThreshold>4.2</someThreshold></" << runnableType << ">\"\n\nIf you saved the file under \"config.xml\" you should then type:\n  '" << programName << " config.xml'\n\n\n";
                    std::cout << "For the same configuration given as command line arguments you should type:\n  '" << programName << " " << runnableType << ".someThreshold=\"4.2\"'\n\n";
                    std::cout << "Following " << T::classInfo().getClassName() << " <choice>s are available: " << std::endl;
                    std::cout << karabo::util::toString(T::getRegisteredClasses());
                    std::cout << "\nType: '" << programName << " help <choice>' for help on a specific choice" << std::endl;
                    std::cout << "Type: '" << programName << " --create-xsd <choice>' to generate full description of all parameters (in xml schema format)" << std::endl;
                } else {
                    std::string classId = what;
                    std::string path;
                    size_t pos = what.find_first_of(".");
                    if (pos != std::string::npos) {
                        classId = what.substr(0, pos);
                        path = what.substr(pos + 1);
                    }
                    T::getSchema(classId).help(path);
                }
                std::cout << std::endl;
            }

            static int buildToken(const std::vector<std::string>& args, int start, std::string & token) {
                // This function builds single token fromm the argument list 
                // It begins with an argument on position "start" and appends 
                // following arguments if needed to build the valid token
                // Function returns the index of the last consumed argument

                using namespace karabo::util;
                using namespace std;

                token += args[start];
                int argc = args.size();
                boost::trim(token);

                size_t pos = token.find_first_of("=");
                string key, value;
                if (pos == std::string::npos) {
                    start++;
                    if (start < argc) {
                        if (*(args[start].begin()) == '=') {
                            start = buildToken(args, start, token);
                        } else {
                            if ((*token.begin()) == '{' || (*token.rbegin()) == '}') {
                                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line \n" + token);
                            }


                        }
                    }
                    return start;


                } else {
                    key = token.substr(0, pos);
                    boost::trim(key);
                    value = token.substr(pos + 1);
                    boost::trim(value);
                    start++;
                    if (value.empty()) {
                        if (start < argc) {
                            if (*(args[start].begin()) != '=') {
                                start = buildToken(args, start, token);
                            } else {
                                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line\n" + token);
                            }
                        } else {
                            throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line \n" + token);
                        }
                    }
                    if ((*value.begin()) == '{') {
                        int countOpen = boost::count(value, '{');
                        int countClosed = boost::count(value, '}');
                        if (countOpen != countClosed) {
                            if (start < argc) {
                                token += " ";
                                start = buildToken(args, start, token);
                            } else {
                                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line. Check curly brackets.\n" + token);
                            }
                        }
                    }
                }

                return start;
            }

            static void readToken(const std::string& token, karabo::util::Hash & config) {
                // This function converts token to Hash entry
                // Tokens must be resolved before (see resolveTokens)
                using namespace karabo::util;

                if(token.empty()){
                    throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line (empty argument?)\n");
                }
                if ((*token.begin()) == '.'){
                    throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line. Token cannot start with dot (" + token + ")");
                }
                boost::filesystem::path possibleFile(token);
                if (boost::filesystem::exists(possibleFile)) {
                    karabo::util::Hash fileConfig;
                    karabo::io::loadFromFile<Hash>(fileConfig, possibleFile.string());
                    fileConfig.flatten(config);
                } else {
                    size_t pos = token.find_first_of("=");
                    if (pos == std::string::npos) {
                        string key = token;
                        if (!token.empty() && (*token.begin()) == '{' && (*token.rbegin()) == '}') {
                            key = token.substr(1, token.size() - 2);
                        }
                        if (token.size() > 2 && token.substr(token.length() - 2, 2) == "[]") {
                            key = token.substr(0, token.length() - 2);
                            if (key.empty()) {
                                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line \n" + token);
                            }
                            config.set(key, vector<Hash>(), ';');
                        } else {
                            if (key.empty()) {
                                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line \n" + token);
                            }
                            config.set(key, Hash(), ';');
                        }
                    } else {
                        std::string key(token.substr(0, pos));
                        boost::trim(key);
                        std::string value(token.substr(pos + 1));
                        boost::trim(value);

                        if (!value.empty() && (*value.begin()) == '{' && (*value.rbegin()) == '}') {
                            value = value.substr(1, value.size() - 2);
                            boost::trim(value);
                            std::vector<std::string> tokens;
                            boost::split(tokens, value, boost::is_any_of(" "));
                            config.set(key, Hash(), ';');

                            BOOST_FOREACH(std::string subToken, tokens) {
                                boost::trim(subToken);
                                if (!subToken.empty()) {
                                    readToken(subToken, config.get<Hash > (key));
                                }
                            }
                        } else {
                            config.set(key, value, ';');
                        }
                    }
                }
            }

            static void printXfelWelcome() {
                std::string runnableType = T::classInfo().getClassName();
                std::string runnableVersion = T::classInfo().getVersion();
                std::cout << "\n ##################################################################\n"
                        << " #             Simple Karabo " << runnableType << " Runner\n"
                        << " #\n"
                        << " # Karabo-Version: " << karabo::util::Version::getVersion() << "\n"
                        << " # Copyright (C) European XFEL GmbH Hamburg. All rights reserved.\n"
                        << " ##################################################################\n\n";
            }

        };
    }
}

#endif