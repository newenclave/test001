cmake_minimum_required(VERSION 2.8)

if( ASIO_STANDALONE_DIR )
    include_directories( ${ASIO_STANDALONE_DIR} )
    add_definitions( -DASIO_STANDALONE=1 )
else( )
    find_package( Boost 1.53 COMPONENTS
                        system
                        thread # test
                    REQUIRED)
endif( ASIO_STANDALONE_DIR )

find_package( Boost 1.53 COMPONENTS
                    system
                    thread # test
                REQUIRED)


add_executable(test001 "main.cpp" "client.cpp" )
target_link_libraries( test001 ${Boost_LIBRARIES} )
