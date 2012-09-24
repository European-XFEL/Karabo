/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>
 *
 * Created on November 8, 2011
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "../IOService.hh"
#include "../Connection.hh"
#include "../Channel.hh"


namespace exfel {
    namespace net {

        struct UdpServer {
            UdpServer();
            virtual ~UdpServer();
            void readVectorHandler(exfel::net::Channel::Pointer channel, const std::vector<char>& data);
            void writeCompleteHandler(exfel::net::Channel::Pointer channel);
            void errorHandler(exfel::net::Channel::Pointer channel, const std::string & errmsg);
            void run();

        private:
            int m_count;
            exfel::net::Connection::Pointer m_connection;
            std::vector<char> m_data;
        };

        struct UdpClient {

            UdpClient();
            virtual ~UdpClient();
            void run();
            void errorHandler(exfel::net::Channel::Pointer channel, const std::string & errmsg);
            void readVectorHandler(exfel::net::Channel::Pointer channel, const std::vector<char>& data);
            //            void writeCompleteHandler(exfel::net::Channel::Pointer channel);
            void timerHandler(exfel::net::Channel::Pointer channel);

        private:
            int m_count;
            exfel::net::Connection::Pointer m_connection;
            //boost::asio::deadline_timer* m_timer;
            std::vector<char> m_data;
        };
    }
}


using namespace std;
using namespace exfel::util;
using namespace exfel::net;

///////////////////////////////////////  UdpServer implementation  ///////////////////////////////////////////

namespace exfel {
    namespace net {

        boost::mutex printMutexUdp;

        static void println(const std::string& str) {
            boost::mutex::scoped_lock lock(printMutexUdp);
            cout << str << endl;
        }

        UdpServer::UdpServer() : m_count(0) {
        }

        UdpServer::~UdpServer() {
        }

        void UdpServer::run() {
            cout << "UDP SERVER: run()" << endl;
            // this factory crates connection and, silently, IOService object ...
            m_connection = Connection::create(Hash("Udp.port", 22222, "Udp.type", "server", "Udp.maxlen", 1400));
            IOService::Pointer io(m_connection->getIOService());
            Channel::Pointer channel = m_connection->start(); // Never block for UDP
            channel->setErrorHandler(boost::bind(&UdpServer::errorHandler, this, _1, _2));
            channel->readAsyncVector(boost::bind(&UdpServer::readVectorHandler, this, _1, _2));

            boost::thread ioThread(boost::bind(&IOService::run, io));  // block on io_service

            println("UDP SERVER: ioThread started");
            ioThread.join(); // block here
            println("UDP SERVER: ioThread joined");
        }

        void UdpServer::readVectorHandler(Channel::Pointer channel, const vector<char>& data) {
            string s(data.begin(), data.end());
            println("\nUDP SERVER:  readHandler: Body ---> " + s);
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
            println("UDP SERVER: writeCompleteHandler");
            if (m_count > 5) {
                errorHandler(channel,"Normal server end");
            }
        }

        void UdpServer::errorHandler(Channel::Pointer channel, const string & errmsg) {
            println("UDP SERVER: Error happened -- " + errmsg + ", close connection with this client");
            channel->close();
            channel->getConnection()->stop(); // this is just to stop test of server-client communication
        }
    }
}

//////////////////////////////////  UdpClient implementation  /////////////////////////////////////////

namespace exfel {
    namespace net {

        UdpClient::UdpClient() : m_count(0) //,m_timer(0)
        {
        }

        UdpClient::~UdpClient() {
        }

        void UdpClient::run() {
            cout << "UDP CLIENT: run()" << endl;
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

            println("UDP CLIENT: ioThread started");
            ioThread.join(); // block here
            println("UDP CLIENT: ioThread joined");
        }

        void UdpClient::errorHandler(Channel::Pointer channel, const string & errmsg) {
            println("Error happened -- " + errmsg + ", close connection with this client");
            channel->close(); // close connection
            // sleep 5 seconds
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
            channel->getConnection()->stop();
        }

        void UdpClient::readVectorHandler(Channel::Pointer channel, const vector<char>& data) {
            string s(data.begin(), data.end());
            println("UDP CLIENT::readVectorHandler  --- data: " + s);

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
                println("UDP CLIENT: timerHandler");

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

int testUdpNetworking(int argc, char** argv) {
    try {

        Test t;
        TEST_INIT(t, argc, argv);

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
        return EXIT_FAILURE;

    } catch (...) {
        RETHROW
    }

    return EXIT_SUCCESS;
}
