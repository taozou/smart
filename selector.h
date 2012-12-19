/* 
 * File:   selector.h
 * Author: taozou
 *
 * Created on December 18, 2012, 7:47 PM
 */

#ifndef SELECTOR_H
#define	SELECTOR_H

#include "s3conn.h"
#include "sysutils.h"

#define AsyncManCount 2
#define ConnectionCount 16

using namespace webstor;
using namespace webstor::internal;

class selector {
public:
    selector();
    ~selector();
    
    void run();
    
    unsigned char** buf;
    AsyncMan asyncMans[AsyncManCount];
    S3Connection *cons;
};

#endif	/* SELECTOR_H */

