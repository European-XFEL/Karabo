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

#include <log4cpp/Category.hh>
#include <karabo/util/Factory.hh>
#include <karabo/io/Writer.hh>
#include <karabo/io/Reader.hh>
#include <karabo/log/Logger.hh>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package xip
     */
    namespace core {

        /**
         * The Runner class.
         */
        template <class T>
        class Runner {
        public:

            KARABO_CLASSINFO(Runner, "Runner", "1.0")

            KARABO_FACTORY_BASE_CLASS

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

                        std::vector<Hash> userInputs;
                        std::string classId;
                        std::string token(argv[1]);
                        // Check whether first argument is a file
                        boost::filesystem::path possibleFile(token);
                        if (boost::filesystem::exists(possibleFile)) {
                            karabo::util::Hash c;
                            c.setFromPath("TextFile.filename", possibleFile);
                            karabo::io::Reader<karabo::util::Hash>::Pointer in = karabo::io::Reader<karabo::util::Hash>::create(c);
                            karabo::util::Hash tmp;
                            in->read(tmp);
                            classId = tmp.begin()->first;
                        } else {
                            classId = token.substr(0, token.find_first_of("."));
                        }
                        karabo::util::Schema schema = T::expectedParameters(classId);

                        for (int i = 1; i < argc; ++i) {
                            std::string token(argv[i]);
                            userInputs.push_back(Hash());
                            Hash& user = userInputs.back();
                            readToken(token, user);
                        }
                        
                        // Auto load configuration
                        string autoLoadFile("autoload.xml");
                        if (boost::filesystem::exists(autoLoadFile)) {
                            karabo::util::Hash autoConfig;
                            karabo::io::Reader<karabo::util::Hash>::create("TextFile", Hash("filename", autoLoadFile))->read(autoConfig);
                            userInputs.push_back(autoConfig);
                        }

                        Hash working = schema.mergeUserInput(userInputs);
                        saveConfiguration(working);
                        return working;
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
                        karabo::util::Schema schema = T::expectedParameters(classId, karabo::util::READ | karabo::util::WRITE | karabo::util::INIT);
                        std::cout << std::endl << "Generating list of expected parameters. Writing output to file: " << classId << ".xsd " << std::endl << std::endl;
                        karabo::io::Writer<karabo::util::Schema>::Pointer out = karabo::io::Writer<karabo::util::Schema>::create(karabo::util::Hash("TextFile.filename", classId + ".xsd"));
                        out->write(schema);
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
                } else {
                    showUsage(std::string(argv[0]));
                }
            }

            static void saveConfiguration(const karabo::util::Hash & config) {
                karabo::io::Writer<karabo::util::Hash>::Pointer out = karabo::io::Writer<karabo::util::Hash>::create(karabo::util::Hash("TextFile.filename", "lastConfiguration.xml"));
                out->write(config);
            }

            static void showUsage(const std::string& programName, const std::string& classId = "") {
                printXfelWelcome();
                std::cout << "Usage: " << programName << " <configuration>\n" << std::endl;
                std::string runnableType = T::classInfo().getClassName();
                if (classId.empty()) {
                    std::cout << "The <configuration> reflects a set of (hierarchical) key-value types." << std::endl;
                    std::cout << "You can supply <configuration> information as xml file or as command-line input or a combination of both.\n" << std::endl;
                    std::cout << "Example:\nAssume the key \"" << runnableType << ".someThreshold\" and a corresponding value \"4.2\".\nThe corresponding xml file should look like this:\n" << std::endl;
                    std::cout << "  \"<" << runnableType << "><someThreshold>4.2</someThreshold></" << runnableType << ">\"\n\nIf you saved the file under \"config.xml\" you should then type:\n  '" << programName << " config.xml'\n\n\n";
                    std::cout << "For the same configuration given as command line arguments you should type:\n  '" << programName << " " << runnableType << ".someThreshold=\"4.2\"'\n\n";
                    std::cout << "Following " << T::classInfo().getClassName() << " <choice>s are available: " << std::endl;
                    T::help();
                    std::cout << "\nType: '" << programName << " help <choice>' for help on a specific choice" << std::endl;
                    std::cout << "Type: '" << programName << " --create-xsd <choice>' to generate full description of all parameters (in xml schema format)" << std::endl;
                } else {
                    T::help(classId);
                }
                std::cout << std::endl;
            }

            static void readToken(const std::string& token, karabo::util::Hash & config) {
                boost::filesystem::path possibleFile(token);
                if (boost::filesystem::exists(possibleFile)) {
                    karabo::util::Hash c;
                    c.setFromPath("TextFile.filename", possibleFile);
                    karabo::io::Reader<karabo::util::Hash>::Pointer in = karabo::io::Reader<karabo::util::Hash>::create(c);
                    in->read(config);
                } else {
                    size_t pos = token.find_first_of("=");
                    if (pos == std::string::npos) {
                        config.setFromPath(token);
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
                            config.setFromPath(key, karabo::util::Hash());

                            BOOST_FOREACH(std::string subToken, tokens) {
                                boost::trim(subToken);
                                if (!subToken.empty()) {
                                    readToken(subToken, config.getFromPath<karabo::util::Hash > (key));
                                }
                            }
                        } else {
                            addToConfig(key, value, config);
                        }
                    }
                }
            }

            static void addToConfig(const std::string& key, std::string value, karabo::util::Hash & config) {
                boost::trim(value);
                if (value == "") {
                    config.setFromPath(key);
                }
                // Check for array input
                std::vector<std::string> items;
                boost::split(items, value, boost::is_any_of(","));
                if (items.size() >= 2) {
                    config.setFromPath(key, items);
                } else {
                    config.setFromPath(key, value);
                }
            }

            static void printXfelWelcome() {
                std::string runnableType = T::classInfo().getClassName();
                std::cout << "\n ##################################################################\n"
                        << "                 Simple XFEL " << runnableType << " Runner\n"
                        << " #                                                                #\n"
                        << " # Copyright (C) European XFEL GmbH Hamburg. All rights reserved. #\n"
                        << " ##################################################################\n\n";
            }

        };
    }
}

#endif