/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "Runner.hh"

#include <algorithm>
#include <regex>
#include <string>
#include <vector>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/io/Input.hh"
#include "karabo/io/Output.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/Version.hh"

using namespace karabo::data;

namespace karabo {

    namespace core {


        DeviceServer::Pointer Runner::instantiate(int argc, const char** argv) {
            const std::string classId("DeviceServer");

            try {
                karabo::data::Hash configuration;
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
                }
            } catch (const karabo::data::Exception& e) {
                KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("Failed to instantiate DeviceServer."));
            }
            return DeviceServer::Pointer();
        }


        bool Runner::parseCommandLine(int argc, const char** argv, karabo::data::Hash& configuration, bool silent) {
            using namespace std;
            using namespace karabo::data;
            std::string firstArg;
            if (argc > 1) {
                firstArg = argv[1];
            }

            if (firstArg.substr(0, 2) == "--") {
                if (!silent) processOption(firstArg.substr(2), argc, argv);
                return false;
            }

            if (firstArg.substr(0, 1) == "-") {
                if (!silent) processOption(firstArg.substr(1), argc, argv);
                return false;
            }

            auto cmdLine = std::vector<std::string>(argv, argv + argc);
            vector<string> args, resolved;

            // Fix 'args' to take into account braces (whitespace inside braces!)
            int braces = 0;
            string arg = "";
            for (size_t i = 1; i < cmdLine.size(); ++i) {
                size_t found = std::string::npos;
                size_t pos = 0;
                string a(cmdLine[i]);

                // Special case: init=json-string
                if (a.substr(0, 5) == "init=") {
                    configuration.set("init", a.substr(5));
                    continue;
                }
                do {
                    found = a.find_first_of("{}", pos);
                    if (found == std::string::npos) break;
                    pos = found;
                    if (a[pos] == '{') {
                        ++braces;
                    } else if (a[pos] == '}') {
                        --braces;
                    }
                    ++pos;
                    if (braces < 0) {
                        throw KARABO_PARAMETER_EXCEPTION("CLI Syntax Error: '}' encounters before corresponding '{'");
                    }
                } while (found != std::string::npos);

                arg = arg.empty() ? a : (arg + " " + a);
                if (braces == 0) {
                    args.push_back(arg);
                    arg = "";
                }
            }

            if (braces > 0) {
                throw KARABO_PARAMETER_EXCEPTION("CLI Syntax Error: missing " + toString(braces) + " closing brace(s)");
            } else if (braces < 0) {
                throw KARABO_PARAMETER_EXCEPTION("CLI Syntax Error: missing " + toString(-braces) +
                                                 " opening brace(s)");
            }

            resolveTokens(args, resolved);

            std::vector<std::string> tokenList;
            for (size_t i = 0; i < resolved.size(); ++i) {
                parseToken("", resolved[i], tokenList);
            }

            karabo::data::Hash flatConfiguration;
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
        }


        void Runner::showUsage(const std::string& programName, const std::string& what) {
            std::cout << "\n ##################################################################\n"
                      << " #                     Karabo Device Server\n"
                      << " #\n"
                      << " # Karabo-Version: " << karabo::util::Version::getVersion() << "\n"
                      << " # Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.\n"
                      << " ##################################################################\n\n";
            if (what.empty()) {
                std::cout << "Usage: " << programName << " [<option> | <configuration>]\n\n";
                std::cout << "<configuration>      A set of (hierarchical) <key>=<value> pairs (see below for keys)\n";
                std::cout << "<option>             Either of the following:\n";
                std::cout << "    -h|--help [key]  This general help or one for 'key'\n";
                std::cout << "    -v|--version     The version\n\n";
                DeviceServer::getSchema("DeviceServer").help();
            } else {
                DeviceServer::getSchema("DeviceServer").help(what);
            }
            std::cout << std::endl;
        }


        void Runner::parseToken(const std::string& prefix, const std::string& token,
                                std::vector<std::string>& tokenList) {
            using namespace std;
            using namespace karabo::data;
            size_t pos = token.find_first_of("=");
            string key, value;
            if (pos == std::string::npos) {
                key = token;
                boost::trim(key);
                if (!prefix.empty()) key = prefix + "." + key;
                tokenList.push_back(key);

            } else {
                key = token.substr(0, pos);
                boost::trim(key);
                if (!prefix.empty()) key = prefix + "." + key;
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
            using namespace karabo::data;

            int argc = argv.size();

            string token;
            for (int i = 0; i < argc;) {
                i = buildToken(argv, i, token);
                args.push_back(token);
                token = "";
            }
        }


        void Runner::processOption(const std::string& option, int argc, const char** argv) {
            std::string lowerOption(boost::to_lower_copy(option));
            if (argc > 2) {
                std::string possibleFileName(argv[2]);
                if (std::filesystem::exists(possibleFileName)) {
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


        int Runner::buildToken(const std::vector<std::string>& args, int start, std::string& token) {
            // This function builds single token fromm the argument list
            // It begins with an argument on position "start" and appends
            // following arguments if needed to build the valid token
            // Function returns the index of the last consumed argument

            using namespace karabo::data;
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
                if (key[0] == '{' || key[0] == '}') {
                    throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line: the key '" + key +
                                                     "' starts with invalid symbol '" + key[0] + "'!");
                }
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
                } else if (value[0] == '{') {
                    int countOpen = boost::count(value, '{');
                    int countClosed = boost::count(value, '}');
                    if (countOpen != countClosed) {
                        if (start < argc) {
                            token += " ";
                            start = buildToken(args, start, token);
                        } else {
                            throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line. Check curly brackets.\n" +
                                                             token);
                        }
                    }
                }
            }

            return start;
        }


        void Runner::readToken(const std::string& token, karabo::data::Hash& config) {
            // This function converts token to Hash entry
            // Tokens must be resolved before (see resolveTokens)
            using namespace karabo::data;

            if (token.empty()) {
                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line (empty argument?)\n");
            }
            if ((*token.begin()) == '.') {
                throw KARABO_PARAMETER_EXCEPTION("Syntax error in command line. Token cannot start with dot (" + token +
                                                 ")");
            }
            std::filesystem::path possibleFile(token);
            if (std::filesystem::exists(possibleFile)) {
                karabo::data::Hash fileConfig;
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


                        for (std::string& subToken : tokens) {
                            boost::trim(subToken);
                            if (!subToken.empty()) {
                                readToken(subToken, config.get<Hash>(key));
                            }
                        }
                    } else {
                        config.set(key, value, ';');
                    }
                }
            }
        }
    } // namespace core
} // namespace karabo
