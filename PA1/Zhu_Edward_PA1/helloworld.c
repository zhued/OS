/* This file defines the HelloWorld System call */

#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage int sys_helloworld(){
	printk(KERN_EMERG "edzh3452 says Hello World!\n");
	return 0;
}
