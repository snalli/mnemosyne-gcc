/*
 * main.cpp
 *
 *  Created on: Apr 23, 2016
 *      Author: pramalhe
 */
#include <thread>
#include <string>

#include "BenchmarkPersistency.hpp"


int main(int argc, char *argv[]){
    BenchmarkPersistency::allThroughputTests();
    return 0;
}

