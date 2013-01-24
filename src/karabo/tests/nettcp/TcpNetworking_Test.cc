/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include "TcpNetworking_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(TcpNetworking_Test);

TcpNetworking_Test::TcpNetworking_Test() {
}

TcpNetworking_Test::~TcpNetworking_Test() {
}

void TcpNetworking_Test::setUp() {
}

void TcpNetworking_Test::tearDown() {
}

//////////////  TcpServer implementation  /////////////////////////

namespace karabo {
    namespace net {

        void TcpServer::run() {
            // this factory crates connection and, silently, IOService object ...
            m_connection = Connection::create(Hash(
                    "Tcp.port", 22222,
                    "Tcp.type", "server",
                    "Tcp.sizeofLength", 2,
                    "Tcp.hashSerialization.Xml.printDataType", true)); // create a connection
            m_connection->startAsync(boost::bind(&TcpServer::connectHandler, this, _1)); // ... and start it (connect!) asynchronously

            // Usually, we might write here simply ...
            // m_connection->getIOService()->run();  // block here
            // However, start io_service::run method in other thread, just for FUN!
            // All the handlers will be called in that thread ...
            boost::thread ioThread(boost::bind(&IOService::run, m_connection->getIOService()));

            ioThread.join(); // block here

        }

        void TcpServer::connectHandler(Channel::Pointer channel) {
            channel->setErrorHandler(boost::bind(&TcpServer::errorHandler, this, _1, _2));
            channel->readAsyncVectorHash(boost::bind(&TcpServer::readVectorHashHandler, this, _1, _2, _3));

        }

        void TcpServer::readVectorHashHandler(Channel::Pointer channel, const vector<char>& data, const Hash& hdr) {
            string s(data.begin(), data.end());                     
            // put business logic of data processing here : do something with read data :)
            // ...
            
            //CppUnit TEST: check content of 's':
            if (m_count == 0) {
                string etalon(80, '5');
                CPPUNIT_ASSERT(s.compare(etalon) == 0);
                CPPUNIT_ASSERT(hdr.getFromPath<string>("Crate2.Module3.Administrator") == "QuestionMark");
            } else { 
                string etalon(50, '7');
                CPPUNIT_ASSERT(s.compare(etalon) == 0);
            }
                        
            // Prepare the answer to the client
            m_count++;

            // fill header
            m_hash.clear();

            if (!hdr.empty()) {
                m_hash.append(hdr);
                if (m_hash.has("Crate2") && m_hash.getFromPath<string > ("Crate2.Module3.Administrator") == "QuestionMark")
                    m_hash.setFromPath("Crate2.Module3.Administrator", "C.Youngman");
            } else
                m_hash.setFromPath("Crate2.Module3.TechDirector", "APPROVED!");

            // fill data
            m_data.clear();
            m_data.assign(60, '9');


            //*************************************************************************************
            // NOTE: this 'write' is asynchronous operation and the user should care about 
            // live time of the data, but string and Hash data are copied internally (not a vector)
            // so we do nothing here
            //*************************************************************************************
            channel->writeAsyncVectorHash(m_data, m_hash, boost::bind(&TcpServer::writeCompleteHandler, this, _1));
        }

        void TcpServer::writeCompleteHandler(Channel::Pointer channel) {
            channel->readAsyncVectorHash(boost::bind(&TcpServer::readVectorHashHandler, this, _1, _2, _3));
        }

        void TcpServer::errorHandler(Channel::Pointer channel, const string & errmsg) {
            channel->close();
        }
    }
}

//////////////////  TcpClient implementation  ///////////////////////////////

namespace karabo {
    namespace net {

        void TcpClient::run() {
            // create connection instance with given parameters
            m_connection = Connection::create(Hash(
                    "Tcp.hostname", "localhost",
                    "Tcp.port", 22222,
                    "Tcp.sizeofLength", 2,
                    "Tcp.hashSerialization.Xml.printDataType", true));
            IOService::Pointer io = m_connection->getIOService();
            m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1));
            boost::thread ioThread(boost::bind(&IOService::run, io));

