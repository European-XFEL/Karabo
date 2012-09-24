/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>
 *
 * Created on November 3, 2011
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

        struct TcpServer {

            TcpServer() : m_count(0) {
            }

            virtual ~TcpServer() {
            }

            void connectHandler(exfel::net::Channel::Pointer channel);
            void readVectorHashHandler(exfel::net::Channel::Pointer channel, const std::vector<char>& data, const exfel::util::Hash & hash);
            void writeCompleteHandler(exfel::net::Channel::Pointer channel);
            void errorHandler(exfel::net::Channel::Pointer channel, const std::string & errmsg);
            void run();

        private:
            int m_count;
            exfel::net::Connection::Pointer m_connection;
            exfel::util::Hash m_hash;
            std::vector<char> m_data;
        };

        struct TcpClient {

            TcpClient() : m_count(0) //,m_timer(0)
            {
            }

            virtual ~TcpClient() {
            }

            void run();
            void errorHandler(exfel::net::Channel::Pointer channel, const std::string & errmsg);
            void readStringHashHandler(exfel::net::Channel::Pointer channel, const std::string& data, const exfel::util::Hash & hash);
            //            void writeCompleteHandler(exfel::net::Channel::Pointer channel);
            void connectHandler(exfel::net::Channel::Pointer channel);
            void timerHandler(exfel::net::Channel::Pointer channel);

        private:
            int m_count;
            exfel::net::Connection::Pointer m_connection;
            //boost::asio::deadline_timer* m_timer;
            exfel::util::Hash m_hash;
            std::string m_data;
        };
    }
}


using namespace std;
using namespace exfel::util;
using namespace exfel::net;

///////////////////////////////////////  TcpServer implementation  ///////////////////////////////////////////

namespace exfel {
    namespace net {
        boost::mutex printMutexTcp;

        static void println(const std::string& str) {
            boost::mutex::scoped_lock lock(printMutexTcp);
            cout << str << endl;
        }

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
            println("TCP SERVER: ioThread started");
            ioThread.join(); // block here
            println("TCP SERVER: ioThread joined");
        }

        void TcpServer::connectHandler(Channel::Pointer channel) {
            channel->setErrorHandler(boost::bind(&TcpServer::errorHandler, this, _1, _2));
            channel->readAsyncVectorHash(boost::bind(&TcpServer::readVectorHashHandler, this, _1, _2, _3));
            println("TCP SERVER: connectHandler");
        }

        void TcpServer::readVectorHashHandler(Channel::Pointer channel, const vector<char>& data, const Hash& hdr) {
            string s(data.begin(), data.end());
            println("\nTCP SERVER:  readHandler: Body ---> " + s);
            //            cout << "Header --->\n" << hdr;
            //            cout << "------------------------" << endl;
            // put business logic of data processing here : do something with read data :)
            // ...

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
            println("TCP SERVER: writeCompleteHandler");
        }

        void TcpServer::errorHandler(Channel::Pointer channel, const string & errmsg) {
            println("TCP SERVER: Error happened -- " + errmsg + ", close connection with this client");
            channel->close();
            //channel->getConnection()->stop(); // this is just to stop test of server-client communication
        }
    }
}

//////////////////////////////////  TcpClient implementation  /////////////////////////////////////////

namespace exfel {
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
            println("TCP CLIENT: ioThread started");
            ioThread.join(); // block here
            //io->run();
            println("TCP CLIENT: ioThread joined");
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
                println("TCP CLIENT: connectHandler");
            } catch (...) {
                RETHROW
            }
        }

        void TcpClient::errorHandler(Channel::Pointer channel, const string & errmsg) {
            println("Error happened -- " + errmsg + ", close connection with this client");
            channel->close(); // close connection
            // sleep 5 seconds
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
            m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1)); // repeat connection attempt

        }

        void TcpClient::readStringHashHandler(Channel::Pointer channel, const string& data, const Hash& hdr) {

            println("TCP CLIENT readStringHashHandler");

            // log data & header
            println("data: " + data);
            stringstream ss;
            ss << hdr;
            println(ss.str());

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
                println("TCP CLIENT: timerHandler");

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

                println("x capacity is " + String::toString(x.capacity()));
                stringstream ss;

                string longMsg("This is a long message to check that vector of char, signed and unsigned char works properly!");
                vector<unsigned char> longV(longMsg.begin(), longMsg.end());
                m_hash.setFromPath("Crate2.Module3.CharImage", longV);

                ss << "About to send -->\n" << m_hash << "--------------------";
                println(ss.str());

                // fill data
                m_data.clear();
                m_data.assign(50, '7');

                // write synchronously
                channel->write(m_data, m_hash);
                m_count++;

                // register read handler
                channel->readAsyncStringHash(boost::bind(&TcpClient::readStringHashHandler, this, _1, _2, _3));
                println("TCP CLIENT: timerHandler:  readAsyncStringHash(readStringHashHandler) registered");
            } catch (...) {
                RETHROW
            }
        }
    }
}

int testTcpNetworking(int argc, char** argv) {
    try {

        Test t;
        TEST_INIT(t, argc, argv);

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
        return EXIT_FAILURE;

    } catch (...) {
        RETHROW
    }

    return EXIT_SUCCESS;
}