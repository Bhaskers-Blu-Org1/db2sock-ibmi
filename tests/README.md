#db2sock/tests

==Experimental==

c tests. 

# compile

```
optional (git already completed) ...
$ cd db2sock/tests
$ python genmake.py

gcc compile ...
$ cd db2sock/tests
$ ./make_tests.sh
```

# run
```
> export SQL_HOST400=myibmi
> export SQL_DB400=*LOCAL
> export SQL_UID400=MYUID
> export SQL_PWD400=MYPWD


One at a time ...
> test0004_async_callback_query_32
> test0004_async_callback_query_64

Run all ...
> python run.py

```

#debug
gdb is gcc environment debugger.


However, if you wish to use dbx, 
dbx can produce 'internal error' on gcc objects.
I use the following technique to keep the mess out.
```
bash-4.3$ cd test

====
run program ...
====
bash-4.3$ export SQL_DB400=*LOCAL
bash-4.3$ export SQL_UID400=MYID
bash-4.3$ export SQL_PWD400=MYPWD
bash-4.3$ dbx  -I. -I.. test0003_async_callback_connect_32 2>&1 | grep -v 'internal error'
(press enter)
(dbx)

====
attach (eample test using lang_wait_init)...
====

bash-4.3$ export SQL_DEBUG400=Y
bash-4.3$ test0003_async_callback_connect_32&
[1] 8216
bash-4.3$ --DEBUG MODE--
Long sleep, attach pid 8216
--DEBUG MODE--

bash-4.3$ dbx -I. -I.. -a 8216 2>&1 | grep -v 'internal error'                  
Waiting to attach to process 8216 ...
Successfully attached to /home/monoroot/libdb400/tests/./test0003_async_callback_connect_32.
Type 'help' for help.

stopped in _p_nsleep at 0xd1c0bdf0 ($t1)
0xd1c0bdf0 (_p_nsleep+0x10) 80410014         lwz   r2,0x14(r1)

(dbx) 
where
(dbx) _p_nsleep(??, ??) at 0xd1c0bdf0
raise.nsleep(??, ??) at 0xd16bd144
sleep(??) at 0xd17b76e8
lang_wait_done(loop = 1, secs = 1), line 79 in "LangCommon.c"
lang_wait_init(), line 41 in "LangCommon.c"
main(argc = 1, argv = 0x2ff22cc8), line 57 in "test0003_async_callback_connect.c"

```

#debug gcc (my way)

Here is my python wrap for dbx.
```
====
pdbx -- python wrap dbx (less noise)
====
bash-4.3$ cp pdbx /QOpenSys/usr/bin/.

bash-4.3$ cat /QOpenSys/usr/bin/pdbx 
#!/opt/freeware/bin/python
# syntax: pdbx test
import sys, os, time
import subprocess
proc = subprocess.Popen(['dbx','-d 100','-I .','-I ..',sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
while True:
  nextline = proc.stdout.readline()
  if nextline == '' and proc.poll() is not None:
    break
  if 'internal error' in nextline:
    continue
  if 'dbx)' in nextline:
    newer = nextline.split('dbx) ');
    nextline = newer[1]
  sys.stdout.write(nextline)
  sys.stdout.flush()
  time.sleep(0.05)


=====
example
=====
bash-4.3$ pdbx test1001_normal_query_json_32
Type 'help' for help.
reading symbolic information ...warning: test1001_normal_query_json.c is newer than test1001_normal_query_json_32
warning: LangCommon.c is newer than ./liblang400.a

list 36
   36     sqlrc = SQL400Json(injson, inlen, outjson, outlen);
stop at 36
[1] stop at "test1001_normal_query_json.c":36
cont
[1] stopped in main at line 36 in file "test1001_normal_query_json.c" ($t1)
   36     sqlrc = SQL400Json(injson, inlen, outjson, outlen);
s
stopped in SQL400Json at line 9863 in file "../PaseCliAsync_gen.c" ($t1)
 9863   {
s
stopped in SQL400Json at line 9864 in file "../PaseCliAsync_gen.c" ($t1)
 9864     SQLRETURN sqlrc = SQL_SUCCESS;
s
stopped in SQL400Json at line 9866 in file "../PaseCliAsync_gen.c" ($t1)
 9866     sqlrc = custom_SQL400Json( injson, inlen, outjson, outlen );
q

=====
example attach running process
=====

bash-4.3$ test1004_async_callback_query_json_32 &
[1] 13629
bash-4.3$ --DEBUG MODE--
Long sleep, attach pid 13629
--DEBUG MODE--

bash-4.3$ pdbx '-a 13629'
Waiting to attach to process 13629 ...
Successfully attached to /home/monoroot/db2sock/tests/./test1004_async_callback_query_json_32.
Type 'help' for help.
reading symbolic information ...warning: test1004_async_callback_query_json.c is newer than /home/monoroot/db2sock/tests/./test1004_async_callback_query_json_32
warning: LangCommon.c is newer than ./liblang400.a

stopped in _p_nsleep at 0xd1c0bdf0 ($t1)
0xd1c0bdf0 (_p_nsleep+0x10) 80410014         lwz   r2,0x14(r1)


```

# note
Your LIBPATH should NOT be set /opt/freeware first (below).
However, when compiling i often set /opt/freeware first.
So i usually use two ssh sessions (one compile, one test).

Makefiles removed /opt/freeware binaries (see Makefile).
At runtime (test time), we want only PASE binaries and new libdb400.a.

```
bash-4.3$ cd tests

== bad libpath runtime ===

bash-4.3$ echo $LIBPATH
.:/opt/freeware/lib:/usr/lib
bash-4.3$ test0003_async_callback_connect_64
Could not load program test0003_async_callback_connect_64:
Could not load module /home/monoroot/libdb400/tests/./libdb400.a(shr_64.o).
        Dependent module /opt/freeware/lib/libiconv.a(shr4_64.o) could not be loaded.
        Member shr4_64.o is not found in archive 
Could not load module st0003_async_callback_connect_64.
        Dependent module /home/monoroot/libdb400/tests/./libdb400.a(shr_64.o) could not be loaded.
Could not load module .

== good LIBPATH set ===

bash-4.3$ export LIBPATH=.:/QOpenSys/usr/lib:/opt/freeware
bash-4.3$ test0003_async_callback_connect_64              
SQL400ConnectAsync (thread 258): connect running
SQL400ConnectAsync (thread 1): hi there from main thread
SUCCESS: tid valid) expect=1 actual=258
SQL400ConnectCallback (thread 258): hi there from callback thread
SQL400ConnectCallback (thread 258): complete: sqlrc=0, henv=1, db= uid= pwd=xxxx *ohnd=2 options=1800015a0 callback=180001248
SQL400ConnectCallback (thread 258): leaving
SUCCESS: operation complete) expect=1 actual=1

== good PASE default ==

bash-4.3$ unset LIBPATH
bash-4.3$ echo $LIBPATH

bash-4.3$ test0003_async_callback_connect_64
SQL400ConnectAsync (thread 258): connect running
SQL400ConnectAsync (thread 1): hi there from main thread
SUCCESS: tid valid) expect=1 actual=258
SQL400ConnectCallback (thread 258): hi there from callback thread
SQL400ConnectCallback (thread 258): complete: sqlrc=0, henv=1, db= uid= pwd=xxxx *ohnd=2 options=1800015a0 callback=180001248
SQL400ConnectCallback (thread 258): leaving
SUCCESS: operation complete) expect=1 actual=1


```

