/* 
 * File:   aggregator.h
 * Author: taozou
 *
 * Created on December 18, 2012, 8:09 PM
 */

#ifndef AGGREGATOR_H
#define	AGGREGATOR_H

#define K 10

class aggregator {
public:
    aggregator();
    
    void run(int, int);
    
    ~aggregator();
    
    int topk[K];
    int sendToRank, receiveCount;
};

#endif	/* AGGREGATOR_H */

