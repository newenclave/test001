#ifndef IFACES_H
#define IFACES_H

#include <memory>
#include "boost/asio.hpp"

using error_code = boost::system::error_code;

using read_cb = std::function<void ( const error_code &, std::size_t ) >;

struct i_client: public std::enable_shared_from_this<i_client> {
    virtual ~i_client( ) = default;
    virtual void close( ) = 0;
    virtual void async_read( read_cb ) = 0;
    virtual void async_write_all( std::string ) = 0;
};

using accept_cb = std::function<void ( const error_code &,
                                       std::shared_ptr<i_client>) >;

struct i_accept: public std::enable_shared_from_this<i_accept> {
    virtual ~i_accept( ) = default;
    virtual void close( ) = 0;
    virtual void async_accept( accept_cb ) = 0;
};

#endif // IFACES_H
