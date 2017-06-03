#ifndef UDP_CONNECTOR_H
#define UDP_CONNECTOR_H


#include "ifaces.h"
#include "block_writer.h"

namespace ba = boost::asio;
namespace bs = boost::system;

struct udp_connector: public i_client {


    udp_connector( ba::io_service &ios )
        :sock_(ios)
    {
        sock_.get_socket( ).open( ba::ip::udp::v4( ) );
    }

    void connect( const char *addr, std::uint16_t port )
    {
        using ep = ba::ip::udp::endpoint;
        sock_.get_socket( ).connect( ep(ba::ip::address::from_string(addr), port) );
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
        //sock_.write( std::move(message), shared_from_this( ) );
    }


    block_writer<ba::ip::udp::socket> sock_;
};


#endif // UDP_CONNECTOR_H
