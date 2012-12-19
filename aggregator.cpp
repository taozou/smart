/* 
 * File:   aggregator.cpp
 * Author: taozou
 * 
 * Created on December 18, 2012, 8:09 PM
 */

#include "aggregator.h"
#include <mpi.h>

aggregator::aggregator() {
    for (int i = 0; i < K; ++i)
        topk[K] = 0;
}

void aggregator::run(int receiveCount, int sendToRank) {
    int data[K];
    while (receiveCount)
    {
        MPI::COMM_WORLD.Recv(&data, K * sizeof(int), MPI::CHAR, MPI::ANY_SOURCE, MPI::ANY_TAG);
        
        for (int i = 0; i < K; ++i)
            for (int j = 0; j < K; ++j)
                if (topk[j] < data[i])
                {
                    topk[j] = data[i];
                    break;
                }
        --receiveCount;
    }
    
    
    if (sendToRank == -1)
    {
        for (int i = 0; i < K; ++i)
            printf("%d ", topk[i]);
        printf("\n");
    }
    else
    {
        MPI::COMM_WORLD.Send(&topk, K * sizeof(int), MPI::CHAR, sendToRank, 0);
    }
}

aggregator::~aggregator() {
}

