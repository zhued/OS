/*
 * File: pi-sched.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 2012/03/07
 * Modify Date: 2012/03/09
 * Description:
 * 	This file contains a simple program for statistically
 *      calculating pi using a specific scheduling policy.
 */

/* Include Flags */
#define _GNU_SOURCE

/* Local Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


#define MAXFILENAMELENGTH 80
#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)
#define DEFAULT_PROCESSES 5
#define USAGE "./cpu-bound ITERATIONS SCHED_POLICY PROC_COUNT\n"
#define DEFAULT_TRANSFERSIZE 1024*100

inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}


int main(int argc, char* argv[]){

    long i;
    long iterations;
    struct sched_param param;
    int policy;
    int numprocess;
    int child_status;
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;
    
    //~ ssize_t writtenData = 0;
    //~ char* transferBuffer = NULL;
    //~ ssize_t blocksize = 0; 
    //~ ssize_t buffersize;
    
    //~ int rv;
    //~ int outputFD;
    //~ char outputFilename[MAXFILENAMELENGTH];
    //~ char outputFilenameBase[MAXFILENAMELENGTH];

    /* Process program arguments to select iterations and policy */
    /* Set default iterations if not supplied */
    if(argc < 2){
	iterations = DEFAULT_ITERATIONS;
    }
    /* Set default policy if not supplied */
    if(argc < 3){
	policy = SCHED_OTHER;
    }
    /* Set iterations if supplied */
    if(argc > 1){
	iterations = atol(argv[1]);
	if(iterations < 1){
	    fprintf(stderr, "Bad iterations value\n");
	    exit(EXIT_FAILURE);
	}
    }
    /* Set policy if supplied */
    if(argc > 2){
	if(!strcmp(argv[2], "SCHED_OTHER")){
	    policy = SCHED_OTHER;
	}
	else if(!strcmp(argv[2], "SCHED_FIFO")){
	    policy = SCHED_FIFO;
	}
	else if(!strcmp(argv[2], "SCHED_RR")){
	    policy = SCHED_RR;
	}
	else{
	    fprintf(stderr, "Unhandeled scheduling policy\n");
	    exit(EXIT_FAILURE);
	}
    }
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);
    
    /* Set new scheduler policy */
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    if(sched_setscheduler(0, policy, &param)){
	perror("Error setting scheduler policy");
	exit(EXIT_FAILURE);
    }
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

	if(argc > 3) {
        numprocess = atol(argv[3]);
        if( numprocess < 1 || numprocess > 5000 ){
            fprintf(stderr, "There will be %d children.", numprocess);
            fprintf(stderr, USAGE);
            exit(EXIT_FAILURE);
        }
    } else {
        numprocess = DEFAULT_PROCESSES;
    }
    
    //~ /* Allocate buffer space */
    //~ buffersize = blocksize;
    //~ if(!(transferBuffer = malloc(buffersize*sizeof(*transferBuffer)))){
	//~ perror("Failed to allocate transfer buffer");
	//~ exit(EXIT_FAILURE);
    //~ }
    
    FILE *f = fopen("junk.txt", "w");
			if(f==NULL){
				printf("Error opening file \n");
				exit(1);
			}
    
    int k = 0; 
	for (k = 0; k < numprocess; k++){
		if (fork() == 0){
			for(i=0; i<iterations; i++){
				x = (random() % (RADIUS * 2)) - RADIUS;
				y = (random() % (RADIUS * 2)) - RADIUS;
				if(zeroDist(x,y) < RADIUS){
					inCircle++;
				}
				inSquare++;
			}
			pCircle = inCircle/inSquare;
			piCalc = pCircle * 4.0;
			fprintf(stdout, "pi = %f\n", piCalc);

			fprintf(f, "%e\n", pCircle);
			fprintf(f, "%e\n", piCalc);

			//~ /* Open Output File Descriptor in Write Only mode with standard permissions*/
			//~ rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
				  //~ outputFilenameBase, getpid());    
			//~ if(rv > MAXFILENAMELENGTH){
			//~ fprintf(stderr, "Output filenmae length exceeds limit of %d characters.\n",
				//~ MAXFILENAMELENGTH);
			//~ exit(EXIT_FAILURE);
			//~ }
			//~ else if(rv < 0){
			//~ perror("Failed to generate output filename");
			//~ exit(EXIT_FAILURE);
			//~ }
			//~ if((outputFD =
			//~ open(outputFilename,
				 //~ O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
				 //~ S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
			//~ perror("Failed to open output file");
			//~ exit(EXIT_FAILURE);
			//~ }
			//~ /* Write to file*/
			//~ writtenData = write(outputFD, transferBuffer, pCircle);
				//~ if(writtenData < 0){
				//~ perror("Error writing output file");
				//~ exit(EXIT_FAILURE);
				//~ }
				//~ 
			//~ /* Free Buffer */
			//~ free(transferBuffer);
			//~ 
			//~ /* Close Output File Descriptor */
			//~ if(close(outputFD)){
			//~ perror("Failed to close output file");
			//~ exit(EXIT_FAILURE);
			//~ }

			return 0;
		}
	}
	fclose(f);
	
	int j;  
	for (j = 0; j < numprocess; j++) {
	pid_t wpid = wait(&child_status);

		if (!WIFEXITED(child_status)){
			printf("Child %d terminated abnormally\n", wpid);
		}
	}
	return 0;

}

