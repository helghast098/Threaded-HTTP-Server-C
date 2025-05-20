
#include "gtest/gtest.h"
extern "C" {
#include "request_parser/request_parser.h"
}
#include <iostream>
#include <cstring>



void SetRequestBuffer( Request *request, std::string s ) {
    strcpy( request->buffer.data, s.c_str() );
    request->buffer.current_index = 0;
    request->buffer.length = s.length();
}


std::string SetFileLength( Request *request, size_t len ) {
    std::string req = "GET /";
    std::string file = "";
    for ( int i = 1; i <= len; ++i ) {
        file += "s";
    }
    req = req + file + " HTTP/1.1";
    SetRequestBuffer( request, req );
    return file;
}

Request request = {
        .buffer = {
            .data = new char[3000],
            .current_index = 0,
            .length = 0,
            .max_size = 3000
        },
        .current_state = INITIAL_STATE,
        .type = NOT_VALID,
        .file = new char[ FILE_NAME_LENGTH + 1 ]
};

TEST( RequestParserTest, _Method) {
    // Testing Empty String
    SetRequestBuffer( &request, "" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    // Testing Spaces
    SetRequestBuffer( &request, "          " );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    // Testing No Method
    SetRequestBuffer( &request, "/index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    // Testing Invalid Characters
    SetRequestBuffer( &request, "G@T /index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    SetRequestBuffer( &request, "123 /index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    SetRequestBuffer( &request, "@POST /index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    SetRequestBuffer( &request, "GE# /index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );
    
    SetRequestBuffer( &request, "get /index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    SetRequestBuffer( &request, "GeT /index.html HTTP/1.1" );
    EXPECT_EQ( -1, RequestChecker( &request ) );

    // Testing For Valid cases
    SetRequestBuffer( &request, "GET /index.html HTTP/1.1" );
    EXPECT_EQ( 0, RequestChecker( &request ) );
    EXPECT_EQ( GET, request.type );

    SetRequestBuffer( &request, "PUT /index.html HTTP/1.1" );
    EXPECT_EQ( 0, RequestChecker( &request ) );
    EXPECT_EQ( PUT, request.type );

    SetRequestBuffer( &request, "HEAD /index.html HTTP/1.1" );
    EXPECT_EQ( 0, RequestChecker( &request ) );
    EXPECT_EQ( HEAD, request.type );


}

TEST ( RequestParserTest, _URI) {
    // No uri given
    SetRequestBuffer(  &request, "GET HTTP/1.1" );
    EXPECT_EQ( -2, RequestChecker( &request ) );

    // single /
    SetRequestBuffer(  &request, "GET / HTTP/1.1" );
    EXPECT_EQ( -2, RequestChecker( &request ) );

    // multiple /// in a row
    SetRequestBuffer(  &request, "GET /hello//asdde HTTP/1.1" );
    EXPECT_EQ( -2, RequestChecker( &request ) );

    // rsurpass max file length
    SetFileLength( &request, FILE_NAME_LENGTH + 10 );
    EXPECT_EQ( -2, RequestChecker( &request ) );

    // Invalid File Chars
    SetRequestBuffer(  &request, "GET /hello/%$#asdde HTTP/1.1" );
    EXPECT_EQ( -2, RequestChecker( &request ) );

    // Valid Files
    // max file name
    std::string file =  SetFileLength( &request, FILE_NAME_LENGTH );
    EXPECT_EQ( 0, RequestChecker( &request ) );
    EXPECT_EQ( strcmp( request.file, file.c_str() ), 0 );

    // 100 char length
    file =  SetFileLength( &request, 100);
    EXPECT_EQ( 0, RequestChecker( &request ) );
    EXPECT_EQ( strcmp( request.file, file.c_str() ), 0 );

}

TEST ( RequestParserTest, _HTTP_VERSION ) {
    // lowercase
    SetRequestBuffer( &request, "HEAD /index.html http/1.1" );
    EXPECT_EQ( -3, RequestChecker( &request ) );

    // random letters
    SetRequestBuffer( &request, "HEAD /index.html fasfasfafsf/1.1" );
    EXPECT_EQ( -3, RequestChecker( &request ) );

    // removed entirely
    SetRequestBuffer( &request, "HEAD /index.html " );
    EXPECT_EQ( -3, RequestChecker( &request ) );

    // Different Version
    SetRequestBuffer( &request, "HEAD /index.html http/1.12323" );
    EXPECT_EQ( -3, RequestChecker( &request ) );

    // valid case
    SetRequestBuffer( &request, "HEAD /index.html HTTP/1.1" );
    EXPECT_EQ( 0, RequestChecker( &request ) );

}