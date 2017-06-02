#ifndef SREAM_WRITER_H
#define SREAM_WRITER_H

#include <string>
#include <queue>

#include "boost/asio.hpp"
#include "/home/data/github/etool/include/etool/slices/memory.h"

#include "message.h"

using buffer = etool::slices::memory<char>;

template <typename StreamType, typename MessageT = std::string>
struct stream_writer {

    using stream_type  = StreamType;
    using message_type = MessageT;
    using this_type = stream_writer<StreamType, MessageT>;

    using weak_pointer = std::weak_ptr<void>;
    using error_code   = boost::system::error_code;

    struct queue_element: public message {

        queue_element(message_type mess)
            :message(std::move(mess))
        { }

        virtual ~queue_element( ) = default;

        const char *data( ) const override
        {
            return message.c_str( );
        }

        std::size_t size( ) const override
        {
            return message.size( );
        }

        void precall( ) override
        { }

        void postcall( const error_code &, std::size_t ) override
        { }

        message_type message;
    };

    using message_sptr  = std::shared_ptr<message>;
    using message_uptr  = std::unique_ptr<message>;
    using message_queue = std::queue<message_uptr>;

    message_uptr make_element( message_type mess )
    {
        message_uptr res(new queue_element(std::move(mess)));
        return res;
    }

    stream_writer( boost::asio::io_service &ios )
        :dispatcher_(ios)
        ,stream_(ios)
        ,active_(true)
    { }

    stream_type &get_stream( )
    {
        return stream_;
    }

    const stream_type &get_stream( ) const
    {
        return stream_;
    }

    void set_active( bool val )
    {
        active_ = val;
    }

    bool active( ) const
    {
        return active_;
    }

private:

    void async_write( weak_pointer wp )
    {
        auto &top( queue_.front( ) );
        top->precall( );
        async_write( top->data( ), top->size( ), 0, std::move(wp) );
    }

    void async_write( const char *data, size_t length, size_t total,
                      weak_pointer wp )
    {
        namespace ph = std::placeholders;
        if( active( ) ) {

            auto handler = std::bind( &this_type::write_handler, this,
                                      ph::_1, ph::_2, length, total,
                                      std::move(wp) );

            get_stream( ).async_write_some( boost::asio::buffer( data, length ),
                                       dispatcher_.wrap( std::move(handler) ) );
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

        std::error_code ec(error.value( ), std::system_category( ));

        auto &top( *queue_.front( ) );

        if( !ec ) {

            if( bytes < length ) {

                total += bytes;

                async_write( top.data( ) + total,
                             top.size( ) - total, total,
                             std::move(wp) );

            } else {

                top.postcall( ec, bytes );

                queue_.pop( );

                if( !queue_.empty( ) ) {
                    async_write( std::move(wp) );
                }
            }

        } else {

            top.postcall( ec, bytes );

            /// pop queue
            queue_.pop( );

            if( !queue_.empty( ) ) {
                async_write( std::move(wp) );
            }
        }
    }

    void write_impl( std::shared_ptr<message_uptr> message, weak_pointer wp )
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

public:

    void write( message_uptr message, weak_pointer wp )
    {
        auto suptr = std::make_shared<message_uptr>(std::move(message));
        dispatcher_.post( std::bind( &this_type::write_impl, this,
                                     std::move(suptr), std::move(wp) ) );

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
    bool                            active_;
};

#endif // SREAM_WRITER_H
