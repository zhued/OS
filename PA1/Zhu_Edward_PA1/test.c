#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h> /* SYS_xxx definitions */
#include <stdio.h>

int main(){
printf("Entered main function…!\n");

/*here starts system call*/
syscall(314);

printf("System call invoked, to see type command: dmesg at your terminal\n");
printf("Exiting main….!\n");

return 0;
}
