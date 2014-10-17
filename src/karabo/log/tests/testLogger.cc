//
// $Id: testLogger.cc 4878 2011-12-15 14:19:14Z irinak@DESY.DE $
//
// Copyright (C) 2009 European XFEL GmbH Hamburg. All rights reserved.
//
// Author: <krzysztof.wrona@xfel.eu>
//
//

#include <stdlib.h>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <krb_log4cpp/OstreamAppender.hh>
#include <krb_log4cpp/Category.hh>
#include <krb_log4cpp/Priority.hh>
#include <exfel/util/Hash.hh>
#include <exfel/io/Writer.hh>
#include <exfel/io/Reader.hh>
#include <cassert>
#include "../Logger.hh"
#include "exfel/util/Test.hh"

/*
 * 
 */
int testLogger(int argc, char** argv) {


  using namespace std;
  using namespace exfel;
  using namespace exfel::util;
  using namespace exfel::io;
  using namespace exfel::log;
  using boost::shared_ptr;
  using namespace boost::filesystem;
  using namespace krb_log4cpp;

  Test t;
  TEST_INIT(t,argc,argv);

  try {

    cout << t << endl;
    
    Schema expected = Logger::expectedParameters("Logger");

    // not yet fully implemented
    {
      Hash input;
      input.setFromPath("TextFile.filename", t.file("expected.xsd"));
      input.setFromPath("TextFile.format.Xsd");
      Writer<Schema>::Pointer out = Writer<Schema>::create(input);
      out->write(expected);
    }


    Hash input;
    Hash inputXml;
    Hash inputLibConfig;

    path inXmlFile(t.file("input.xml"));
    path inFile(t.file("input.conf"));

    if (exists(inXmlFile)) {
      Hash rinput;
      rinput.setFromPath("TextFile.filename", inXmlFile.string());
      Reader<Hash>::Pointer in = Reader<Hash>::create(rinput);
      in->read(input);
      cout << "using configuration file " << inXmlFile.string() << endl;
    } else if (exists(inFile)) {
      Hash rinput;
      rinput.setFromPath("TextFile.filename", inFile.string());
      rinput.setFromPath("TextFile.format.LibConfig");
      Reader<Hash>::Pointer in = Reader<Hash>::create(rinput);
      in->read(input);
      cout << "using configuration file " << inFile.string() << endl;
    } else {

      cout << "using program default configuration (hard coded) " << endl;
      input.setFromPath("Logger.categories[0].Category.name", "exfel");
      input.setFromPath("Logger.categories[0].Category.priority", "WARN");

      input.setFromPath("Logger.categories[1].Category.name", "exfel.io");
      input.setFromPath("Logger.categories[1].Category.priority", "DEBUG");
      input.setFromPath("Logger.categories[1].Category.additivity", false);

      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.name", "file");
      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.layout.Pattern");
      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.filename", "message.log");
      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.maxSize", (unsigned int) 10);
      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.maxSizeUnit", "kB");
      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.maxBackupIndex", (unsigned short) 3);
      input.setFromPath("Logger.categories[1].Category.appenders[0].RollingFile.append", true);
      input.setFromPath("Logger.categories[1].Category.appenders[1].File.name", "trala");
      //input.setFromPath("Logger.categories[1].Category.appenders[1].File.filename", "/tmp/mytest.log");
      input.setFromPath("Logger.categories[1].Category.appenders[1].File.layout.Pattern");
      //input.setFromPath("Logger.categories[1].Category.appenders[1].Ostream.name", "DWADWA");
      input.setFromPath("Logger.categories[1].Category.appenders[2].Ostream.name", "stderr");
      input.setFromPath("Logger.categories[1].Category.appenders[2].Ostream.layout.Pattern");
      
      //input.setFromPath("Logger.categories[1].Category.appenders[3].Network.name","networkLog");
      //input.setFromPath("Logger.categories[1].Category.appenders[3].Network.layout.Simple");
      //input.setFromPath("Logger.categories[1].Category.appenders[3].Network.connection.Jms.destinationName", "DestName");

      //input.setFromPath("Logger.appenders[0].Ostream.name", "stdout");
      //input.setFromPath("Logger.appenders[0].Ostream.layout.Simple");
      //input.setFromPath("Logger.appenders[0].Ostream.layout.Basic");
      //input.setFromPath("Logger.appenders[0].Ostream.layout.Pattern.pattern","%d %c %p %m %n");
      input.setFromPath("Logger.appenders[0].Ostream.layout.Pattern.pattern", "%d %-6c [%-5p] %m %n");
      //Logger.priority
    }

    {
      Hash wrinput;
      wrinput.setFromPath("TextFile.filename", t.file("input-saved.conf"));
      wrinput.setFromPath("TextFile.format.LibConfig");
      Writer<Hash>::Pointer out = Writer<Hash>::create(wrinput);
      out->write(input);
    }

    {
      Hash wrinput;
      wrinput.setFromPath("TextFile.filename", t.file("input-saved.xml"));
      Writer<Hash>::Pointer out = Writer<Hash>::create(wrinput);
      out->write(input);
    }




    //input.setFromPath("OstreamAppenderLog4cpp.treshold","WARN");
    //input.setFromPath("OstreamAppenderConfigurator.output","STDOUT");
    //input.setFromPath("Logger.appender.OstreamAppender.pattern", "DEFAULT");

    //  input.setFromPath("FileAppenderConfigurator.name", "file");
    //  input.setFromPath("FileAppenderConfigurator.treshold", "WARN");
    //  input.setFromPath("FileAppenderConfigurator.filename", "app.log");

    cout << input << endl;


    Logger::Pointer logP = Logger::create(input);
    logP->initialize();
    
    //assert(0 == 1);

    Category& log = Category::getInstance("exfel");
    //  cout << "appenderName = " << log.getAppender()->getName() << endl;

    log << Priority::DEBUG << "This is DEBUG message - must be suppressed";
    log << Priority::INFO << "This is INFO message - must be suppressed";
    log << Priority::WARN << "This is WARN message";
    log << Priority::ERROR << "This is an ERROR message";


    assert(log.getName() == "exfel");
    assert(log.getAdditivity() == true);
    assert(log.getChainedPriority() == Priority::WARN);
    assert(log.getAllAppenders().size() == 0);
    assert(log.getParent()->getName() == "");

    Category& log1 = Category::getInstance("exfel.io");
    log1 << Priority::DEBUG << "log1 says This is a debug message";
    log1 << Priority::INFO << "log1 says This is a info message";
    log1 << Priority::WARN << "log1 says This is a warn message";
    log1 << Priority::ERROR << "log1 says This is a error message";

    assert(log1.getName() == "exfel.io");
    assert(log1.getAdditivity() == false);
    assert(log1.getChainedPriority() == Priority::DEBUG);
    assert(log1.getAllAppenders().size() == 3);
    assert(log1.getParent()->getName() == "exfel");
    assert(log1.getParent()->getParent()->getName() == "");

    Category& log2 = Category::getInstance("");
    log2 << Priority::DEBUG << "log2 says This is a debug message";
    log2 << Priority::INFO << "log2 says This is a info message";
    log2 << Priority::WARN << "log2 says This is a warn message";
    log2 << Priority::ERROR << "log2 says This is a error message";

    assert(log2.getName() == "");
    assert(log2.getAdditivity() == true);
    assert(log2.getChainedPriority() == Priority::INFO);
    assert(log2.getAllAppenders().size() == 1);
    assert(log2.getParent() == NULL);

    log << Priority::ERROR << "log " << log.getName() << " " << log.getPriority();
    log1 << Priority::ERROR << "log1 " << log1.getName() << " " << log1.getPriority();
    log2 << Priority::ERROR << "log2 " << log2.getName() << " " << log2.getPriority();


    Category::shutdown();

    {
       cout << "\n>>>>>> TEST help() function <<<<<<<<" << endl;
       //exfel::util::Schema expLog=exfel::log::Logger::expectedParameters();
       //cout<< "expLog:\n"<< expLog <<endl;
        Logger::help();
        Logger::help("Logger");
        Logger::help("Logger.appenders");
        Logger::help("Logger.appenders.File");
        Logger::help("Logger.appenders.File.filename");
        Logger::help("Logger.appenders.File.layout");
        Logger::help("Logger.appenders.File.layout.Pattern");
        Logger::help("Logger.appenders.File.layout.Pattern.pattern");

        Logger::help("Logger.appenders.Ostream");

        Logger::help("Logger.categories");
        Logger::help("Logger.categories.Category");
        Logger::help("Logger.categories.Category.appenders");
        Logger::help("Logger.categories.Category.appenders.RollingFile");
        Logger::help("Logger.categories.Category.appenders.RollingFile.layout");
        cout << "\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
     }
            

  } catch (const exfel::util::Exception& e) {
    std::cout << e;
    RETHROW
  }

  return (EXIT_SUCCESS);
}

