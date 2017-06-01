#include <iostream>

#include "ifaces.h"


namespace ba = boost::asio;
namespace bs = boost::system;

struct tcp_comnector: public i_client {

    tcp_comnector( ba::io_service &ios )
        :sock_(ios)
    {
        sock_.open( ba::ip::tcp::v4( ) );
    }

    void connect( const char *addr, std::uint16_t port )
    {
        using tcp_ep = ba::ip::tcp::endpoint;
        sock_.connect( tcp_ep(ba::ip::address::from_string(addr), port) );
    }

    void close( )
    {
        sock_.close( );
    }

    void async_read( read_cb )
    {

    }

    ba::ip::tcp::socket sock_;
};

int main_c( )
{
    try {

        ba::io_service ios;
        ba::io_service::work wrk(ios);

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "G Error: " << ex.what( ) << "\n";
    }
    return 0;
}
