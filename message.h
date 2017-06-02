#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <memory>
#include <system_error>


struct message {

    using error_code = std::error_code;

    virtual ~message( ) { }
    virtual const char *data( ) const                           = 0;
    virtual std::size_t size( ) const                           = 0;
    virtual void precall( )                                     = 0;
    virtual void postcall( const error_code &, std::size_t )    = 0;

    using shared_ptr = std::shared_ptr<message>;
    using unique_ptr = std::unique_ptr<message>;
};

#endif // MESSAGE_H
