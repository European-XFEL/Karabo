/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include "UdpNetworking_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(UdpNetworking_Test);

UdpNetworking_Test::UdpNetworking_Test() {
}

UdpNetworking_Test::~UdpNetworking_Test() {
}

void UdpNetworking_Test::setUp() {
}

void UdpNetworking_Test::tearDown() {
}

//////////////////////  UdpServer implementation  //////////////////

namespace karabo {
    namespace net {

        UdpServer::UdpServer() : m_count(0) {
        }

        UdpServer::~UdpServer() {
        }

        void UdpServer::run() {

            // this factory creates connection and, silently, IOService object ...
            m_connection = Connection::create(Hash("Udp.port", 22222, "Udp.type", "server", "Udp.maxlen", 1400));
            IOService::Pointer io(m_connection->getIOService());
            Channel::Pointer channel = m_connection->start(); // Never block for UDP
            channel->setErrorHandler(boost::bind(&UdpServer::errorHandler, this, _1, _2));
            channel->readAsyncVector(boost::bind(&UdpServer::readVectorHandler, this, _1, _2));

            boost::thread ioThread(boost::bind(&IOService::run, io)); // block on io_service

            ioThread.join(); // block here
        }

        void UdpServer::readVectorHandler(Channel::Pointer channel, const vector<char>& data) {
            string s(data.begin(), data.end());
            
            //CppUnit TEST: check content of 's':
            if (m_count == 0) {
                string etalon(80, '5');
                CPPUNIT_ASSERT(s.compare(etalon) == 0);
            } else {
                string etalon(50, '7');
                CPPUNIT_ASSERT(s.compare(etalon) == 0);
            }
            
            m_count++;

            // fill data
            m_data.clear();
            m_data.assign(60, '9');


            //*************************************************************************************
            // NOTE: this 'write' is asynchronous operation and the user should care about 
            // live time of the data, but string and Hash data are copied internally (not a vector)
            // so we do nothing here
            //*************************************************************************************
            channel->writeAsyncVector(m_data, boost::bind(&UdpServer::writeCompleteHandler, this, _1));
        }

        void UdpServer::writeCompleteHandler(Channel::Pointer channel) {
            channel->readAsyncVector(boost::bind(&UdpServer::readVectorHandler, this, _1, _2));

            if (m_count > 5) {
                errorHandler(channel, "Normal server end");
            }
        }

        void UdpServer::errorHandler(Channel::Pointer channel, const string & errmsg) {
            channel->close();
            channel->getConnection()->stop(); // this is just to stop test of server-client communication
        }
    }
}

/////////////////////  UdpClient implementation  ///////////////////////////////

namespace karabo {
    namespace net {

        UdpClient::UdpClient() : m_count(0) {
        }

        UdpClient::~UdpClient() {
        }

        void UdpClient::run() {

            // create connection instance with given parameters
            m_connection = Connection::create(Hash("Udp.hostname", "localhost", "Udp.port", 22222, "Udp.maxlen", 1400));
            IOService::Pointer io = m_connection->getIOService();
            Channel::Pointer channel = m_connection->start(); // Never block for UDP
            channel->setErrorHandler(boost::bind(&UdpClient::errorHandler, this, _1, _2)); // register error handler
            m_data.clear();
            m_data.assign(80, '5'); // fill data
            channel->write(m_data); // synchronous write -- first request to server
            channel->readAsyncVector(boost::bind(&UdpClient::readVectorHandler, this, _1, _2));

            boost::thread ioThread(boost::bind(&IOService::run, io));

            ioThread.join(); // block here

        }

        void UdpClient::errorHandler(Channel::Pointer channel, const string & errmsg) {
            cout << "Error happened -- " << errmsg << ", close connection with this client" << endl;
            channel->close(); // close connection
            // sleep 5 seconds
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
            channel->getConnection()->stop();
        }

        void UdpClient::readVectorHandler(Channel::Pointer channel, const vector<char>& data) {
            string s(data.begin(), data.end());
            
            //CppUnit TEST: check content of 's':
            string etalon(60,'9');
            CPPUNIT_ASSERT(s.compare(etalon) == 0);
            
            // check if we have to stop sending
            if (m_count >= 5) {
                channel->close();
                return;
            }
            // wait a bit (100 milliseconds) to be polite to the server :)
            channel->waitAsync(100, boost::bind(&UdpClient::timerHandler, this, channel));
        }

        void UdpClient::timerHandler(Channel::Pointer channel) {
            // send next message, increase counter
            try {

                // fill data
                m_data.clear();
                m_data.assign(50, '7');

                // write synchronously
                channel->write(m_data);
                m_count++;

                // register read handler
                channel->readAsyncVector(boost::bind(&UdpClient::readVectorHandler, this, _1, _2));
            } catch (...) {
                RETHROW
            }
        }
    }
}

void UdpNetworking_Test::testMethod() {
    
    try {
        
        {
            UdpServer server;
            boost::thread serverThread(boost::bind(&UdpServer::run, &server));
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));

        UdpClient client;
        client.run();

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    } catch (const Exception& e) {
        cout << "Test produced an error:" << endl;
        cout << e.userFriendlyMsg() << endl << endl;
        cout << "Details:" << endl;
        cout << e.detailedMsg();

    } catch (...) {
        RETHROW
    }
}



