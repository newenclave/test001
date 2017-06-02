#ifndef TCP_ACCEPTOR_H
#define TCP_ACCEPTOR_H

#include "ifaces.h"
#include "stream_writer.h"

namespace ba = boost::asio;
namespace bs = boost::system;

struct tcp_acceptor: public i_accept {

    struct tcp_client: public i_client {

        tcp_client( ba::io_service &ios )
            :sock_(ios)
        { }

        void close( )
        {
            sock_.get_stream( ).close( );
        }

        void async_read( message_type & mess, read_cb cb ) override
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
            sock_.get_stream( ).async_read_some( ba::buffer(&mess[0],
                                                            mess.size( )),
                                                 std::move(this_cb) );
        }

        std::uintptr_t native_handle( ) override
        {
            using uptr = std::uintptr_t;
            return static_cast<uptr>(sock_.get_stream( ).native_handle( ));
        }

        void async_write_all( message::unique_ptr mess ) override
        {
            sock_.write( std::move(mess), shared_from_this( ) );
        }

        std::weak_ptr<i_accept> parent_;
        stream_writer<ba::ip::tcp::socket> sock_;
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

        acc_.async_accept( client->sock_.get_stream( ), std::move(this_cb) );
    }

    ba::ip::tcp::acceptor acc_;
};


#endif // TCP_ACCEPTOR_H
