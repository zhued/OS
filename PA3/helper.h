// Required by for routine
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h> 
#include <string.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>     /* atoi */


int forkThis(int numOfFork)
{
  pid_t pid;
  int ii; 
  for(ii = 0; ii < numOfFork; ii++)
  {
    pid = fork();
    if(pid == 0)
    {
      break; 
    }
  }
  if(pid == 0)
  {
    printf("New Process %d\n", getpid());
    return getpid(); 
  }
  else
  {
    printf("Parent Process\n"); 
    while ((pid = waitpid(-1, NULL, 0))) {
	if (errno == ECHILD) {
	  break;
	}
    }
    printf("Done with all processes\n"); 
    return 0; 
  }
  return 0; 
}
