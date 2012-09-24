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

#ifndef EXFEL_CORE_RUNNER_HH
#define	EXFEL_CORE_RUNNER_HH

#include <log4cpp/Category.hh>
#include <karabo/util/Factory.hh>
#include <karabo/io/Writer.hh>
#include <karabo/io/Reader.hh>
#include <karabo/log/Logger.hh>

/**
 * The main European XFEL namespace
 */
namespace exfel {

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

            EXFEL_CLASSINFO(Runner, "Runner", "1.0")

            EXFEL_FACTORY_BASE_CLASS

            static typename T::Pointer instantiate(int argc, char** argv) {

                typename T::Pointer instancePointer;

                try {

                    exfel::util::Hash configuration = parseCommandLine(argc, argv);      

                    if (!configuration.empty())
                        instancePointer = T::create(configuration);                    

                    return instancePointer;

                } catch (const exfel::util::Exception& e) {
                    std::cout << e.userFriendlyMsg() << std::endl;
                    return instancePointer;
                }
            }

        private:

            static exfel::util::Hash parseCommandLine(int argc, char** argv) {
                using namespace std;
                using namespace exfel::util;
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
                            exfel::util::Hash c;
                            c.setFromPath("TextFile.filename", possibleFile);
                            exfel::io::Reader<exfel::util::Hash>::Pointer in = exfel::io::Reader<exfel::util::Hash>::create(c);
                            exfel::util::Hash tmp;
                            in->read(tmp);
                            classId = tmp.begin()->first;
                        } else {
                            classId = token.substr(0, token.find_first_of("."));
                        }
                        exfel::util::Schema schema = T::expectedParameters(classId);

                        for (int i = 1; i < argc; ++i) {
                            std::string token(argv[i]);
                            userInputs.push_back(Hash());
                            Hash& user = userInputs.back();
                            readToken(token, user);
                        }
                        
                        // Auto load configuration
                        string autoLoadFile("autoload.xml");
                        if (boost::filesystem::exists(autoLoadFile)) {
                            exfel::util::Hash autoConfig;
                            exfel::io::Reader<exfel::util::Hash>::create("TextFile", Hash("filename", autoLoadFile))->read(autoConfig);
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
                        exfel::util::Schema schema = T::expectedParameters(classId, exfel::util::READ | exfel::util::WRITE | exfel::util::INIT);
                        std::cout << std::endl << "Generating list of expected parameters. Writing output to file: " << classId << ".xsd " << std::endl << std::endl;
                        exfel::io::Writer<exfel::util::Schema>::Pointer out = exfel::io::Writer<exfel::util::Schema>::create(exfel::util::Hash("TextFile.filename", classId + ".xsd"));
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

            static void saveConfiguration(const exfel::util::Hash & config) {
                exfel::io::Writer<exfel::util::Hash>::Pointer out = exfel::io::Writer<exfel::util::Hash>::create(exfel::util::Hash("TextFile.filename", "lastConfiguration.xml"));
                out->write(config);
            }

            static void showUsage(const std::string& programName, const std::string& classId = "") {
                printXfelWelcome();
                std::cout << "Usage: " << programName << " <configuration>\n" << std::endl;
                if (classId.empty()) {
                    std::cout << "The <configuration> reflects a set of (hierarchical) key-value types." << std::endl;
                    std::cout << "and can be given as: \n(1) .xml file\n(2) .libconfig file\n(3) command line input.\n" << std::endl;
                    std::cout << "Example: <key> = a.b, <value> = foo\n(1)\n<a>\n <b>foo</b>\n</a>\n\n";
                    std::cout << "(3) a.b=foo\n\n";
                    std::cout << "Following " << T::classInfo().getClassName() << " <choice>s are availble: " << std::endl;
                    T::help();
                    std::cout << "\nType: '" << programName << " help <choice>' for help on a specific choice" << std::endl;
                    std::cout << "Type: '" << programName << " --create-xsd <choice>' to generate full description of all parameters (in xml schema format)" << std::endl;
                } else {
                    T::help(classId);
                }
                std::cout << std::endl;
            }

            static void readToken(const std::string& token, exfel::util::Hash & config) {
                boost::filesystem::path possibleFile(token);
                if (boost::filesystem::exists(possibleFile)) {
                    exfel::util::Hash c;
                    c.setFromPath("TextFile.filename", possibleFile);
                    exfel::io::Reader<exfel::util::Hash>::Pointer in = exfel::io::Reader<exfel::util::Hash>::create(c);
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
                            config.setFromPath(key, exfel::util::Hash());

                            BOOST_FOREACH(std::string subToken, tokens) {
                                boost::trim(subToken);
                                if (!subToken.empty()) {
                                    readToken(subToken, config.getFromPath<exfel::util::Hash > (key));
                                }
                            }
                        } else {
                            addToConfig(key, value, config);
                        }
                    }
                }
            }

            static void addToConfig(const std::string& key, std::string value, exfel::util::Hash & config) {
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