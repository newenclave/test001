#ifndef SREAM_WRITER_H
#define SREAM_WRITER_H

#include <string>
#include <queue>
#include "boost/asio.hpp"

#include "/home/data/github/etool/include/etool/slices/memory.h"

using buffer = etool::slices::memory<char>;

template <typename StreamType, typename MessageT = std::string>
struct stream_writer {

    using stream_type  = StreamType;
    using message_type = MessageT;
    using this_type = stream_writer<StreamType, MessageT>;

    using weak_pointer = std::weak_ptr<void>;
    using error_code   = boost::system::error_code;

    struct queue_element {

        queue_element(message_type mess)
            :message(std::move(mess))
        { }

        virtual
        ~queue_element( ) = default;

        virtual
        void pre( )
        { }

        virtual
        void post( const error_code &, std::size_t )
        { }

        message_type &get( )
        {
            return message;
        }

        message_type message;
    };

    using queue_element_sptr = std::shared_ptr<queue_element>;
    using message_queue      = std::queue<queue_element_sptr>;

    std::shared_ptr<queue_element> make_element( message_type mess )
    {
        return std::make_shared<queue_element>(std::move(mess));
    }

    stream_writer( boost::asio::io_service &ios )
        :dispatcher_(ios)
        ,stream_(ios)
    { }

    stream_type &get_stream( )
    {
        return stream_;
    }

    const stream_type &get_stream( ) const
    {
        return stream_;
    }

    void async_write( weak_pointer wp )
    {
        message_type &top( queue_.front( )->message );
        queue_.front( )->pre( );
        async_write( top.data( ), top.size( ), 0, std::move(wp) );
    }

    bool active( ) const
    {
        return true;
    }

    void async_write( const char *data, size_t length, size_t total,
                      weak_pointer wp )
    {
        namespace ph = std::placeholders;
        if( active( ) ) {
            get_stream( ).async_write_some(
                    boost::asio::buffer(data, length),
                    dispatcher_.wrap(
                        std::bind( &this_type::write_handler, this,
                                    ph::_1, ph::_2,
                                    length, total,
                                    std::move(wp) )
                    )
            );
        }
    }

    void write_handler( const boost::system::error_code &error,
                        size_t const bytes,
                        size_t const length,
                        size_t       total,
                        weak_pointer wp )
    {
        auto sp = wp.lock( );
        if( !sp ) {
            return;
        }

        auto &top( *queue_.front( ) );

        if( !error ) {

            if( bytes < length ) {

                total += bytes;

                const message_type &top_mess( top.message );
                async_write( top_mess.data( ) + total,
                             top_mess.size( ) - total, total,
                             std::move(wp) );

            } else {

                top.post( error, bytes );

                queue_.pop( );

                if( !queue_.empty( ) ) {
                    async_write( std::move(wp) );
                }
            }

        } else {

            top.post( error, bytes );

            /// pop queue
            queue_.pop( );

            if( !queue_.empty( ) ) {
                async_write( std::move(wp) );
            }
        }

    }

    void write_impl( queue_element_sptr message, weak_pointer wp )
    {
        auto sp = wp.lock( );
        if( !sp ) {
            return;
        }

        const bool empty = queue_.empty( );
        queue_.emplace( message );
        if( empty ) {
            async_write( std::move(wp) );
        }
    }

    void write( queue_element_sptr message, weak_pointer wp )
    {
        dispatcher_.post( std::bind( &this_type::write_impl, this,
                                     message, wp ) );

    }

    void write( message_type message, weak_pointer wp )
    {
        write( make_element(std::move(message)), std::move(wp) );
    }

    boost::asio::io_service &get_io_service( )
    {
        return dispatcher_.get_io_service( );
    }

    boost::asio::io_service::strand dispatcher_;
    stream_type                     stream_;
    message_queue                   queue_;
};

#endif // SREAM_WRITER_H
