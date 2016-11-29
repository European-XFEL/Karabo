#include "Runner.hh"

#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/io/Output.hh"
#include "karabo/io/Input.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/Version.hh"

#include <krb_log4cpp/Category.hh>

#include <string>
#include <vector>

using namespace karabo::util;

namespace karabo {

    namespace core {


        DeviceServer::Pointer Runner::instantiate(int argc, char** argv) {

            const std::string classId("DeviceServer");

            try {

                karabo::util::Hash configuration;
                if (parseCommandLine(argc, argv, configuration)) {

                    if (!configuration.empty()) {
                        if (configuration.has(classId)) {
                            return Configurator<DeviceServer>::create(configuration);
                        } else {
                            return Configurator<DeviceServer>::create(classId, configuration);
                        }
                    } else {
                        return Configurator<DeviceServer>::create(classId);
                    }
                } else {
                    return DeviceServer::Pointer();
                }
            } catch (const karabo::util::Exception& e) {
                std::cout << e.userFriendlyMsg() << std::endl;
                return DeviceServer::Pointer();
            }
        }


        bool Runner::parseCommandLine(int argc, char** argv, karabo::util::Hash & configuration) {
            using namespace std;
            using namespace karabo::util;
            try {

                std::string firstArg;
                if (argc > 1) {
                    firstArg = argv[1];
                }

                if (firstArg.substr(0, 2) == "--") {
                    processOption(firstArg.substr(2), argc, argv);
                    return false;
                }

                if (firstArg.substr(0, 1) == "-") {
                    processOption(firstArg.substr(1), argc, argv);
                    return false;
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

                karabo::util::Hash flatConfiguration;
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

                return true;

            } catch (const Exception& e) {
                cout << endl << e.userFriendlyMsg() << endl;
                return false;
            }
        }


        void Runner::showUsage(const std::string& programName, const std::string & what) {
            std::cout << "\n ##################################################################\n"
                    << " #                     Karabo Device Server\n"
                    << " #\n"
                    << " # Karabo-Version: " << karabo::util::Version::getVersion() << "\n"
                    << " # Copyright (C) European XFEL GmbH Hamburg. All rights reserved.\n"
                    << " ##################################################################\n\n";
            if (what.empty()) {
                std::cout << "\nUsage: " << programName << " <configuration>\n" << std::endl;
                std::cout << "\nPositional arguments:" << std::endl;
                std::cout << "<configuration> " << "A set of (hierarchical) <key> = <value> pairs" << std::endl;
                std::cout << "                " << "Use: --help [key] to list available keys or sub-keys, respectively" << std::endl;
                DeviceServer::getSchema("DeviceServer").help();
            } else {
                DeviceServer::getSchema("DeviceServer").help(what);
            }
            std::cout << std::endl;
        }


        void Runner::parseToken(const std::string& prefix, const std::string& token, std::vector<std::string>& tokenList) {
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


        void Runner::resolveTokens(const std::vector<std::string>& argv, std::vector<std::string>& args) {
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


        void Runner::processOption(const std::string& option, int argc, char** argv) {
            std::string lowerOption(boost::to_lower_copy(option));
            if (argc > 2) {
                std::string possibleFileName(argv[2]);
                if (boost::filesystem::exists(possibleFileName)) {
                    argc--;
                }
            }

            if (lowerOption == "help" || lowerOption == "h") {
                if (argc > 2) {
                    showUsage(std::string(argv[0]), std::string(argv[2]));
                } else {
                    showUsage(std::string(argv[0]));
                }
            } else if (lowerOption == "version" || lowerOption == "v") {

                std::cout << "Karabo-Version: " << karabo::util::Version::getVersion() << std::endl;

            } else {
                showUsage(std::string(argv[0]));
            }
        }


        int Runner::buildToken(const std::vector<std::string>& args, int start, std::string & token) {
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


        void Runner::readToken(const std::string& token, karabo::util::Hash & config) {
            // This function converts token to Hash entry
            // Tokens must be resolved before (see resolveTokens)
            using namespace karabo::util;

            if (token.empty()) {
                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line (empty argument?)\n");
            }
            if ((*token.begin()) == '.') {
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
                    std::string key = token;
                    if (!token.empty() && (*token.begin()) == '{' && (*token.rbegin()) == '}') {
                        key = token.substr(1, token.size() - 2);
                    }
                    if (token.size() > 2 && token.substr(token.length() - 2, 2) == "[]") {
                        key = token.substr(0, token.length() - 2);
                        if (key.empty()) {
                            throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line \n" + token);
                        }
                        config.set(key, std::vector<Hash>(), ';');
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
    }
}