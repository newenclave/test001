#include <iostream>

#include "ifaces.h"
#include "stream_writer.h"

namespace ba = boost::asio;
namespace bs = boost::system;

struct tcp_connector: public i_client {

    tcp_connector( ba::io_service &ios )
        :sock_(ios)
    {
        sock_.get_stream( ).open( ba::ip::tcp::v4( ) );
    }

    void connect( const char *addr, std::uint16_t port )
    {
        using tcp_ep = ba::ip::tcp::endpoint;
        sock_.get_stream( ).connect(
                    tcp_ep(ba::ip::address::from_string(addr), port) );
    }

    void close( )
    {
        sock_.get_stream( ).close( );
    }

    void async_read( read_cb )
    {

    }

    void async_write_all( std::string mess )
    {
        sock_.write( std::move(mess), shared_from_this( ) );
    }

    stream_writer<ba::ip::tcp::socket> sock_;
};

int main_c( )
{
    try {

        ba::io_service ios;
        ba::io_service::work wrk(ios);

        auto tcp_conn = std::make_shared<tcp_connector>(std::ref(ios));

        tcp_conn->connect( "127.0.0.1", 44556 );
        tcp_conn->async_write_all( std::string(1024 * 1024, '!') );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "G Error: " << ex.what( ) << "\n";
    }
    return 0;
}
