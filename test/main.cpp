#include <functional>
#include <iostream>
#include "boost/asio.hpp"

namespace ba = boost::asio;
namespace bs = boost::system;

using sock_addr = ba::ip::address;
using read_cb   = std::function<void (const bs::system_error&, std::size_t)>;
using accept_cb = std::function<void (const bs::system_error&, sock_addr)>;


struct client_iface {
    virtual ~client_iface( ) { }
    virtual void async_read( read_cb cb ) = 0;
    virtual void close( ) = 0;
    virtual sock_addr address( ) const = 0;
};

struct acceptor_iface {
    virtual ~client_iface( ) { }
    virtual void async_accept( accept_cb ) = 0;
    virtual void close( ) = 0;
    virtual sock_addr address( ) const = 0;
};



int main( )
{
    try {
        ba::io_service ios;

    } catch( const std::exception &ex) {
        std::cerr << "General error: " << ex.what( ) << "\n";
    }

    return 0;
}

