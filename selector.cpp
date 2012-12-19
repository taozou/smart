/* 
 * File:   selector.cpp
 * Author: taozou
 * 
 * Created on December 18, 2012, 7:47 PM
 */

#include "selector.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <mpi.h>

selector::selector()
{
}

inline void selector::getKey(char* buf, int id)
{
    sprintf(buf, "%d/16mb", id);
}

void selector::preProcess(unsigned char* buf)
{
    int* data = (int *) buf;
    int count = BucketSize / sizeof(int);
    
    for (int i = 0; i < count ; ++i)
        for (int j = 0 ; j < K; ++j)
            if (topk[j] < data[i])
            {
                topk[j] = data[i];
                break;
            }
}

bool selector::init(char * bucketName) {
    toDelete = false;
    S3Config config = {};
    
    if( !( config.accKey = getenv( "AWS_ACCESS_KEY" ) ) ||
        !( config.secKey = getenv( "AWS_SECRET_KEY" ) )  )
    {
        fprintf(stderr, "no AWS_XXXX is set. \n");
        return false;
    }

    memset(topk, 0 , K * sizeof(int));
    strcpy(this->bucketName, bucketName);

    cons = new S3Connection*[ConnectionCount];
    buf = new unsigned char*[ConnectionCount];
    
    for ( int i = 0; i < ConnectionCount; ++i )
    {
        cons[i] = new S3Connection(config);
        buf[i] = new unsigned char[ BucketSize ];
    }
    toDelete = true;
    return true;
}

void selector::run(int idLow, int idHigh, int sendToRank) {
    char key[100];
    int totalKey = idHigh - idLow;
    for ( int i = 0; i < ConnectionCount && i < totalKey; ++i )
    {
        getKey(key, idLow+i);
        cons[i]->pendGet( &asyncMans[i % AsyncManCount],
                bucketName, key, buf[i], BucketSize);
    }

    for ( int i = ConnectionCount; i < totalKey; ++i)
    {
        int k = S3Connection::waitAny( cons, ConnectionCount, i % ConnectionCount);

        //S3GetResponse response;
        try
        {
            cons[k]->completeGet();
        }
        catch ( ... ) {
            fprintf(stderr, "get fail on %d\n", idLow + i);
        }
        
        preProcess(buf[k]);
        
        getKey(key, idLow+i);
        cons[k]->pendGet( &asyncMans[i % AsyncManCount], 
            bucketName, key, buf[k], BucketSize);
    }
    
    for ( int i = 0; i < ConnectionCount; ++i )
    {
        cons[i]->completeGet();
        preProcess(buf[i]);
    }
    //double bandwidth = 1000.0 * objectMB * totalKey/ stopwatch.elapsed();
    //std::cout << rank << ": " << bandwidth << "MiB/s\n";
    MPI::COMM_WORLD.Send(&topk, K * sizeof(int), MPI::CHAR, sendToRank, 0);
}

selector::~selector() {
    if (toDelete)
    {
        for ( int i = 0; i < ConnectionCount; ++i )
        {
            delete[] buf[i];
            delete cons[i];
        }
        delete cons;
    }
}

