#ifndef IFACES_H
#define IFACES_H

#include <memory>
#include "boost/asio.hpp"
#include "/home/data/github/etool/include/etool/slices/memory.h"

#include "message.h"

using error_code = boost::system::error_code;

using read_cb = std::function<void ( const error_code &, std::size_t ) >;
using message_type = std::string;

struct i_client: public std::enable_shared_from_this<i_client> {
    virtual ~i_client( ) = default;
    virtual void close( ) = 0;
    virtual void async_read( message_type *, read_cb ) = 0;
    virtual void async_write_all( message::unique_ptr ) = 0;
    virtual std::uintptr_t native_handle( ) = 0;
};

using accept_cb = std::function<void ( const error_code &,
                                       std::shared_ptr<i_client>) >;

struct i_accept: public std::enable_shared_from_this<i_accept> {
    virtual ~i_accept( ) = default;
    virtual void close( ) = 0;
    virtual void async_accept( accept_cb ) = 0;
};

#endif // IFACES_H
