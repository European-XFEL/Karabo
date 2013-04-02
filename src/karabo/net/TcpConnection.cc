#include <boost/any.hpp>
#include <boost/pointer_cast.hpp>
#include "IOService.hh"
#include "TcpConnection.hh"
#include "TcpChannel.hh"
#include "AsioIOService.hh"

using namespace std;
using namespace boost::asio;
using namespace karabo::util;

namespace karabo {
    namespace net {


        KARABO_REGISTER_FOR_CONFIGURATION(Connection, TcpConnection)

        void TcpConnection::expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                    .key("type")
                    .displayedName("Connection Type")
                    .description("Decide whether the connection is used to implement a TCP Server or TCP Client")
                    .assignmentOptional().defaultValue("client")
                    .options("server,client")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("hostname")
                    .displayedName("Hostname")
                    .description("Hostname of a peer (used only for client)")
                    .assignmentOptional().defaultValue("localhost")
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Hostport of a peer for type 'client' and local port for type 'server'")
                    .assignmentOptional().defaultValue(11111)
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("sizeofLength")
                    .displayedName("Size of Message Length")
                    .description("The size of messageLength field in communication protocol")
                    .assignmentOptional().defaultValue(4)
                    .init()
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("messageTagIsText")
                    .displayedName("Message Tag is Text")
                    .description("The length field in communication protocol is considered as text string")
                    .assignmentOptional().defaultValue(false)
                    .init()
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("manageAsyncData")
                    .displayedName("Manage Async Data")
                    .description("If set to true, asynchronous write handlers will copy the data to be written. The user does not "
                    "have to make sure that the to-be-written data has a long-enough life time.")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .advanced()
                    .commit();
        }


        TcpConnection::TcpConnection(const karabo::util::Hash& input) : Connection(input) {

            this->setIOServiceType("Asio");

            // Get the specific boost::asio::io_service object
            m_boostIoServicePointer = getIOService()->castTo<AsioIOService > ()->getBoostIOService();

            input.get("hostname", m_hostname);
            input.get("port", m_port);
            input.get("type", m_connectionType);
            input.get("sizeofLength", m_sizeofLength);
            input.get("messageTagIsText", m_lengthIsTextFlag);
            input.get("manageAsyncData", m_manageAsyncData);


            if (m_connectionType == "server") {
                BoostTcpAcceptor acceptor(new ip::tcp::acceptor(*m_boostIoServicePointer));
                m_acceptor = acceptor;
                ip::tcp::endpoint endpoint(ip::tcp::v4(), m_port);
                m_acceptor->open(endpoint.protocol());
                m_acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
                m_acceptor->bind(endpoint);
                m_acceptor->listen();
            } else {
                BoostTcpResolver resolv(new ip::tcp::resolver(*m_boostIoServicePointer));
                m_resolver = resolv;
            }
        }


        Channel::Pointer TcpConnection::start() {
            m_isAsyncConnect = false;
            if (m_connectionType == "server")
                return startServer();
            return startClient();
        }


        Channel::Pointer TcpConnection::startServer() {
            Channel::Pointer new_channel;
            try {
                new_channel = this->createChannel();
                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                m_acceptor->accept(ch->socket());
                //cout << "New incoming connect: " << ch->socket().remote_endpoint().address() << " : " << ch->socket().remote_endpoint().port() << endl;
            } catch (...) {
                KARABO_RETHROW
            }
            return new_channel;
        }


        Channel::Pointer TcpConnection::startClient() {
            Channel::Pointer new_channel;
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::util::toString(m_port));
                ip::tcp::resolver::iterator endpoint_iterator = m_resolver->resolve(query);
                new_channel = this->createChannel();
                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                ch->socket().connect(*endpoint_iterator);
                //cout << "Successfully connected to: " << ch->socket().remote_endpoint().address() << " : " << ch->socket().remote_endpoint().port() << endl;
            } catch (...) {
                KARABO_RETHROW
            }
            return new_channel;
        }


        void TcpConnection::startAsync(const ConnectionHandler& handler) {
            m_isAsyncConnect = true;
            if (m_connectionType == "server")
                startServer(handler);
            else if (m_connectionType == "client")
                startClient(handler);
        }


        void TcpConnection::startServer(const ConnectionHandler& handler) {
            try {
                Channel::Pointer new_channel = this->createChannel();
                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                m_acceptor->async_accept(ch->socket(), boost::bind(&TcpConnection::acceptHandler,
                                                                   this, new_channel, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::acceptHandler(Channel::Pointer channel, const ConnectionHandler& handler, const boost::system::error_code& e) {
            if (!e) {
                //                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (channel);
                //                cout << "acceptHandler: New incoming connect: " << ch->socket().remote_endpoint().address() << " : " << ch->socket().remote_endpoint().port() << endl;
                handler(channel);
            } else {
                if (m_errorHandler)
                    m_errorHandler(channel, e);
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            }
        }


        void TcpConnection::startClient(const ConnectionHandler& handler) {
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::util::toString(m_port));
                m_resolver->async_resolve(query, boost::bind(&TcpConnection::resolveHandler,
                                                             this, handler, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::resolveHandler(const ConnectionHandler& handler, const ErrorCode& e, ip::tcp::resolver::iterator it) {
            try {
                if (!e) {
                    Channel::Pointer new_channel = createChannel();
                    TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                    ch->socket().async_connect(*it, boost::bind(&TcpConnection::connectHandler,
                                                                this, new_channel, handler, boost::asio::placeholders::error));
                } else {
                    throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::connectHandler(Channel::Pointer channel, const ConnectionHandler& handler, const boost::system::error_code& e) {
            try {
                if (!e) {
                    //                    TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (channel);
                    //                    cout << "Successfully connected to: " << ch->socket().remote_endpoint().address() << " : " << ch->socket().remote_endpoint().port() << endl;
                    handler(channel);
                } else {
                    if (m_errorHandler)
                        m_errorHandler(channel, e);
                    else
                        throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::stop() {
            if (m_connectionType == "server")
                m_acceptor->close();

            // Think about stopping service
            //m_boost_io_service->stop();
        }


        ChannelPointer TcpConnection::createChannel() {
            ChannelPointer channel(new TcpChannel(shared_from_this()));
            registerChannel(channel);
            return channel;
        }
    }
}