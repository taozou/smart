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
#define BucketSize 16777216
#define K 10

using namespace std;
using namespace webstor;
using namespace webstor::internal;

class selector {
public:
    selector();
    ~selector();
    
    bool init(char * bucketName);
    
    void run(int idLow, int idHigh, int sendToRank);

    
    inline void getKey(char *buf, int id);
    void preProcess(unsigned char * buf);
    
    bool toDelete;
    char bucketName[100];
    int topk[K];
    unsigned char** buf;
    AsyncMan asyncMans[AsyncManCount];
    S3Connection **cons;
};

#endif	/* SELECTOR_H */

