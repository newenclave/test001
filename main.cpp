#include <iostream>
#include <memory>
#include <map>

#include "ifaces.h"
#include "stream_writer.h"
#include "tcp_acceptor.h"

struct udp_acceptor: public i_accept {

    using ep = ba::ip::udp::endpoint;
    using clients = std::map<ep, std::weak_ptr<i_client> >;
    using udp_acceptor_wptr = std::weak_ptr<udp_acceptor>;
    using acceptor_wptr = std::weak_ptr<i_accept>;

    struct udp_client: public i_client {

        using weak_type = std::weak_ptr<i_client>;

        udp_client( ba::io_service &ios, std::weak_ptr<i_accept> p )
            :parent_(p)
            ,dispatcher_(ios)
        { }

        void close( ) override
        {
            auto sp = parent_.lock( );
            if( !sp ) {
                return;
            }
            auto spp = static_cast<udp_acceptor *>(sp.get( ));
            spp->remove_client( std::move(my_) );
        }

        void async_read( message_type *mess, read_cb cb ) override
        {
            weak_type wp = shared_from_this( );
            dispatcher_.post( [this, wp, mess, cb]( ) {
                auto sp = wp.lock( );
                if( !sp ) {
                    return;
                }
                if( read_ ) {
                    read_(error_code(0, bs::system_category( )), 0);
                }
                read_       = std::move(cb);
                read_param_ = mess;
            } );
        }

        void async_write_all( message::unique_ptr mess ) override
        {
            auto sp = parent_.lock( );
            if( !sp ) {
                return;
            }
            auto spp = static_cast<udp_acceptor *>(sp.get( ));
            spp->write_message( my_, std::move(mess) );
        }

        std::uintptr_t native_handle( ) override
        {
            return 0;
        }

        void push_impl( message_type mess, weak_type wp )
        {
            auto sp = wp.lock( );
            if( !sp ) {
                return;
            }
            if( read_ ) {
                read_param_->swap(mess);
                error_code ec(0, bs::system_category( ));
                read_( ec, mess.size( ) );

                read_ = read_cb( );
                read_param_ = nullptr;

            } else {
                queue_.emplace(std::move(mess));
            }
        }

        void push_message( message_type mess, std::size_t len )
        {
            weak_type wp = shared_from_this( );
            mess.resize( len );
            dispatcher_.post( [this, wp, mess](  ){
                push_impl( mess, wp );
            } );
        }

        std::weak_ptr<i_accept> parent_;
        ba::io_service::strand  dispatcher_;

        std::queue<message_type> queue_;
        read_cb       read_;
        message_type *read_param_;
        ep            my_;

    };

    udp_acceptor( ba::io_service &ios )
        :sock_(ios)
        ,dispatcher_(ios)
    {
        sock_.open( ba::ip::udp::v4( ) );
        sock_.bind( ep(ba::ip::address::from_string("0.0.0.0"), 44557) );
        message_.resize( 4096 );
    }

    void close( )
    {
        sock_.close( );
    }

    void write_handle( const error_code &err, std::size_t len,
                       acceptor_wptr wp )
    {
        auto sp = wp.lock( );
        if( !sp ) {
            return;
        }
        std::error_code ec(err.value( ), std::system_category( ));
        queue_.front( )->postcall( ec, len );
        queue_.pop( );
        if( !queue_.empty( ) ) {
//            async_write( wp, ... );
        }

    }

    void async_write( acceptor_wptr wp, const ep &to )
    {
        namespace ph = std::placeholders;
        auto sp = wp.lock( );
        if( !sp ) {
            return;
        }
        queue_.front( )->precall( );

        sock_.async_send_to( ba::buffer( queue_.front( )->data( ),
                                         queue_.front( )->size( )),
                             to, 0,
                             dispatcher_.wrap(
                                 std::bind( &udp_acceptor::write_handle, this,
                                            ph::_1, ph::_2, wp ) ) );
    }

    void write_message( const ep &to, message::unique_ptr mess )
    {
        std::weak_ptr<i_accept> wp = shared_from_this( );
        auto suptr = std::make_shared<message::unique_ptr>(std::move(mess));
        dispatcher_.post( [this, wp, to, suptr]( ) {

            auto sp = wp.lock( );
            if( !sp ) {
                return;
            }

            auto empty = queue_.empty( );
            queue_.emplace( std::move(*suptr) );
            if( empty ) {
                async_write(wp, to);
            }
        } );
    }

    void remove_client( ep client )
    {
        std::weak_ptr<i_accept> wp = shared_from_this( );
        dispatcher_.post( [this, wp, client]( ) {
            auto sp = wp.lock( );
            if( !sp ) {
                return;
            }
            clients_.erase( client );
        } );
    }

    void read_handler( const error_code &error,
                       size_t const bytes, acceptor_wptr wp )
    {
        auto sp = wp.lock( );
        if( sp ) {
            if( !error ) {

                std::shared_ptr<udp_client> client;
                auto f = clients_.find( sender_ );
                if( f != clients_.end( ) ) {
                    client = f->second.lock( );
                }

                if(client) {
                    client->push_message( std::move(message_), bytes );
                    message_.resize( 4096 );
                } else {
                    client = std::make_shared<udp_client>( get_io_service( ),
                                                           shared_from_this( ));
                }

            } else {
                if(current_) {
                    current_(error, std::shared_ptr<i_client>( ) );
                }
            }
        }
    }

    void start_read( )
    {
        namespace ph = std::placeholders;

        std::weak_ptr<i_accept> wp = shared_from_this( );
        sender_ = ep( );
        sock_.async_receive_from( ba::buffer(&message_[0], message_.size( )),
                    sender_, 0,
                    dispatcher_.wrap(
                        std::bind(&udp_acceptor::read_handler, this,
                                   ph::_1, ph::_2, wp )
                    ) );
    }

    void async_accept( accept_cb cb )
    {
        std::weak_ptr<i_accept> wp = shared_from_this( );

        dispatcher_.post( [cb, wp, this]( ) {
            auto sp = wp.lock( );
            if( !sp ) {
                return;
            }
            current_( error_code(0, boost::system::system_category( ) ),
                      std::shared_ptr<i_client>( ) );
            current_ = std::move(cb);
        } );
    }

    ba::io_service &get_io_service( )
    {
        return sock_.get_io_service( );
    }

    message_type            message_;
    ba::ip::udp::socket     sock_;
    ba::io_service::strand  dispatcher_;
    accept_cb               current_;
    ep                      sender_;
    std::map<ep, std::weak_ptr<udp_client> > clients_;
    std::queue<message::unique_ptr> queue_;
};

int main_c( );
int main_s( )
{
    try {

        ba::io_service ios( 1 );
        ba::io_service::work wrk(ios);
        std::string message(1024, '\0');

        auto ta = std::make_shared<udp_acceptor>(std::ref(ios));

        ta->async_accept( [ta, &message](const error_code &e,
                                         std::shared_ptr<i_client> c)
        {
            std::cout << e.message( ) << " Accept!\n";

            auto call = [ta, c ]( const error_code &e, std::size_t l ) {
                std::cout << e.message( ) <<  ": Read " << l << " bytes! "
                          << std::endl;
                // ta->acc_.get_io_service( ).stop( );
            };
            c->async_read( &message, call );
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