            ioThread.join(); // block here

        }

        void TcpClient::connectHandler(Channel::Pointer channel) {

            try {
                // register error handler
                channel->setErrorHandler(boost::bind(&TcpClient::errorHandler, this, _1, _2));
                // fill header
                m_hash.clear();
                m_hash.setFromPath("Crate2.Module3.Administrator", "QuestionMark");

                // fill data
                m_data.clear();
                m_data.assign(80, '5');

                // synchronous write
                channel->write(m_data, m_hash);

                // register read handler
                channel->readAsyncStringHash(boost::bind(&TcpClient::readStringHashHandler, this, _1, _2, _3));

            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpClient::errorHandler(Channel::Pointer channel, const string & errmsg) {

            channel->close(); // close connection
            // sleep 5 seconds
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
            m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1)); // repeat connection attempt

        }

        void TcpClient::readStringHashHandler(Channel::Pointer channel, const string& data, const Hash& hdr) {

            //uncomment to see 'data' and 'header':
            //cout<<"\n data: \n" << data << endl;
            //cout << "hdr: \n" << hdr << endl;
            
            //CppUnit TEST: check content of 'data':
            string etalon(60,'9');
            CPPUNIT_ASSERT(data.compare(etalon) == 0);

            //CppUnit TEST: check content of header:
            if (m_count == 0) {
                CPPUNIT_ASSERT(hdr.getFromPath<string>("Crate2.Module3.Administrator") == "C.Youngman");
            } else {
                CPPUNIT_ASSERT(hdr.getFromPath<string>("Crate2.Module3.Administrator") == "N.Coppola");
                CPPUNIT_ASSERT(hdr.getFromPath<float>("Crate2.Module3.Channel0.Voltage") == 201.500000);
                CPPUNIT_ASSERT(hdr.getFromPath<string>("Crate2.Module3.Location") == "Located AER19, room 2-21, rack 4");
            }
            
            // check if we have to stop sending
            if (m_count >= 5) {
                channel->close();
                return;
            }
            // wait a bit to be polite to the server :)
            channel->waitAsync(100, boost::bind(&TcpClient::timerHandler, this, channel));
        }

        void TcpClient::timerHandler(Channel::Pointer channel) {
            // send next message, increase counter
            try {

                // fill header
                vector<short> v;
                m_hash.clear();
                m_hash.setFromPath("Crate2.Module3.Administrator", "N.Coppola");
                m_hash.setFromPath("Crate2.Module3.Location", "Located AER19, room 2-21, rack 4");
                m_hash.setFromPath("Crate2.Module3.Channel0.Voltage", 201.5f);
                m_hash.setFromPath("Crate2.Module3.Channel0.RampUp", 20.3f);
                m_hash.setFromPath("Crate2.Module3.Channel0.RampDown", 22.2f);
                m_hash.setFromPath("Crate2.Module3.Channel1.Voltage", 15.0f);
                m_hash.setFromPath("Crate2.Module3.Channel1.RampUp", 30.3f);
                m_hash.setFromPath("Crate2.Module3.Channel1.RampDown", 30.3f);
                m_hash.setFromPath("Crate2.Module3.Channel2.Voltage", 70.0f);
                m_hash.setFromPath("Crate2.Module3.Channel2.RampUp", 10.0f);
                m_hash.setFromPath("Crate2.Module3.Channel2.RampDown", 10.0f);
                m_hash.setFromPath("Crate2.Module3.Image", v);
                vector<short>& x = m_hash.getFromPath<vector<short> >("Crate2.Module3.Image");
                x.push_back(12);
                x.push_back(42);
                x.push_back(77);
                x.push_back(101);
                x.push_back(-3);
                x.push_back(-101);
                x.push_back(0);
                for (short i = 0; i < 10; i++) {
                    x.push_back(i);
                }

                CPPUNIT_ASSERT(x.capacity() == 32);

                string longMsg("This is a long message to check that vector of char, signed and unsigned char works properly!");
                vector<unsigned char> longV(longMsg.begin(), longMsg.end());
                m_hash.setFromPath("Crate2.Module3.CharImage", longV);

                // fill data
                m_data.clear();
                m_data.assign(50, '7');

                // write synchronously
                channel->write(m_data, m_hash);
                m_count++;

                // register read handler
                channel->readAsyncStringHash(boost::bind(&TcpClient::readStringHashHandler, this, _1, _2, _3));
                
            } catch (...) {
                KARABO_RETHROW
            }
        }
    }
}

void TcpNetworking_Test::testMethod() {
    try {

        // Create server object and run it in different thread
        TcpServer server;
        boost::thread serverThread(boost::bind(&TcpServer::run, &server));

        // suspend main thread to give a chance for server thread really to start
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));

        // create client object and run it in main thread
        TcpClient client;
        client.run();

        // when client id done the main thread is waiting for server thread to join
        serverThread.join();

    } catch (const Exception& e) {
        cout << "Test produced an error:" << endl;
        cout << e.userFriendlyMsg() << endl << endl;
        cout << "Details:" << endl;
        cout << e.detailedMsg();

    } catch (...) {
        KARABO_RETHROW
    }
}



