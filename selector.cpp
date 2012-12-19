/* 
 * File:   selector.cpp
 * Author: taozou
 * 
 * Created on December 18, 2012, 7:47 PM
 */

#include "selector.h"

selector::selector() {
    S3Config config = {};
    cons = new S3Connection*[ConnectionCount];
    buf = new unsigned char*[ConnectionCount];
    asyncMans = new AsyncMan[AsyncManCount];
    
    if( !( config.accKey = getenv( "AWS_ACCESS_KEY" ) ) ||
        !( config.secKey = getenv( "AWS_SECRET_KEY" ) )  )
    {
        std::cout << "no AWS_XXXX is set. ";
        return 1;
    }
    
    for ( int i = 0; i < ConnectionCount; ++i )
    {
        cons[i] = new S3Connection(config);
        buf[i] = new unsigned char[ unitSize ];
        memset(buf[i], 0, unitSize);
    }
}

selector::~selector() {
    
}

