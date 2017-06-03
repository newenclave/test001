#include <iostream>
#include <memory>
#include <map>
#include <set>

#include "ifaces.h"
#include "stream_writer.h"
#include "tcp_acceptor.h"
#include "udp_acceptor.h"

int count = 0;

std::set<std::shared_ptr<i_client> > clients;


int main_c( );
int main_s( )
{
//    try {

        ba::io_service ios( 1 );
        ba::io_service::work wrk(ios);
        std::string message(1024, '\0');

        auto ta = std::make_shared<udp_acceptor>(std::ref(ios));

        ta->start_read( );
        ta->async_accept( [ta, &message](const error_code &e,
                                         std::shared_ptr<i_client> c)
        {
            std::cout << e.message( ) << " Accept!\n";

            auto call = [ta, c ]( const error_code &e, std::size_t l ) {
                std::cout << e.message( ) <<  ": Read " << l << " bytes! "
                          << std::endl;
                ta->sock_.get_io_service( ).stop( );
            };
            c->async_read( &message, call );
        } );

        ios.run( );

//    } catch( const std::exception &ex ) {
//        std::cerr << "G Error: " << ex.what( ) << "\n";
//    }

    return 0;
}


int main( int argc, char **argv )
{
    return argc == 1 ? main_s( ) : main_c( );
}
