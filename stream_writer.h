#ifndef SREAM_WRITER_H
#define SREAM_WRITER_H

#include "boost/asio.hpp"

template <typename StreamType>
struct stream_writer {

    using stream_type = StreamType;

    stream_writer( boost::asio::io_service &ios, stream_type *stream )
        :dispatcher_(ios)
        ,stream_(stream)
    { }

    boost::asio::io_service::strand dispatcher_;
    stream_type                    *stream_;
};

#endif // SREAM_WRITER_H
