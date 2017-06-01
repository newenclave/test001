#include <iostream>
#include <memory>

#include "ifaces.h"

#include <sys/types.h>
#include <sys/sysctl.h>

namespace ba = boost::asio;
namespace bs = boost::system;

struct tcp_acceptor: public i_accept {

    struct tcp_client: public i_client {

        tcp_client( ba::io_service &ios )
            :sock_(ios)
            ,block_(4096)
        { }

        void close( )
        {
            sock_.close( );
        }

        void async_read( read_cb cb )
        {
            std::weak_ptr<i_client> weak_this(shared_from_this( ));

            auto this_cb = [cb, weak_this](const error_code &e, std::size_t l) {
                if( e ) {
                    std::cout << "read error " << e.message( ) << "\n";
                    cb(e, 0);
                    return;
                }
                auto lock = weak_this.lock( );
                if( lock ) {
                    cb(e, l);
                } else {
                    cb( e, 0 );
                    return;
                }
            };
            sock_.async_read_some( ba::buffer(block_), this_cb );
        }

        void async_write_all(  )
        {

        }

        std::weak_ptr<i_accept> parent_;
        ba::ip::tcp::socket sock_;
        std::vector<char> block_;
    };

    tcp_acceptor( ba::io_service &ios )
        :acc_(ios)
    {
        using ep = ba::ip::tcp::endpoint;
        acc_.open( ba::ip::tcp::v4( ) );
        acc_.bind( ep(ba::ip::address_v4( ), 44556) );
        acc_.listen( 5 );
    }

    void close( )
    {
        acc_.close( );
    }

    void async_accept( accept_cb cb )
    {
        auto client = std::make_shared<tcp_client>(acc_.get_io_service( ));
        std::weak_ptr<i_accept> weak_this(shared_from_this( ));

        auto this_cb = [client, weak_this, cb]( const error_code &err ) {
            if( !err ) {
                auto lck = weak_this.lock( );
                if( lck ) {
                    cb( err, client );
                    return;
                }
            }
            cb( err, std::shared_ptr<i_client>( ) );
        };

        acc_.async_accept( client->sock_, this_cb );
    }

    ba::ip::tcp::acceptor acc_;
};

int main_c( );
int main_s( )
{
    try {

        ba::io_service ios;
        ba::io_service::work wrk(ios);

        auto ta = std::make_shared<tcp_acceptor>(std::ref(ios));

        ta->async_accept( [ta](const error_code &e, std::shared_ptr<i_client> c) {
            std::cout << e.message( ) << " Accept!\n";
            c->async_read( [ta, c]( const error_code &e, std::size_t l ) {
                std::cout << e.message( ) <<  ": Read " << l << " bytes!\n";
                ta->acc_.get_io_service( ).stop( );
            } );
        } );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "G Error: " << ex.what( ) << "\n";
    }

    return 0;
}


int main( int argc, char **argv )
{
    return argc == 1 ? main_s( ) : main_c( );
}
