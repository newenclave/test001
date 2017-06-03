#ifndef TCT_CONNECTOR_H
#define TCT_CONNECTOR_H


#include "ifaces.h"
#include "stream_writer.h"

namespace ba = boost::asio;
namespace bs = boost::system;

struct tcp_connector: public i_client {

    tcp_connector( ba::io_service &ios )
        :sock_(ios)
    {
        sock_.get_socket( ).open( ba::ip::tcp::v4( ) );
    }

    void connect( const char *addr, std::uint16_t port )
    {
        using tcp_ep = ba::ip::tcp::endpoint;
        sock_.get_socket( ).connect(
                    tcp_ep(ba::ip::address::from_string(addr), port) );
    }

    void close( ) override
    {
        sock_.get_socket( ).close( );
    }

    void async_read( std::string *, read_cb ) override
    {

    }

    std::uintptr_t native_handle( ) override
    {
        using uptr = std::uintptr_t;
        return static_cast<uptr>(sock_.get_socket( ).native_handle( ));
    }

    void async_write_all( message::unique_ptr mess ) override
    {
        sock_.write( std::move(mess), shared_from_this( ) );
    }

    void async_write( message_type message )
    {
        sock_.write( std::move(message), shared_from_this( ) );
    }

    stream_writer<ba::ip::tcp::socket> sock_;
};

#endif // TCT_CONNECTOR_H
