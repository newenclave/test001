#include <iostream>

#include "tcp_connector.h"
#include "udp_connector.h"

message::unique_ptr make_message( std::string mess )
{
    struct m: public message {
        m(std::string mess)
            :mm_(std::move(mess))
        { }
        const char *data( ) const
        {
            return mm_.c_str( );
        }

        std::size_t size( ) const
        {
            return mm_.size( );
        }

        void precall( )
        {
            std::cout << "Precall!\n";
        }
        void postcall( const error_code &ec, std::size_t len)
        {
            std::cout << "Postcall! " << ec.message( ) << " " << len << "\n";

        }
        std::string mm_;
    };
    return message::unique_ptr(new m(std::move(mess)));
}

int main_c( )
{
    try {

        ba::io_service ios;
        ba::io_service::work wrk(ios);

        auto tcp_conn = std::make_shared<udp_connector>(std::ref(ios));

        tcp_conn->connect( "127.0.0.1", 44557 );
        tcp_conn->async_write_all( make_message(std::string(1024, '!')) );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "G Error: " << ex.what( ) << "\n";
    }
    return 0;
}
