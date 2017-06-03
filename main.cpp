#include <iostream>
#include <memory>
#include <map>
#include <set>

#include "ifaces.h"
#include "stream_writer.h"
#include "tcp_acceptor.h"
#include "udp_acceptor.h"

#include "/home/data/github/etool/include/etool/dumper/dump.h"

namespace {
    int count = 0;

    std::set<std::shared_ptr<i_client> > clients;

    std::shared_ptr<udp_acceptor> ta;
}

using namespace etool;

void client_read( const error_code &e, size_t len, std::shared_ptr<i_client> c )
{
    if( e ) {
        std::cerr << "Client read error " << e.message( ) << "\n";
        count = 100;
    } else {
        count ++;
        std::cout << "Client read " << len << " bytes\n";
        auto &m(c->last_message( ));
        dumper::make<>::all( m.c_str( ), len, std::cout ) << "\n";
        c->close( );
        clients.erase( c );
    }
    if( count > 1 ) {
        ta->sock_.get_io_service( ).stop( );
    }
}

void accept_client( const error_code &e, std::shared_ptr<i_client> c )
{
    if( !e ) {
        //clients.insert( c );
        std::cout << "New client!\n";
        ta->async_accept( &accept_client );
        //ta->sock_.get_io_service( ).stop( );
        c->async_read( 255, [c](const error_code &e, size_t len) {
            client_read(e, len, c);
        } );
    }
}

int main_c( );
int main_s( )
{
    try {

        ba::io_service ios( 1 );
        ba::io_service::work wrk(ios);
        std::string message(1024, '\0');

        ta = std::make_shared<udp_acceptor>(std::ref(ios));

        ta->start_read( );
        ta->async_accept( &accept_client );

        ios.run( );

        clients.clear( );
        ta.reset( );

    } catch( const std::exception &ex ) {
        std::cerr << "G Error: " << ex.what( ) << "\n";
    }

    return 0;
}


int main( int argc, char **argv )
{
    return argc == 1 ? main_s( ) : main_c( );
}
