/*
 * $Id: Application.cc 5114 2012-02-13 09:37:51Z wrona $
 *
 * File:   Application.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 24, 2010, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <exfel/io/Reader.hh>
#include <exfel/io/Writer.hh>
#include <exfel/log/Logger.hh>

#include <log4cpp/Category.hh>


#include "Application.hh"


namespace exfel {
    namespace core {

        EXFEL_REGISTER_ONLY_ME_CC(Application)

        using namespace std;
        using namespace exfel::util;
        using namespace exfel::io;
        using namespace exfel::log;
        using namespace log4cpp;


        int Application::runModules(int argc, char** argv) {

            try {
                Hash working = Application::parseCommandLine(argc, argv);

                if (working.empty()) {
                    working.set("Application", Hash());
                }
                Application::Pointer app = Application::create(working);
                app->run();

            } catch (const Exception& e) {
                cout << e << endl;
                return (EXIT_FAILURE);
            }
            return (EXIT_SUCCESS);
        }


        Hash Application::parseCommandLine(int argc, char** argv) {
            //Hash user;
            Schema master = Application::expectedParameters("Application");
            vector<Hash> userInputs;
            try {
                if (argc == 1) {
                    showUsage();
                    return Hash();
                }
                for (int i = 1; i < argc; ++i) {
                    string token(argv[i]);
                    if (token.substr(0, 2) == "--") {
                        processOption(token.substr(2), i);
                    } else {
                        userInputs.push_back(Hash());
                        Hash& user = userInputs.back();
                        readToken(token, user);
                    }
                }
            } catch (const Exception& e) {
                cout << endl << e << endl;
            }

            // Save the working configuration to file
            Hash working = master.mergeUserInput(userInputs);
            Writer<Hash>::Pointer out = Writer<Hash>::create(Hash("TextFile.filename", "lastConfiguration.xml"));
            out->write(working);
            return working;
        }


        void Application::readToken(const string& token, Hash& config) {
            boost::filesystem::path possibleFile(token);
            if (boost::filesystem::exists(possibleFile)) {
                Hash c;
                c.setFromPath("TextFile.filename", possibleFile);
                Reader<Hash>::Pointer in = Reader<Hash>::create(c);
                in->read(config);
            } else {
                size_t pos = token.find_first_of("=");
                if (pos == string::npos) {
                    config.setFromPath(token);
                } else {
                    string key(token.substr(0, pos));
                    boost::trim(key);
                    string value(token.substr(pos + 1));
                    boost::trim(value);
                    if (!value.empty() && (*value.begin()) == '{' && (*value.rbegin()) == '}') {
                        value = value.substr(1, value.size() - 2);
                        boost::trim(value);
                        vector<string> tokens;
                        boost::split(tokens, value, boost::is_any_of(" "));
                        config.setFromPath(key, Hash());


                        BOOST_FOREACH(string subToken, tokens) {
                            boost::trim(subToken);
                            if (!subToken.empty()) {
                                readToken(subToken, config.getFromPath<Hash > (key));
                            }
                        }
                    } else {
                        addToConfig(key, value, config);
                    }
                }
            }
        }


        void Application::expectedParameters(Schema& expected) {

            // Logger
            SINGLE_ELEMENT<Logger > (expected).key("Logger")
                    .displayedName("Logger")
                    .description("Log configuration")
                    .assignmentOptional().defaultValue("Logger")
                    .commit();

            // modules
            NON_EMPTY_LIST_ELEMENT<Module>(expected).key("modules")
                    .displayedName("Modules")
                    .description("The modules to be executed")
                    .assignmentMandatory()
                    .commit();

        }


        void Application::configure(const exfel::util::Hash& input) {
            loadLogger(input);
            loadModules(input);
        }


        void Application::loadLogger(const Hash& input) {
            Logger::Pointer log = Logger::createSingle("Logger", "Logger", input);
            log->initialize();
        }


        void Application::loadModules(const Hash& input) {
            m_modules = Module::createList("modules", input);
        }


        void Application::run() const {
            Category& log = Category::getInstance("exfel.core.Application");
            log << Priority::INFO << "Starting European XFEL Application";
            log << Priority::INFO << "Executing modules...";
            for (size_t i = 0; i < m_modules.size(); ++i) {
                log << Priority::INFO << "Running module[" << i << "]: " << m_modules[i]->getName();
                m_modules[i]->compute();
                log << Priority::INFO << "Module[" << i << "] " << m_modules[i]->getName() << " finished";

            }
        }


        void Application::showUsage() {
            cout << endl;
            cout << "Usage: Up to now you have to ask either BH or KW" << endl;
            cout << " --show-expected - generates list of expected parameters" << endl;
            cout << endl;
        }


        void Application::addToConfig(const string& key, string value, Hash& config) {
            boost::trim(value);
            if (value == "") {
                config.setFromPath(key);
            }
            // Check for array input
            vector<string> items;
            boost::split(items, value, boost::is_any_of(","));
            if (items.size() >= 2) {
                config.setFromPath(key, items);
            } else {
                config.setFromPath(key, value);
            }
        }


        void Application::processOption(const string& option, int& argc) {
            string lowerOption(option);
            boost::to_lower(lowerOption);
            if (lowerOption == "show-expected") {
                cout << endl << "Generating list of expected parameters. Output written to file: expected.xsd " << endl;
                Writer<Hash>::Pointer out = Writer<Hash>::create(Hash("TextFile.filename", "expected.xsd"));
                out->write(Application::expectedParameters("Application"));
                exit(0);
            } else if (lowerOption == "help") {
                showUsage();
                exit(0);
            } else if (lowerOption == "show-expected-xml") {
            } else if (lowerOption == "show-expected-libConfig") {
            }
        }
    } // namespace core
} // namespace exfel
