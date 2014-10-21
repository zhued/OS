/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.


	EDWARD ZHU FINAL REVISION
*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#define ENC_DATA ((struct enc_state *) fuse_get_context()->private_data)

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 700
/* For open_memstream() */
#define _POSIX_C_SOURCE 200809L
#endif

#include "aes-crypt.h"
#include <linux/limits.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define USAGE "Usage:\n\t./pa4-encfs KEY ENCYPTION_DIRECTORY MOUNT_POINT\n"

#define ENCRYPT 1
#define DECRYPT 0
#define PASS -1

#define ENC_ATTR "user.pa4-encfs.encrypted"
#define ENCRYPTED "true"
#define UNENCRYPTED "false"

// data structure that stores root directory and passphrase
struct enc_state {
	char *rootdir;
	char *key;
};

// this function is directly related to the bbfs implementation listed in the pdf
// makes the full path and not just the root dir
static void xmp_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, ENC_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	
	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;
	char flink[PATH_MAX];
	xmp_fullpath(flink, to);
	res = symlink(from, flink);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];
	xmp_fullpath(fpath, from);
	xmp_fullpath(fnewpath, to);
	res = rename(fpath, fnewpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];
	xmp_fullpath(fpath, from);
	xmp_fullpath(fnewpath, to);
	res = link(fpath, fnewpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	// would also have to do stuff in here, decrypt then truncate
	// then encrypt	
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;
		
	close(res);
	
	fi->fh = res;
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	// initialize file and temp files for memory storage
	// also xattr data information 
	FILE *file;
	FILE *tempfile;
	char *tempdata;
	size_t tempsize;
	
	char xattrval[8];
	ssize_t xattrlen;
	
	// default action is to pass
	int action = PASS;
	
	int res;
	
	//fuse to make directory not root
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);

	(void) fi;
	
	// open file
	file = fopen(fpath, "r");
	if (file == NULL)
		return -errno;
		
	// opem memory stream
	tempfile = open_memstream(&tempdata, &tempsize);
	if (tempfile == NULL)
		return -errno;

	// check if decrypting or now, if it is encrypted then we do
	// otherwise do nothing
	xattrlen = getxattr(fpath, ENC_ATTR, xattrval, 8);
	if(xattrlen != -1 && strcmp(xattrval, ENCRYPTED) == 0) {
		action = DECRYPT;
	}
	
	do_crypt(file, tempfile, action, ENC_DATA->key);
	fclose(file);

	// flush/write temp file in memory in user space
	fflush(tempfile);
	// seek according to the offset from memory
	fseek(tempfile, offset, SEEK_SET);
	
	// open up temp file and read
	res = fread(buf, 1, size, tempfile);
	if (res == -1)
		res = -errno;
	
	// close temp file
	fclose(tempfile);

	return res;

}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int res;
	// initialize everything like before
	FILE *file; 
	FILE *tempfile;
	char *tempdata;
	size_t tempsize;

	char xattr_value[6];
	ssize_t xattr_length;
	
	// fuse to mirror directory instead of root, and mostly everything 
	// is the same afterwards here
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);

	int action = PASS;

	(void) fi;

	file = fopen(fpath, "r");
	if (file == NULL){
		return -errno;
	}

	tempfile = open_memstream(&tempdata, &tempsize);
	if (tempfile == NULL) {
		return -errno;
	}
	
	// if its encrypted, then we decrypt and close the file
	xattr_length = getxattr(fpath, ENC_ATTR, xattr_value, 6);
	if(xattr_length != -1 && strcmp(xattr_value, ENCRYPTED) == 0) {
		action = DECRYPT;
	}

	do_crypt(file, tempfile, action, ENC_DATA->key);
	fclose(file);

	// give extra bytes in buffer to memory file
	fseek(tempfile, offset, SEEK_SET);
	res = fwrite(buf, 1, size, tempfile);
	if (res == -1)
		res = -errno;
	fflush(tempfile);

    // open file again, encrypt from the temp mem file into file
	file = fopen(fpath, "w");
	fseek(tempfile, 0, SEEK_SET);
	do_crypt(tempfile, file, ENCRYPT, ENC_DATA->key);

	//close all files
	fclose(tempfile);
	fclose(file);

	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);
	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
	// initialize everything
	FILE *file;
	FILE *tempfile;
    char *tempdata;
	size_t tempsize;
	
	// fuse thing
	char fpath[PATH_MAX];
	xmp_fullpath(fpath, path);

	(void) fi;
	(void) mode;

	// create file
	file = fopen(fpath, "w");
	if(file == NULL){
		return -errno;
	}
	// put file into memstream
	tempfile = open_memstream(&tempdata, &tempsize);
	if(tempfile == NULL){
		return -errno;
	}
	// set attributes to encrypted
	if(setxattr(fpath, ENC_ATTR, ENCRYPTED, sizeof(ENCRYPTED), 0) == -1){
		return -errno;
	}
	// encrypt from temp file and push into regular file
	do_crypt(tempfile, file, ENCRYPT, ENC_DATA->key);
	//close all
	fclose(tempfile);
	fclose(file);

    return 0;

}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char fpath[PATH_MAX];
    xmp_fullpath(fpath, path);
	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char fpath[PATH_MAX];
    xmp_fullpath(fpath, path);
	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char fpath[PATH_MAX];
    xmp_fullpath(fpath, path);
	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char fpath[PATH_MAX];
    xmp_fullpath(fpath, path);
	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	struct enc_state * enc_data;
	// copied from the bbfs implementation
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running ENCFS as root opens unnacceptable security holes\n");
		return 1;
    }
	// if less than 4 things, error
	if ((argc < 4) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
		fprintf(stderr, USAGE);
	}
	
	// malloc space for the data
	enc_data = malloc(sizeof(struct enc_state));
	if (enc_data == NULL){
		perror("Error");
		exit(1);
	}
	// make the directory not root
	enc_data->rootdir = realpath(argv[argc-2], NULL);
	
	//make key and then make key as the first value to plug into fuse_main
	enc_data->key = argv[argc-3];
	argv[argc-3] = argv[argc-1];
    argv[argc-2] = NULL;
    argv[argc-1] = NULL;
    argc -= 2;
    
    umask(0);
	//give to fuse
	fprintf(stderr, "about to call fuse_main\n");
    return fuse_main(argc, argv, &xmp_oper, enc_data);
    fprintf(stderr, "Fused finished\n");
}
