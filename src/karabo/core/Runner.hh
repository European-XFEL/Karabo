/*
 * $Id: Runner.hh 4649 2011-11-04 17:46:36Z heisenb@DESY.DE $
 *
 * File:   Runner.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on December 1, 2011, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_RUNNER_HH
#define	KARABO_CORE_RUNNER_HH

#include <string>

#include <log4cpp/Category.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/io/Output.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/log/Logger.hh>

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

            KARABO_CLASSINFO(Runner, "Runner", "1.0")

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
                    if (argc == 1) {
                        showUsage(std::string(argv[0]));
                        return Hash();
                    }
                    // Check first argument
                    std::string firstArg(argv[1]);
                    if (firstArg.substr(0, 2) == "--") {
                        processOption(firstArg.substr(2), argc, argv);
                        return Hash();
                    } else if (firstArg.substr(0, 1) == "-") {
                        processOption(firstArg.substr(1), argc, argv);
                        return Hash();
                    } else if (firstArg == "help") {
                        processOption(firstArg, argc, argv);
                        return Hash();
                    } else {
                        // Loop all arguments an assemble Hash on the way
                        Hash configuration;
                        for (int i = 1; i < argc; ++i) {
                            karabo::util::Hash tmp;
                            std::string token(argv[i]);
                            readToken(token, tmp);
                            configuration.merge(tmp);
                        }

                        // Auto load configuration
                        string autoLoadFileName("autoload.xml");
                        if (boost::filesystem::exists(autoLoadFileName)) {
                            Hash tmp;
                            karabo::io::loadFromFile<Hash>(tmp, autoLoadFileName);
                            configuration.merge(tmp);
                        }

                        karabo::io::saveToFile<Hash>(configuration, "lastConfiguration.xml");
                        return configuration;
                    }
                } catch (const Exception& e) {
                    cout << endl << e.userFriendlyMsg() << endl;
                    return Hash();
                }
            }

            static void processOption(const std::string& option, int argc, char** argv) {
                std::string lowerOption(boost::to_lower_copy(option));
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
                    if (argc > 2) {
                        std::string classId = std::string(argv[2]);
                        classId.substr(0, classId.find_first_of("."));
                        // TODO implement
                    } else {
                        std::cout << "Runner-Version: " << Runner::classInfo().getVersion() << std::endl;
                        std::cout << T::classInfo().getClassName() << "-Version: " << T::classInfo().getVersion() << std::endl << std::endl;
                    }
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
                        path = what.substr(pos+1);
                    }
                    T::getSchema(classId).help(path);
                }
                std::cout << std::endl;
            }

            static void readToken(const std::string& token, karabo::util::Hash & config) {
                using namespace karabo::util;

                boost::filesystem::path possibleFile(token);
                if (boost::filesystem::exists(possibleFile)) {
                    karabo::io::loadFromFile<Hash>(config, possibleFile.string());
                } else {
                    size_t pos = token.find_first_of("=");
                    if (pos == std::string::npos) {
                        config.set(token, Hash());
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
                            config.set(key, Hash());

                            BOOST_FOREACH(std::string subToken, tokens) {
                                boost::trim(subToken);
                                if (!subToken.empty()) {
                                    readToken(subToken, config.get<Hash > (key));
                                }
                            }
                        } else {
                            config.set(key, value);
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
                        << " # Runner-Version: " << Runner::classInfo().getVersion() << "\n"
                        << " # Copyright (C) European XFEL GmbH Hamburg. All rights reserved.\n"
                        << " ##################################################################\n\n";
            }

        };
    }
}

#endif