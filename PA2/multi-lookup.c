/*******************************
 * Edward Zhu
 *******************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "multi-lookup.h"
#include "util.h"
#include "queue.h"

#define PRINT_DEBUG 0
//~ #define NUMBER_OF_RESOLVERS 3

int resolver_running = 1;
char* outputFile;

queue hostQ;
pthread_mutex_t queueM;
pthread_mutex_t outputM;


int main(int argc, char* argv[]){
	int fileCount = argc-2;
	outputFile = argv[(argc-1)];
	
	pthread_t resolverThread[MAX_RESOLVER_THREADS];
	pthread_t requesterThread[fileCount];
	
	queue_init(&hostQ, QUEUEMAXSIZE);
	pthread_mutex_init(&outputM, NULL);
	pthread_mutex_init(&queueM, NULL);
	
	FILE* inputfp[fileCount];
	FILE* outputfp;
	
	int i;
	int ret;
	
	/* Check Arguments copied from lookup.c*/
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }
	
	/* Input file copied from lookup.c*/
	for(i=0; i<fileCount; ++i){
		inputfp[i] = fopen(argv[i+1], "r");
			if(!inputfp[i]){
				fprintf(stderr, "Failed input file: %s\n", argv[i+1]);
				perror("Failed to open input file");
				return EXIT_FAILURE;
			}
	}
	
	/* Output file copied from lookup.c*/
	outputfp = fopen(argv[argc-1], "w");
    if(!outputfp){
		fprintf(stderr, "Failed input file: %s\n", argv[argc-1]);
		perror("Failed to open output file");
		return EXIT_FAILURE;
    }
    
    /* Extra Credit*/
	int cores = sysconf( _SC_NPROCESSORS_ONLN );
	int threads;
	if(cores <= MAX_RESOLVER_THREADS && cores >= MIN_RESOLVER_THREADS){
		threads = cores;
	}else if (cores > MAX_RESOLVER_THREADS){
		threads = MAX_RESOLVER_THREADS;
	}else if (cores < MIN_RESOLVER_THREADS){
		threads = MIN_RESOLVER_THREADS;
	}
	
	#if PRINT_DEBUG == 1 
	printf("Num cores available: %d\n", cores);
	printf("Thread: %d\n", threads);
	#endif
    
	
	/* Create and start request threads */
    printf("Getting domain names...\n");
	for(i=0; i<fileCount; i++){
		#if PRINT_DEBUG == 1 
		printf("File: %s\n", argv[i+1]);
		#endif
		ret = pthread_create(&(requesterThread[i]), NULL, Requester, (void*) argv[i+1]);
		if (ret){
			fprintf(stderr, "Error: pthread_create returned %d\n", ret);
			exit(EXIT_FAILURE);
		}
		#if PRINT_DEBUG == 1 
		//~ printf("Doing this: (pthread id %d)\n", requesterThread[i]); 
		#endif
	}

	/* Create and start resolve threads */
	printf("Resolving names to IP addresses...\n");
        //~ for(i=0;i<NUMBER_OF_RESOLVERS;++i){
		for(i=0;i<threads;++i){
			ret = pthread_create(&(resolverThread[i]), NULL, Resolver, NULL);
			if(ret != 0){
					fprintf(stderr, "Error: pthread_create returned %d\n", ret);
					return EXIT_FAILURE;
			}      
			#if PRINT_DEBUG == 1 
			//~ printf("Doing this: (pthread id %d)\n", resolverThread[i]); 
			#endif
        }
    #if PRINT_DEBUG == 1 
    printf("Resolving Done...\n");
	#endif
	
	/* Wait for the requesters to finish */
	for(i=0;i<fileCount;i++){
		int join = pthread_join(requesterThread[i],NULL);
		if (join != 0){
				printf("Joining pthreads error.\n");
		}
	}
    
    #if PRINT_DEBUG == 1 
    printf("joining requesting done...\n");
	#endif
	resolver_running = 0;
	
    /* Wait for the resolvers to finish */
	//~ for(i=0;i<NUMBER_OF_RESOLVERS;i++){
	for(i=0;i<threads;i++){
		int join = pthread_join(resolverThread[i],NULL);
		if (join != 0){
				printf("Joining pthreads error.\n");
		}
	}
        
   printf("All of the threads were completed!\n");
 
    /* Free memory and close files */
	pthread_mutex_destroy(&queueM);
	pthread_mutex_destroy(&outputM);
	queue_cleanup(&hostQ); 
    for(i=0;i<fileCount;++i){
		fclose(inputfp[i]);
    }
    fclose(outputfp);
	return 0;
}


void* Requester(void* args){
	char* inputfile = args;
	FILE* inputfp = NULL;
	inputfp = fopen((char*)args, "r");
	char* host = malloc(SBUFSIZE*sizeof(char));
	char* errorstr = malloc(SBUFSIZE*sizeof(char));
	
	if(!inputfp){
		sprintf(errorstr, "Error Opening Input File: %s", inputfile);
		perror(errorstr);
		exit(EXIT_FAILURE);
	}
	
	#if PRINT_DEBUG == 1
	printf("File: %s\n", inputfile);
	#endif
	
	/* Read File and Process*/
	while(fscanf(inputfp, INPUTFS, host) > 0){
		#if PRINT_DEBUG == 1 
		printf("Pushing Host: %s\n", host); 
		#endif
		int queueStatus = QUEUE_FAILURE;
		/* Duplicate string of host */
		char * string = strdup(host);
		
		/* If queue full, wait for empty */
		while(queueStatus == QUEUE_FAILURE){
			pthread_mutex_lock(&queueM); 
			  /* Add to hostQ */
			queueStatus = queue_push(&hostQ, (void*) string); 
			pthread_mutex_unlock(&queueM);
			if(queueStatus == QUEUE_FAILURE){ 
				usleep(rand()%100); //Sleep between 0 and 100 microseconds
			}
		}
	}
	/* Close and Free */
	fclose(inputfp);
	free(host); 
	free(errorstr);
	return NULL;
}


void* Resolver(){
	FILE* outputfp = NULL;
	char* host = NULL;
	char firstipstr[INET6_ADDRSTRLEN];
	
	while(resolver_running || !queue_is_empty(&hostQ)){
		pthread_mutex_lock(&queueM);
		host = queue_pop(&hostQ); 
		pthread_mutex_unlock(&queueM);
		/* Look up with dnslookup */
		if(host != NULL){
			#if PRINT_DEBUG == 1 
			printf("Looking up hostname: %s\n", host); 
			#endif
			if(dnslookup(host, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
				fprintf(stderr, "dnslookup error: %s\n", host);
				strncpy(firstipstr, "", sizeof(firstipstr));
			}
			
			/* Write to Output File */
			pthread_mutex_lock(&outputM);
			outputfp = fopen(outputFile, "a");
			if(!outputfp){
				perror("Error Opening Output File");
				exit(EXIT_FAILURE);
			}
			fprintf(outputfp, "%s,%s\n", host, firstipstr);

			/* Close Output File */
			fclose(outputfp);
			pthread_mutex_unlock(&outputM);
			free(host); 
		} 
	}
	return NULL;
}
