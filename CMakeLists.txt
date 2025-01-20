cmake_minimum_required(VERSION 3.25)
project(xhttp C)

set(CMAKE_C_STANDARD 11)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g -O1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")


add_executable(xhttp_c
        client/Client.c
        utils/TcpClientUtility.c
        logger/Logger.c
        includes/crypt.h
        includes/utils.h
        utils/TcpServerUtility.c
        utils/SocketUtility.c
        utils/Compressor.c
        Encoder/Encoder.c
        includes/packet.h
        crypt/AES.c
        utils/AddressUtility.c
        )

add_executable(test_c
        tests/test.c
        utils/TcpClientUtility.c
        logger/Logger.c
        includes/crypt.h
        includes/utils.h
        utils/TcpServerUtility.c
        utils/SocketUtility.c
        utils/Compressor.c
        Encoder/Encoder.c
        includes/packet.h
        crypt/AES.c utils/AddressUtility.c
        )

add_executable(xhttp_s
        server/Server.c
        utils/TcpClientUtility.c
        logger/Logger.c
        includes/crypt.h
        includes/utils.h
        utils/TcpServerUtility.c
        utils/SocketUtility.c
        utils/Compressor.c
        Encoder/Encoder.c
        includes/packet.h
        crypt/AES.c crypt/AES.c utils/AddressUtility.c)

target_link_libraries(test_c PRIVATE ZLIB::ZLIB)
target_link_libraries(xhttp_c PRIVATE ZLIB::ZLIB)
target_link_libraries(xhttp_s PRIVATE ZLIB::ZLIB)
target_link_libraries(xhttp_c PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(test_c PRIVATE OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(xhttp_s PRIVATE OpenSSL::Crypto OpenSSL::SSL)