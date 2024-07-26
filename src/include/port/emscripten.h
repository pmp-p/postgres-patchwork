/* src/include/port/emscripten.h */

#ifndef I_EMSCRIPTEN
#define I_EMSCRIPTEN

#include <emscripten.h>

/*
 * https://github.com/electric-sql/postgres-wasm/pull/1/files#diff-6e542ba2eb1d83ef90e65cdc0912b51a295184701c7e3bd236937c43c4cac4b9R63
 */

#define WAIT_USE_POLL 1

/* internal size 8 is invalid for passed-by-value type */
/* #define USE_FLOAT8_BYVAL 1 */

#define HAVE_LINUX_EIDRM_BUG
/*
 * Set the default wal_sync_method to fdatasync.  With recent Linux versions,
 * xlogdefs.h's normal rules will prefer open_datasync, which (a) doesn't
 * perform better and (b) causes outright failures on ext4 data=journal
 * filesystems, because those don't support O_DIRECT.
 */
#define PLATFORM_DEFAULT_WAL_SYNC_METHOD	WAL_SYNC_METHOD_FDATASYNC

// force the name used with --single
#define WASM_USERNAME "postgres"

/* reduce size */
#define PG_FORCE_DISABLE_INLINE

// we want client and server in the same lib for now.

extern int	pg_char_to_encoding_private(const char *name);
extern const char *pg_encoding_to_char_private(int encoding);
extern int	pg_valid_server_encoding_id_private(int encoding);

#if defined(pg_char_to_encoding)
#undef pg_char_to_encoding
#endif
#define pg_char_to_encoding(encoding) pg_char_to_encoding_private(encoding)

#if defined(pg_encoding_to_char)
#undef pg_encoding_to_char
#endif
#define pg_encoding_to_char(encoding) pg_encoding_to_char_private(encoding)

#if defined(pg_valid_server_encoding_id)
#undef pg_valid_server_encoding_id
#endif
#define pg_valid_server_encoding_id(encoding) pg_valid_server_encoding_id_private(encoding)


/*
 * 'proc_exit' is a wasi system call, so change its name everywhere.
 */

#define proc_exit(arg) pg_proc_exit(arg)


/*
 * popen / pclose for initdb is routed to stderr
 * link a pclose replacement when we are in exec.c ( PG_EXEC defined )
 */

#if defined(PG_EXEC)
#define pclose(stream) pg_pclose(stream)
#include <stdio.h> // FILE

int pg_pclose(FILE *stream) {
    puts("FIXME: pclose->pg_pclose: " __FILE__);
    return 0;
}


#endif // PG_EXEC

/* and now popen will return stderr as file handle in initdb.c */
#if defined(PG_INITDB)
#define popen(command, mode) pg_popen(command, mode)
#include <stdio.h> // FILE
FILE *pg_popen(const char *command, const char *type) {
	fprintf(stderr,"# popen[%s]\n", command);
	return stderr;
}
#endif // PG_INITDB


/*
 *  handle pg_shmem.c special case
 */

#if defined(PG_SHMEM)
#include <stdio.h>  // print
#include <stdlib.h> // malloc
// #include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>

/*
 * Shared memory control operation.
 */

//extern int shmctl (int __shmid, int __cmd, struct shmid_ds *__buf);

int
shmctl (int __shmid, int __cmd, struct shmid_ds *__buf) {
	printf("FIXME: int shmctl (int __shmid=%d, int __cmd=%d, struct shmid_ds *__buf=%p)\n", __shmid, __cmd, __buf);
	return 0;
}


void *FAKE_SHM ;
key_t FAKE_KEY = 0;

/* Get shared memory segment.  */
// extern int shmget (key_t __key, size_t __size, int __shmflg);
int
shmget (key_t __key, size_t __size, int __shmflg) {
	printf("# FIXING: int shmget (key_t __key=%d, size_t __size=%d, int __shmflg=%d)\n", __key, __size, __shmflg);
	if (!FAKE_KEY) {
		FAKE_SHM = malloc(__size);
		FAKE_KEY = 666;
		return FAKE_KEY;
	} else {
		printf("# ERROR: int shmget (key_t __key=%d, size_t __size=%d, int __shmflg=%d)\n", __key, __size, __shmflg);
		abort();
	}
	return -1;
}

/* Attach shared memory segment.  */
// extern void *shmat (int __shmid, const void *__shmaddr, int __shmflg);
void *shmat (int __shmid, const void *__shmaddr, int __shmflg) {
	printf("# FIXING: void *shmat (int __shmid=%d, const void *__shmaddr=%p, int __shmflg=%d)\n", __shmid, __shmaddr, __shmflg);
	if (__shmid==666)
		return FAKE_SHM;
	return NULL;
}

/* Detach shared memory segment.  */
// extern int shmdt (const void *__shmaddr);
int
shmdt (const void *__shmaddr) {
	puts("# FIXME: int shmdt (const void *__shmaddr)");
	return 0;
}

#endif // PG_SHMEM
#endif // I_EMSCRIPTEN
