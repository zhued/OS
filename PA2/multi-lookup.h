
#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

int Checker(char* inputFileName, int* currentPosition);
/* Function to do all requests */
void *Requester(void* inputFilePath);
/* Does the resolving of the addresses */
void *Resolver(); 
/* Initializes queue and mutexes */
void initAll();
/* Cleans up queue and mutexes */ 
void freeAll();


#endif
