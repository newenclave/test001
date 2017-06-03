#ifndef BLOCK_WRITER_H
#define BLOCK_WRITER_H

#include <string>
#include <queue>

#include "boost/asio.hpp"
#include "/home/data/github/etool/include/etool/slices/memory.h"

#include "message.h"

template <typename SocketType, typename MessageT = std::string>
struct block_writer {

    using socket_type  = SocketType;
    using message_type = MessageT;
    using this_type = block_writer<SocketType, MessageT>;

    using weak_pointer = std::weak_ptr<void>;
    using error_code   = boost::system::error_code;

    using message_queue = std::queue<message::unique_ptr>;

    block_writer( boost::asio::io_service &ios )
        :dispatcher_(ios)
        ,stream_(ios)
    { }

    bool active(  ) const
    {
        return true;
    }

    socket_type &get_socket( )
    {
        return stream_;
    }

    const socket_type &get_socket( ) const
    {
        return stream_;
    }

    void write_handler( const boost::system::error_code &error,
                        size_t const bytes, weak_pointer wp )
    {
        auto sp = wp.lock( );
        if( !sp ) {
            return;
        }
        std::error_code ec(error.value( ), std::system_category( ));
        queue_.front( )->postcall( ec, bytes );
        queue_.pop( );
        if( !queue_.empty( ) ) {
            async_write( std::move(wp) );
        }
    }

    void async_write( weak_pointer wp )
    {
        namespace ph = std::placeholders;

        auto &top( queue_.front( ) );
        top->precall( );
        if( active( ) ) {

            auto handler = std::bind( &this_type::write_handler, this,
                                      ph::_1, ph::_2, std::move(wp) );

            stream_.async_send( boost::asio::buffer( top->data( ),
                                                     top->size( ) ),
                                dispatcher_.wrap( std::move(handler) ) );
        }

    }

    void write_impl( std::shared_ptr<message::unique_ptr> message,
                     weak_pointer wp )
    {
        auto sp = wp.lock( );
        if( !sp ) {
            return;
        }

        const bool empty = queue_.empty( );
        queue_.emplace( std::move(*message) );
        if( empty ) {
            async_write( std::move(wp) );
        }
    }

    void write( message::unique_ptr message, weak_pointer wp )
    {
        auto suptr = std::make_shared<message::unique_ptr>(std::move(message));
        dispatcher_.post( std::bind( &this_type::write_impl, this,
                                     std::move(suptr), std::move(wp) ) );

    }

    boost::asio::io_service::strand dispatcher_;
    socket_type                     stream_;
    message_queue                   queue_;
    bool                            active_;

};

#endif // BLOCK_WRITER_H
