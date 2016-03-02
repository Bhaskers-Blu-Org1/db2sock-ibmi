#ifndef _PASECLIINIT_H
#define _PASECLIINIT_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <iconv.h>
#include <as400_types.h>
#include <as400_protos.h>
#include "PaseCliLic.h"

#ifdef __64BIT__
#define PASECLIDRIVER "/QOpenSys/QIBM/ProdData/OS400/PASE/lib/libdb400.a(shr_64.o)"
#else
#define PASECLIDRIVER "/QOpenSys/QIBM/ProdData/OS400/PASE/lib/libdb400.a(shr.o)"
#endif

#define DB2CLISRVPGM "QSYS/QSQCLI"

#define PASECLIMAXRESOURCE 33000
typedef struct PaseCliResource {
  int hstmt;                       /* index hstmt or hdbc   */
  int hdbc;                        /* index to hdc (above)  */ 
  pthread_mutex_t threadMutexLock; /* lock hdbc or hstmt    */
  pthread_mutexattr_t threadMutexAttr; /* recursive lock    */
  int in_progress;                 /* operation in progress */
  char *hKey;                      /* persistent key        */
} PaseCliResource;

#define PASECLIMAXCCSID 3000
typedef struct PaseConvResource {
  int cssid_Ascii;
  int ccsid_Utf;
  char * charset_Ascii;
  char * charset_Utf;
  iconv_t AsciiToUtf;
  iconv_t UtfToAscii;
  int in_progress;
  pthread_mutex_t threadMutexLock;
  pthread_mutexattr_t threadMutexAttr;
} PaseConvResource;

/* load */
void * init_cli_dlsym();
int init_cli_srvpgm();
int init_CCSID400( int newCCSID );

/* hdbc/hstmt scope locking */
void init_table_ctor(int hstmt, int hdbc);
void init_table_dtor(int hstmt);
void init_table_lock(int hstmt,int flag);
void init_table_unlock(int hstmt,int flag);
int init_table_in_progress(int hstmt,int flag);

/* persistent connection */
void init_table_add_hash(int hstmt, char * db, char * uid, char * pwd, char * qual, int flag);
int init_table_hash_2_conn(char * db, char * uid, char * pwd, char * qual);
void init_table_add_hash_W(int hstmt, unsigned int * db, unsigned int * uid, unsigned int * pwd, unsigned int * qual, int flag);
int init_table_hash_2_conn_W(unsigned int * db, unsigned int * uid, unsigned int * pwd, unsigned int * qual);
int init_table_hash_active(int hstmt, int flag);

/* utilities */
int custom_strlen_utf16(unsigned int * src);
void * custom_alloc_zero(int sz);

/* conversion */
int custom_iconv(int isInput, char *fromBuffer, char *toBuffer, size_t sourceLen, size_t bufSize, int myccsid, int utfccsid);

#endif /* _PASECLIINIT_H */

