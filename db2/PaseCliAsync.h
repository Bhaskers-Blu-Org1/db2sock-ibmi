#ifndef _PASECLIASYNC_H
#define _PASECLIASYNC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sqlcli1.h>
#include <as400_types.h>
#include <as400_protos.h>

#ifdef __cplusplus
extern "C" {
#endif
/* ===================================================
 * Using the driver
 * ===================================================
 */

/* 
 * SQL400Environment -- set the mode of driver
 * 
 * === UTF-8 mode, most popular AIX/PASE/Linux ===
 * SQLOverrideCCSID400(1208) -- UTF-8 mode, CLI normal/async direct ILE call
 * -->SQLExecDirect(Async)-->ILE_SQLExecDirect-->DB2
 * -->SQLExecDirectW(Async)-->ILE_SQLExecDirectW-->DB2
 * 
 * === UTF-8 mode, most popular Windows/Java ===
 * SQLOverrideCCSID400(1200) -- UTF-16 mode, CLI normal/async direct ILE call
 * -->SQLExecDirect(Async)-->ILE_SQLExecDirectW-->DB2
 * -->SQLExecDirectW(Async)-->ILE_SQLExecDirectW-->DB2
 * 
 * === PASE default (original libdb400.a) ===
 * SQLOverrideCCSID400(other) -- PASE ccsid, CLI API calls PASE libdb400.a
 * -->SQLExecDirect(Async)-->PASE libdb400.a(SQLExecDirect)-->DB2
 * -->SQLExecDirectW(Async)-->ILE_SQLExecDirectW-->DB2 (*)
 * 
 * PASE ccsid setting occurs before any other CLI operations
 * at the environment level. Therefore the driver mode is
 * established (restricted, if you choose). 
 * 
 * 1) non-Unicode interfaces (call old libdb400.a, messy stuff):
 *   int ccsid = 819;
 *   env attr SQL400_ATTR_PASE_CCSID &ccsid -- set pase ccsid
 * 
 * 2) UTF8 interfaces (call direct to ILE DB2):
 *   int ccsid = 1208;
 *   env attr SQL400_ATTR_PASE_CCSID &ccsid -- set pase ccsid
 *   if ccsid == 1208:
 *     env attr SQL_ATTR_UTF8 &true -- no conversion required by PASE
 * 
 * 3) UTF16 wide interfaces (call direct to ILE DB2):
 *   **** NEVER set PASE_CCSID or ATTR_UTF8 in UTF-16 mode. ****
 *   So, database exotic ebcdic column and PASE binds c type as WVARCHAR/WCHAR output is UTF16?
 *   Yes, the database will do the conversion from EBCDIC to UTF16 for data bound as WVARCHAR/WCHAR.
 *   not sure about DBCLOB -- I want to guess that is bound as UTF-16, not 100% sure.
 * 
 * IF your data not UTF-8 or UTF-16, interfaces to convert (experimental).
 *   SQL400ToUtf8    -- use before passing to CLI normal interfaces
 *   SQL400FromUtf8  -- use return output normal CLI (if needed)
 *   SQL400ToUtf16   -- use before passing to CLI wide interfaces
 *   SQL400FromUtf16 -- use return output wide CLI (if needed)
 * 
 * A choice of exported APIs (SQLExecDirect one of many):
 * 
 * === CLI APIs UTF-8 or APIWs UTF-16  ===
 * === choose async and/or normal wait ===
 * SQLRETURN SQLExecDirect(..);
 * SQLRETURN SQLExecDirectW(..);
 * 
 * == callback or reap/join with async ===
 * pthread_t SQLExecDirectAsync(..);
 * pthread_t SQLExecDirectWAsync(..);
 * void SQLExecDirectCallback(SQLExecDirectStruct* );
 * SQLExecDirectStruct * SQLExecDirectJoin (pthread_t tid, SQLINTEGER flag);
 * void SQLExecDirectWCallback(SQLExecDirectWStruct* );
 * SQLExecDirectWStruct * SQLExecDirectWJoin (pthread_t tid, SQLINTEGER flag);
 * 
 * === bypass all, call PASE libdb400.a directly  (not recommended) ===
 * SQLRETURN libdb400_SQLExecDirect(..);
 * SQLRETURN libdb400_SQLExecDirectW(..); (*)
 * 
 * === bypass all, call ILE directly (not recommended) ===
 * SQLRETURN ILE_SQLExecDirect(..);
 * SQLRETURN ILE_SQLExecDirectW(..);
 * 
 */

/* ===================================================
 * CUSTOM interfaces
 * ===================================================
 */

/* Use:
 *   SQL400Environment ( ..., SQLPOINTER  options )
 *   SQL400Connect ( ..., SQLPOINTER  options )
 *   SQL400SetAttr ( ..., SQLPOINTER  options )
 * CTOR:
 *   SQL400AddAttr
 * Struct:
 *   SQL400AttrStruct
 *     e - EnvAttr
 *     c - ConnectAttr
 *     s - StmtAttr
 *     o - StmtOption
 */
/* CLI hidden attributes */
#define SQL400_ATTR_PASE_CCSID 10011
#define SQL400_ATTR_CONN_JDBC 10201
/* stop or not on attribute failure */
#define SQL400_ONERR_CONT 1
#define SQL400_ONERR_STOP 2
/* normal attributes */
#define SQL400_FLAG_IMMED 0
/* post connect attributes */
#define SQL400_FLAG_POST_CONNECT 1
#define SQL400_ATTR_CONN_CHGCURLIB 424201
#define SQL400_ATTR_CONN_CHGLIBL 424202
typedef struct SQL400AttrStruct {
 SQLINTEGER scope;   /*      - scope  - SQL_HANDLE_ENV|DBC|SRTMT|DESC */
 SQLHANDLE  hndl;    /* ecso - hndl   - CLI handle                    */
 SQLINTEGER attrib;  /* ecso - attrib - CLI attribute                 */
 SQLPOINTER vParam;  /* ecso - vParam - ptr to CLI value              */
 SQLINTEGER inlen;   /* ecs  - inlen  - len CLI value (string)        */
 SQLINTEGER sqlrc;   /*      - sqlrc  - sql return code               */
 SQLINTEGER onerr;   /*      - onerr  - SQL400_ONERR_xxx              */
 SQLINTEGER flag;    /*      - flag   - SQL400_FLAG_xxx               */
} SQL400AttrStruct;
/* Use:
 *   SQL400Execute( ..., SQLPOINTER parms, SQLPOINTER desc_parms)
 *   SQL400Fetch (..., SQLPOINTER desc_cols)
 * CTOR:
 *   SQL400Describe
 *   SQL400AddCVar
 * Struct:
 *   SQL400ParamStruct
 *   SQL400DescribeStruct
 *     p - SQLDescribeParam
 *     c - SQLDescribeCol
 */
#define SQL400_DESC_PARM 0
#define SQL400_DESC_COL  1
#define SQL400_PARM_IO_FILE 42
typedef struct SQL400ParamStruct {
 SQLSMALLINT icol;            /* icol         - param number      (in)  */
 SQLSMALLINT inOutType;       /* inOutType    - sql C in/out flag (in)  */
 SQLSMALLINT pfSqlCType;      /* pfSqlCType   - sql C data type   (in)  */
 SQLPOINTER  pfSqlCValue;     /* pfSqlCValue  - sql C ptr to data (out) */
 SQLINTEGER * indPtr;         /* indPtr       - sql strlen/ind    (out) */
} SQL400ParamStruct;

typedef struct SQL400DescStruct {
 SQLSMALLINT itype;           /*    - itype        - descr col/parm  (out) */
 SQLSMALLINT icol;            /* pc - icol         - column number   (in)  */
 SQLCHAR *   szColName;       /*  c - szColName    - column name ptr (out) */
 SQLSMALLINT cbColNameMax;    /*  c - cbColNameMax - name max len    (in)  */
 SQLSMALLINT pcbColName;      /*  c - pcbColName   - name size len   (out) */
 SQLSMALLINT pfSqlType;       /* pc - pfSqlType    - sql data type   (out) */
 SQLINTEGER  pcbColDef;       /* pc - pcbColDef    - sql size column (out) */
 SQLSMALLINT pibScale;        /* pc - pibScale     - decimal digits  (out) */
 SQLSMALLINT pfNullable;      /* pc - pfNullable   - null allowed    (out) */
 SQLCHAR     bfColName[1024]; /*  c - bfColName    - column name buf (out) */
} SQL400DescStruct;

/* special SQL400 aggregate functions */
/* do common work for language driver */
/* composite calls to CLI also async  */

SQLRETURN SQL400Version( SQLPOINTER  outversion, SQLINTEGER  outlen );
SQLRETURN SQL400Stmt2Hdbc( SQLHSTMT  hstmt, SQLINTEGER * ohnd );
SQLRETURN SQL400Connect( SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400ConnectW( SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400pConnect( SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400pConnectW( SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400ConnectUtf8( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400pConnectUtf8( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400ConnectUtf16( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400pConnectUtf16( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN SQL400Close( SQLHDBC  hdbc );
SQLRETURN SQL400pClose( SQLHDBC  hdbc );
SQLRETURN SQL400CloseAllStmts( SQLHDBC  hdbc );
SQLRETURN SQL400Cmd( SQLHDBC  hdbc, SQLCHAR * cmd );
SQLRETURN SQL400ChgLibl( SQLHDBC  hdbc, SQLCHAR * libl );
SQLRETURN SQL400ChgCurLib( SQLHDBC  hdbc, SQLCHAR * curlib );
SQLRETURN SQL400ToUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN SQL400FromUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN SQL400ToUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN SQL400FromUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN SQL400IgnoreNullToUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN SQL400IgnoreNullFromUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN SQL400IgnoreNullToUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN SQL400IgnoreNullFromUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN SQL400AnyToAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN SQL400AnyFromAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN SQL400IgnoreNullAnyToAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN SQL400IgnoreNullAnyFromAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN SQL400Json( SQLHDBC  hdbc, SQLCHAR * injson, SQLINTEGER  inlen, SQLCHAR * outjson, SQLINTEGER  outlen );

/* ===================================================
 * NORMAL CLI interfaces
 * ===================================================
 */

/* === sqlcli.h -- normal CLI interfaces (copy) === 
 * SQLRETURN SQLAllocConnect( SQLHENV  henv, SQLHDBC * phdbc );
 * SQLRETURN SQLAllocEnv( SQLHENV * phenv );
 * SQLRETURN SQLAllocHandle( SQLSMALLINT  htype, SQLINTEGER  ihnd, SQLINTEGER * ohnd );
 * SQLRETURN SQLAllocStmt( SQLHDBC  hdbc, SQLHSTMT * phstmt );
 * SQLRETURN SQLBindCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  iType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
 * SQLRETURN SQLBindFileToCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * sLen, SQLINTEGER * pcbValue );
 * SQLRETURN SQLBindFileToParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  iType, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * pcbValue );
 * SQLRETURN SQLBindParam( SQLHSTMT  hstmt, SQLSMALLINT  iparm, SQLSMALLINT  iType, SQLSMALLINT  pType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER * pcbValue );
 * SQLRETURN SQLBindParameter( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fParamType, SQLSMALLINT  fCType, SQLSMALLINT  fSQLType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
 * SQLRETURN SQLCancel( SQLHSTMT  hstmt );
 * SQLRETURN SQLCloseCursor( SQLHSTMT  hstmt );
 * SQLRETURN SQLColAttribute( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
 * SQLRETURN SQLColAttributeW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
 * SQLRETURN SQLColAttributes( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
 * SQLRETURN SQLColAttributesW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLWCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
 * SQLRETURN SQLColumnPrivileges( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
 * SQLRETURN SQLColumnPrivilegesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
 * SQLRETURN SQLColumns( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
 * SQLRETURN SQLColumnsW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
 * SQLRETURN SQLConnect( SQLHDBC  hdbc, SQLCHAR * szDSN, SQLSMALLINT  cbDSN, SQLCHAR * szUID, SQLSMALLINT  cbUID, SQLCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
 * SQLRETURN SQLConnectW( SQLHDBC  hdbc, SQLWCHAR * szDSN, SQLSMALLINT  cbDSN, SQLWCHAR * szUID, SQLSMALLINT  cbUID, SQLWCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
 * SQLRETURN SQLCopyDesc( SQLHDESC  sDesc, SQLHDESC  tDesc );
 * SQLRETURN SQLDataSources( SQLHENV  henv, SQLSMALLINT  fDirection, SQLCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
 * SQLRETURN SQLDataSourcesW( SQLHENV  henv, SQLSMALLINT  fDirection, SQLWCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLWCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
 * SQLRETURN SQLDescribeCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
 * SQLRETURN SQLDescribeColW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLWCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
 * SQLRETURN SQLDescribeParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
 * SQLRETURN SQLDisconnect( SQLHDBC  hdbc );
 * SQLRETURN SQLDriverConnect( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
 * SQLRETURN SQLDriverConnectW( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLWCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
 * SQLRETURN SQLEndTran( SQLSMALLINT  htype, SQLHENV  henv, SQLSMALLINT  ctype );
 * SQLRETURN SQLError( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
 * SQLRETURN SQLErrorW( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLWCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
 * SQLRETURN SQLExecDirect( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
 * SQLRETURN SQLExecDirectW( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
 * SQLRETURN SQLExecute( SQLHSTMT  hstmt );
 * SQLRETURN SQLExtendedFetch( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset, SQLINTEGER * pcrow, SQLSMALLINT * rgfRowStatus );
 * SQLRETURN SQLFetch( SQLHSTMT  hstmt );
 * SQLRETURN SQLFetchScroll( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset );
 * SQLRETURN SQLForeignKeys( SQLHSTMT  hstmt, SQLCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
 * SQLRETURN SQLForeignKeysW( SQLHSTMT  hstmt, SQLWCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLWCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLWCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLWCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLWCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLWCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
 * SQLRETURN SQLFreeConnect( SQLHDBC  hdbc );
 * SQLRETURN SQLFreeEnv( SQLHENV  henv );
 * SQLRETURN SQLFreeStmt( SQLHSTMT  hstmt, SQLSMALLINT  fOption );
 * SQLRETURN SQLFreeHandle( SQLSMALLINT  htype, SQLINTEGER  hndl );
 * SQLRETURN SQLGetCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
 * SQLRETURN SQLGetColW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
 * SQLRETURN SQLGetConnectAttr( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
 * SQLRETURN SQLGetConnectAttrW( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
 * SQLRETURN SQLGetConnectOption( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
 * SQLRETURN SQLGetConnectOptionW( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
 * SQLRETURN SQLGetCursorName( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
 * SQLRETURN SQLGetCursorNameW( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
 * SQLRETURN SQLGetData( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fCType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
 * SQLRETURN SQLGetDescField( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
 * SQLRETURN SQLGetDescFieldW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
 * SQLRETURN SQLGetDescRec( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
 * SQLRETURN SQLGetDescRecW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLWCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
 * SQLRETURN SQLGetDiagField( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
 * SQLRETURN SQLGetDiagFieldW( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
 * SQLRETURN SQLGetDiagRec( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLCHAR * SQLstate, SQLINTEGER * SQLcode, SQLCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
 * SQLRETURN SQLGetDiagRecW( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLWCHAR * SQLstate, SQLINTEGER * SQLcode, SQLWCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
 * SQLRETURN SQLGetEnvAttr( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParamMax, SQLINTEGER * pcbParam );
 * SQLRETURN SQLGetFunctions( SQLHDBC  hdbc, SQLSMALLINT  fFunction, SQLSMALLINT * pfExists );
 * SQLRETURN SQLGetInfo( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
 * SQLRETURN SQLGetInfoW( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
 * SQLRETURN SQLGetLength( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  locator, SQLINTEGER * sLength, SQLINTEGER * ind );
 * SQLRETURN SQLGetPosition( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
 * SQLRETURN SQLGetPositionW( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLWCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
 * SQLRETURN SQLGetStmtAttr( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
 * SQLRETURN SQLGetStmtAttrW( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
 * SQLRETURN SQLGetStmtOption( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
 * SQLRETURN SQLGetStmtOptionW( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
 * SQLRETURN SQLGetSubString( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
 * SQLRETURN SQLGetSubStringW( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
 * SQLRETURN SQLGetTypeInfo( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
 * SQLRETURN SQLGetTypeInfoW( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
 * SQLRETURN SQLLanguages( SQLHSTMT  hstmt );
 * SQLRETURN SQLMoreResults( SQLHSTMT  hstmt );
 * SQLRETURN SQLNativeSql( SQLHDBC  hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
 * SQLRETURN SQLNativeSqlW( SQLHDBC  hdbc, SQLWCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
 * SQLRETURN SQLNextResult( SQLHSTMT  hstmt, SQLHSTMT  hstmt2 );
 * SQLRETURN SQLNumParams( SQLHSTMT  hstmt, SQLSMALLINT * pcpar );
 * SQLRETURN SQLNumResultCols( SQLHSTMT  hstmt, SQLSMALLINT * pccol );
 * SQLRETURN SQLParamData( SQLHSTMT  hstmt, SQLPOINTER * Value );
 * SQLRETURN SQLParamOptions( SQLHSTMT  hstmt, SQLINTEGER  crow, SQLINTEGER * pirow );
 * SQLRETURN SQLPrepare( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
 * SQLRETURN SQLPrepareW( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
 * SQLRETURN SQLPrimaryKeys( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
 * SQLRETURN SQLPrimaryKeysW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
 * SQLRETURN SQLProcedureColumns( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
 * SQLRETURN SQLProcedureColumnsW( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
 * SQLRETURN SQLProcedures( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName );
 * SQLRETURN SQLProceduresW( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName );
 * SQLRETURN SQLPutData( SQLHSTMT  hstmt, SQLPOINTER  Data, SQLINTEGER  SLen );
 * SQLRETURN SQLReleaseEnv( SQLHENV  henv );
 * SQLRETURN SQLRowCount( SQLHSTMT  hstmt, SQLINTEGER * pcrow );
 * SQLRETURN SQLSetConnectAttr( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
 * SQLRETURN SQLSetConnectAttrW( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
 * SQLRETURN SQLSetConnectOption( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
 * SQLRETURN SQLSetConnectOptionW( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
 * SQLRETURN SQLSetCursorName( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursor );
 * SQLRETURN SQLSetCursorNameW( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursor );
 * SQLRETURN SQLSetDescField( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
 * SQLRETURN SQLSetDescFieldW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
 * SQLRETURN SQLSetDescRec( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  Type, SQLSMALLINT  subType, SQLINTEGER  fLength, SQLSMALLINT  fPrec, SQLSMALLINT  fScale, SQLPOINTER  Value, SQLINTEGER * sLength, SQLINTEGER * indic );
 * SQLRETURN SQLSetEnvAttr( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParam );
 * SQLRETURN SQLSetParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fCType, SQLSMALLINT  fSqlType, SQLINTEGER  cbColDef, SQLSMALLINT  ibScale, SQLPOINTER  rgbValue, SQLINTEGER * pcbValue );
 * SQLRETURN SQLSetStmtAttr( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
 * SQLRETURN SQLSetStmtAttrW( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
 * SQLRETURN SQLSetStmtOption( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
 * SQLRETURN SQLSetStmtOptionW( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
 * SQLRETURN SQLSpecialColumns( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
 * SQLRETURN SQLSpecialColumnsW( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLWCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
 * SQLRETURN SQLStartTran( SQLSMALLINT  htype, SQLHENV  henv, SQLINTEGER  mode, SQLINTEGER  clevel );
 * SQLRETURN SQLStatistics( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
 * SQLRETURN SQLStatisticsW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
 * SQLRETURN SQLTablePrivileges( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
 * SQLRETURN SQLTablePrivilegesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
 * SQLRETURN SQLTables( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szTableType, SQLSMALLINT  cbTableType );
 * SQLRETURN SQLTablesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szTableType, SQLSMALLINT  cbTableType );
 * SQLRETURN SQLTransact( SQLHENV  henv, SQLHDBC  hdbc, SQLSMALLINT  fType );
 * SQLRETURN SQLOverrideCCSID400( SQLINTEGER  newCCSID );
 * === sqlcli.h === */

/* ===================================================
 * ASYNC CLI interfaces
 * ===================================================
 */

/* choose either callback or join    */
/* following structures returned     */
/* caller must free return structure */

typedef struct SQLBindColStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  iType; SQLPOINTER  rgbValue; SQLINTEGER  cbValueMax; SQLINTEGER * pcbValue; void * callback; } SQLBindColStruct;
typedef struct SQLBindFileToColStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLCHAR * fName; SQLSMALLINT * fNameLen; SQLINTEGER * fOptions; SQLSMALLINT  fValueMax; SQLINTEGER * sLen; SQLINTEGER * pcbValue; void * callback; } SQLBindFileToColStruct;
typedef struct SQLBindFileToParamStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  ipar; SQLSMALLINT  iType; SQLCHAR * fName; SQLSMALLINT * fNameLen; SQLINTEGER * fOptions; SQLSMALLINT  fValueMax; SQLINTEGER * pcbValue; void * callback; } SQLBindFileToParamStruct;
typedef struct SQLBindParamStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  iparm; SQLSMALLINT  iType; SQLSMALLINT  pType; SQLINTEGER  pLen; SQLSMALLINT  pScale; SQLPOINTER  pData; SQLINTEGER * pcbValue; void * callback; } SQLBindParamStruct;
typedef struct SQLBindParameterStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  ipar; SQLSMALLINT  fParamType; SQLSMALLINT  fCType; SQLSMALLINT  fSQLType; SQLINTEGER  pLen; SQLSMALLINT  pScale; SQLPOINTER  pData; SQLINTEGER  cbValueMax; SQLINTEGER * pcbValue; void * callback; } SQLBindParameterStruct;
typedef struct SQLCancelStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; void * callback; } SQLCancelStruct;
typedef struct SQLCloseCursorStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; void * callback; } SQLCloseCursorStruct;
typedef struct SQLColAttributeStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  fDescType; SQLPOINTER  rgbDesc; SQLSMALLINT  cbDescMax; SQLSMALLINT * pcbDesc; SQLPOINTER  pfDesc; void * callback; } SQLColAttributeStruct;
typedef struct SQLColAttributeWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  fDescType; SQLPOINTER  rgbDesc; SQLSMALLINT  cbDescMax; SQLSMALLINT * pcbDesc; SQLPOINTER  pfDesc; void * callback; } SQLColAttributeWStruct;
typedef struct SQLColAttributesStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  fDescType; SQLCHAR * rgbDesc; SQLINTEGER  cbDescMax; SQLINTEGER * pcbDesc; SQLINTEGER * pfDesc; void * callback; } SQLColAttributesStruct;
typedef struct SQLColAttributesWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  fDescType; SQLWCHAR * rgbDesc; SQLINTEGER  cbDescMax; SQLINTEGER * pcbDesc; SQLINTEGER * pfDesc; void * callback; } SQLColAttributesWStruct;
typedef struct SQLColumnPrivilegesStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; SQLCHAR * szColumnName; SQLSMALLINT  cbColumnName; void * callback; } SQLColumnPrivilegesStruct;
typedef struct SQLColumnPrivilegesWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; SQLWCHAR * szColumnName; SQLSMALLINT  cbColumnName; void * callback; } SQLColumnPrivilegesWStruct;
typedef struct SQLColumnsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; SQLCHAR * szColumnName; SQLSMALLINT  cbColumnName; void * callback; } SQLColumnsStruct;
typedef struct SQLColumnsWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; SQLWCHAR * szColumnName; SQLSMALLINT  cbColumnName; void * callback; } SQLColumnsWStruct;
typedef struct SQLConnectStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLCHAR * szDSN; SQLSMALLINT  cbDSN; SQLCHAR * szUID; SQLSMALLINT  cbUID; SQLCHAR * szAuthStr; SQLSMALLINT  cbAuthStr; void * callback; } SQLConnectStruct;
typedef struct SQLConnectWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLWCHAR * szDSN; SQLSMALLINT  cbDSN; SQLWCHAR * szUID; SQLSMALLINT  cbUID; SQLWCHAR * szAuthStr; SQLSMALLINT  cbAuthStr; void * callback; } SQLConnectWStruct;
typedef struct SQLCopyDescStruct { SQLRETURN sqlrc; SQLHDESC  sDesc; SQLHDESC  tDesc; void * callback; } SQLCopyDescStruct;
typedef struct SQLDataSourcesStruct { SQLRETURN sqlrc; SQLHENV  henv; SQLSMALLINT  fDirection; SQLCHAR * szDSN; SQLSMALLINT  cbDSNMax; SQLSMALLINT * pcbDSN; SQLCHAR * szDescription; SQLSMALLINT  cbDescriptionMax; SQLSMALLINT * pcbDescription; void * callback; } SQLDataSourcesStruct;
typedef struct SQLDataSourcesWStruct { SQLRETURN sqlrc; SQLHENV  henv; SQLSMALLINT  fDirection; SQLWCHAR * szDSN; SQLSMALLINT  cbDSNMax; SQLSMALLINT * pcbDSN; SQLWCHAR * szDescription; SQLSMALLINT  cbDescriptionMax; SQLSMALLINT * pcbDescription; void * callback; } SQLDataSourcesWStruct;
typedef struct SQLDescribeColStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLCHAR * szColName; SQLSMALLINT  cbColNameMax; SQLSMALLINT * pcbColName; SQLSMALLINT * pfSqlType; SQLINTEGER * pcbColDef; SQLSMALLINT * pibScale; SQLSMALLINT * pfNullable; void * callback; } SQLDescribeColStruct;
typedef struct SQLDescribeColWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLWCHAR * szColName; SQLSMALLINT  cbColNameMax; SQLSMALLINT * pcbColName; SQLSMALLINT * pfSqlType; SQLINTEGER * pcbColDef; SQLSMALLINT * pibScale; SQLSMALLINT * pfNullable; void * callback; } SQLDescribeColWStruct;
typedef struct SQLDescribeParamStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  ipar; SQLSMALLINT * pfSqlType; SQLINTEGER * pcbColDef; SQLSMALLINT * pibScale; SQLSMALLINT * pfNullable; void * callback; } SQLDescribeParamStruct;
typedef struct SQLDisconnectStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; void * callback; } SQLDisconnectStruct;
typedef struct SQLDriverConnectStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  hwnd; SQLCHAR * szConnStrIn; SQLSMALLINT  cbConnStrin; SQLCHAR * szConnStrOut; SQLSMALLINT  cbConnStrOutMax; SQLSMALLINT * pcbConnStrOut; SQLSMALLINT  fDriverCompletion; void * callback; } SQLDriverConnectStruct;
typedef struct SQLDriverConnectWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  hwnd; SQLWCHAR * szConnStrIn; SQLSMALLINT  cbConnStrin; SQLWCHAR * szConnStrOut; SQLSMALLINT  cbConnStrOutMax; SQLSMALLINT * pcbConnStrOut; SQLSMALLINT  fDriverCompletion; void * callback; } SQLDriverConnectWStruct;
typedef struct SQLEndTranStruct { SQLRETURN sqlrc; SQLSMALLINT  htype; SQLHENV  henv; SQLSMALLINT  ctype; void * callback; } SQLEndTranStruct;
typedef struct SQLErrorStruct { SQLRETURN sqlrc; SQLHENV  henv; SQLHDBC  hdbc; SQLHSTMT  hstmt; SQLCHAR * szSqlState; SQLINTEGER * pfNativeError; SQLCHAR * szErrorMsg; SQLSMALLINT  cbErrorMsgMax; SQLSMALLINT * pcbErrorMsg; void * callback; } SQLErrorStruct;
typedef struct SQLErrorWStruct { SQLRETURN sqlrc; SQLHENV  henv; SQLHDBC  hdbc; SQLHSTMT  hstmt; SQLWCHAR * szSqlState; SQLINTEGER * pfNativeError; SQLWCHAR * szErrorMsg; SQLSMALLINT  cbErrorMsgMax; SQLSMALLINT * pcbErrorMsg; void * callback; } SQLErrorWStruct;
typedef struct SQLExecDirectStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szSqlStr; SQLINTEGER  cbSqlStr; void * callback; } SQLExecDirectStruct;
typedef struct SQLExecDirectWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szSqlStr; SQLINTEGER  cbSqlStr; void * callback; } SQLExecDirectWStruct;
typedef struct SQLExecuteStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; void * callback; } SQLExecuteStruct;
typedef struct SQLExtendedFetchStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fOrient; SQLINTEGER  fOffset; SQLINTEGER * pcrow; SQLSMALLINT * rgfRowStatus; void * callback; } SQLExtendedFetchStruct;
typedef struct SQLFetchStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; void * callback; } SQLFetchStruct;
typedef struct SQLFetchScrollStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fOrient; SQLINTEGER  fOffset; void * callback; } SQLFetchScrollStruct;
typedef struct SQLForeignKeysStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szPkTableQualifier; SQLSMALLINT  cbPkTableQualifier; SQLCHAR * szPkTableOwner; SQLSMALLINT  cbPkTableOwner; SQLCHAR * szPkTableName; SQLSMALLINT  cbPkTableName; SQLCHAR * szFkTableQualifier; SQLSMALLINT  cbFkTableQualifier; SQLCHAR * szFkTableOwner; SQLSMALLINT  cbFkTableOwner; SQLCHAR * szFkTableName; SQLSMALLINT  cbFkTableName; void * callback; } SQLForeignKeysStruct;
typedef struct SQLForeignKeysWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szPkTableQualifier; SQLSMALLINT  cbPkTableQualifier; SQLWCHAR * szPkTableOwner; SQLSMALLINT  cbPkTableOwner; SQLWCHAR * szPkTableName; SQLSMALLINT  cbPkTableName; SQLWCHAR * szFkTableQualifier; SQLSMALLINT  cbFkTableQualifier; SQLWCHAR * szFkTableOwner; SQLSMALLINT  cbFkTableOwner; SQLWCHAR * szFkTableName; SQLSMALLINT  cbFkTableName; void * callback; } SQLForeignKeysWStruct;
typedef struct SQLGetColStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  itype; SQLPOINTER  tval; SQLINTEGER  blen; SQLINTEGER * olen; void * callback; } SQLGetColStruct;
typedef struct SQLGetColWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  itype; SQLPOINTER  tval; SQLINTEGER  blen; SQLINTEGER * olen; void * callback; } SQLGetColWStruct;
typedef struct SQLGetConnectAttrStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLINTEGER  attr; SQLPOINTER  oval; SQLINTEGER  ilen; SQLINTEGER * olen; void * callback; } SQLGetConnectAttrStruct;
typedef struct SQLGetConnectAttrWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLINTEGER  attr; SQLPOINTER  oval; SQLINTEGER  ilen; SQLINTEGER * olen; void * callback; } SQLGetConnectAttrWStruct;
typedef struct SQLGetConnectOptionStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  iopt; SQLPOINTER  oval; void * callback; } SQLGetConnectOptionStruct;
typedef struct SQLGetConnectOptionWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  iopt; SQLPOINTER  oval; void * callback; } SQLGetConnectOptionWStruct;
typedef struct SQLGetCursorNameStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szCursor; SQLSMALLINT  cbCursorMax; SQLSMALLINT * pcbCursor; void * callback; } SQLGetCursorNameStruct;
typedef struct SQLGetCursorNameWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szCursor; SQLSMALLINT  cbCursorMax; SQLSMALLINT * pcbCursor; void * callback; } SQLGetCursorNameWStruct;
typedef struct SQLGetDataStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  icol; SQLSMALLINT  fCType; SQLPOINTER  rgbValue; SQLINTEGER  cbValueMax; SQLINTEGER * pcbValue; void * callback; } SQLGetDataStruct;
typedef struct SQLGetDescFieldStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLSMALLINT  fieldID; SQLPOINTER  fValue; SQLINTEGER  fLength; SQLINTEGER * stLength; void * callback; } SQLGetDescFieldStruct;
typedef struct SQLGetDescFieldWStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLSMALLINT  fieldID; SQLPOINTER  fValue; SQLINTEGER  fLength; SQLINTEGER * stLength; void * callback; } SQLGetDescFieldWStruct;
typedef struct SQLGetDescRecStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLCHAR * fname; SQLSMALLINT  bufLen; SQLSMALLINT * sLength; SQLSMALLINT * sType; SQLSMALLINT * sbType; SQLINTEGER * fLength; SQLSMALLINT * fprec; SQLSMALLINT * fscale; SQLSMALLINT * fnull; void * callback; } SQLGetDescRecStruct;
typedef struct SQLGetDescRecWStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLWCHAR * fname; SQLSMALLINT  bufLen; SQLSMALLINT * sLength; SQLSMALLINT * sType; SQLSMALLINT * sbType; SQLINTEGER * fLength; SQLSMALLINT * fprec; SQLSMALLINT * fscale; SQLSMALLINT * fnull; void * callback; } SQLGetDescRecWStruct;
typedef struct SQLGetDiagFieldStruct { SQLRETURN sqlrc; SQLSMALLINT  hType; SQLINTEGER  hndl; SQLSMALLINT  rcdNum; SQLSMALLINT  diagID; SQLPOINTER  dValue; SQLSMALLINT  bLength; SQLSMALLINT * sLength; void * callback; } SQLGetDiagFieldStruct;
typedef struct SQLGetDiagFieldWStruct { SQLRETURN sqlrc; SQLSMALLINT  hType; SQLINTEGER  hndl; SQLSMALLINT  rcdNum; SQLSMALLINT  diagID; SQLPOINTER  dValue; SQLSMALLINT  bLength; SQLSMALLINT * sLength; void * callback; } SQLGetDiagFieldWStruct;
typedef struct SQLGetDiagRecStruct { SQLRETURN sqlrc; SQLSMALLINT  hType; SQLINTEGER  hndl; SQLSMALLINT  rcdNum; SQLCHAR * SQLstate; SQLINTEGER * SQLcode; SQLCHAR * msgText; SQLSMALLINT  bLength; SQLSMALLINT * SLength; void * callback; } SQLGetDiagRecStruct;
typedef struct SQLGetDiagRecWStruct { SQLRETURN sqlrc; SQLSMALLINT  hType; SQLINTEGER  hndl; SQLSMALLINT  rcdNum; SQLWCHAR * SQLstate; SQLINTEGER * SQLcode; SQLWCHAR * msgText; SQLSMALLINT  bLength; SQLSMALLINT * SLength; void * callback; } SQLGetDiagRecWStruct;
typedef struct SQLGetEnvAttrStruct { SQLRETURN sqlrc; SQLHENV  hEnv; SQLINTEGER  fAttribute; SQLPOINTER  pParam; SQLINTEGER  cbParamMax; SQLINTEGER * pcbParam; void * callback; } SQLGetEnvAttrStruct;
typedef struct SQLGetFunctionsStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  fFunction; SQLSMALLINT * pfExists; void * callback; } SQLGetFunctionsStruct;
typedef struct SQLGetInfoStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  fInfoType; SQLPOINTER  rgbInfoValue; SQLSMALLINT  cbInfoValueMax; SQLSMALLINT * pcbInfoValue; void * callback; } SQLGetInfoStruct;
typedef struct SQLGetInfoWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  fInfoType; SQLPOINTER  rgbInfoValue; SQLSMALLINT  cbInfoValueMax; SQLSMALLINT * pcbInfoValue; void * callback; } SQLGetInfoWStruct;
typedef struct SQLGetLengthStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  locType; SQLINTEGER  locator; SQLINTEGER * sLength; SQLINTEGER * ind; void * callback; } SQLGetLengthStruct;
typedef struct SQLGetPositionStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  locType; SQLINTEGER  srceLocator; SQLINTEGER  srchLocator; SQLCHAR * srchLiteral; SQLINTEGER  srchLiteralLen; SQLINTEGER  fPosition; SQLINTEGER * located; SQLINTEGER * ind; void * callback; } SQLGetPositionStruct;
typedef struct SQLGetPositionWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  locType; SQLINTEGER  srceLocator; SQLINTEGER  srchLocator; SQLWCHAR * srchLiteral; SQLINTEGER  srchLiteralLen; SQLINTEGER  fPosition; SQLINTEGER * located; SQLINTEGER * ind; void * callback; } SQLGetPositionWStruct;
typedef struct SQLGetStmtAttrStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER  fAttr; SQLPOINTER  pvParam; SQLINTEGER  bLength; SQLINTEGER * SLength; void * callback; } SQLGetStmtAttrStruct;
typedef struct SQLGetStmtAttrWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER  fAttr; SQLPOINTER  pvParam; SQLINTEGER  bLength; SQLINTEGER * SLength; void * callback; } SQLGetStmtAttrWStruct;
typedef struct SQLGetStmtOptionStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fOption; SQLPOINTER  pvParam; void * callback; } SQLGetStmtOptionStruct;
typedef struct SQLGetStmtOptionWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fOption; SQLPOINTER  pvParam; void * callback; } SQLGetStmtOptionWStruct;
typedef struct SQLGetSubStringStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  locType; SQLINTEGER  srceLocator; SQLINTEGER  fPosition; SQLINTEGER  length; SQLSMALLINT  tType; SQLPOINTER  rgbValue; SQLINTEGER  cbValueMax; SQLINTEGER * StringLength; SQLINTEGER * ind; void * callback; } SQLGetSubStringStruct;
typedef struct SQLGetSubStringWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  locType; SQLINTEGER  srceLocator; SQLINTEGER  fPosition; SQLINTEGER  length; SQLSMALLINT  tType; SQLPOINTER  rgbValue; SQLINTEGER  cbValueMax; SQLINTEGER * StringLength; SQLINTEGER * ind; void * callback; } SQLGetSubStringWStruct;
typedef struct SQLGetTypeInfoStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fSqlType; void * callback; } SQLGetTypeInfoStruct;
typedef struct SQLGetTypeInfoWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fSqlType; void * callback; } SQLGetTypeInfoWStruct;
typedef struct SQLLanguagesStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; void * callback; } SQLLanguagesStruct;
typedef struct SQLMoreResultsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; void * callback; } SQLMoreResultsStruct;
typedef struct SQLNativeSqlStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLCHAR * szSqlStrIn; SQLINTEGER  cbSqlStrIn; SQLCHAR * szSqlStr; SQLINTEGER  cbSqlStrMax; SQLINTEGER * pcbSqlStr; void * callback; } SQLNativeSqlStruct;
typedef struct SQLNativeSqlWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLWCHAR * szSqlStrIn; SQLINTEGER  cbSqlStrIn; SQLWCHAR * szSqlStr; SQLINTEGER  cbSqlStrMax; SQLINTEGER * pcbSqlStr; void * callback; } SQLNativeSqlWStruct;
typedef struct SQLNextResultStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLHSTMT  hstmt2; void * callback; } SQLNextResultStruct;
typedef struct SQLNumParamsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT * pcpar; void * callback; } SQLNumParamsStruct;
typedef struct SQLNumResultColsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT * pccol; void * callback; } SQLNumResultColsStruct;
typedef struct SQLParamDataStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLPOINTER * Value; void * callback; } SQLParamDataStruct;
typedef struct SQLParamOptionsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER  crow; SQLINTEGER * pirow; void * callback; } SQLParamOptionsStruct;
typedef struct SQLPrepareStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szSqlStr; SQLINTEGER  cbSqlStr; void * callback; } SQLPrepareStruct;
typedef struct SQLPrepareWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szSqlStr; SQLINTEGER  cbSqlStr; void * callback; } SQLPrepareWStruct;
typedef struct SQLPrimaryKeysStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; void * callback; } SQLPrimaryKeysStruct;
typedef struct SQLPrimaryKeysWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; void * callback; } SQLPrimaryKeysWStruct;
typedef struct SQLProcedureColumnsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szProcQualifier; SQLSMALLINT  cbProcQualifier; SQLCHAR * szProcOwner; SQLSMALLINT  cbProcOwner; SQLCHAR * szProcName; SQLSMALLINT  cbProcName; SQLCHAR * szColumnName; SQLSMALLINT  cbColumnName; void * callback; } SQLProcedureColumnsStruct;
typedef struct SQLProcedureColumnsWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szProcQualifier; SQLSMALLINT  cbProcQualifier; SQLWCHAR * szProcOwner; SQLSMALLINT  cbProcOwner; SQLWCHAR * szProcName; SQLSMALLINT  cbProcName; SQLWCHAR * szColumnName; SQLSMALLINT  cbColumnName; void * callback; } SQLProcedureColumnsWStruct;
typedef struct SQLProceduresStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szProcQualifier; SQLSMALLINT  cbProcQualifier; SQLCHAR * szProcOwner; SQLSMALLINT  cbProcOwner; SQLCHAR * szProcName; SQLSMALLINT  cbProcName; void * callback; } SQLProceduresStruct;
typedef struct SQLProceduresWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szProcQualifier; SQLSMALLINT  cbProcQualifier; SQLWCHAR * szProcOwner; SQLSMALLINT  cbProcOwner; SQLWCHAR * szProcName; SQLSMALLINT  cbProcName; void * callback; } SQLProceduresWStruct;
typedef struct SQLPutDataStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLPOINTER  Data; SQLINTEGER  SLen; void * callback; } SQLPutDataStruct;
typedef struct SQLReleaseEnvStruct { SQLRETURN sqlrc; SQLHENV  henv; void * callback; } SQLReleaseEnvStruct;
typedef struct SQLRowCountStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER * pcrow; void * callback; } SQLRowCountStruct;
typedef struct SQLSetConnectAttrStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLINTEGER  attrib; SQLPOINTER  vParam; SQLINTEGER  inlen; void * callback; } SQLSetConnectAttrStruct;
typedef struct SQLSetConnectAttrWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLINTEGER  attrib; SQLPOINTER  vParam; SQLINTEGER  inlen; void * callback; } SQLSetConnectAttrWStruct;
typedef struct SQLSetConnectOptionStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  fOption; SQLPOINTER  vParam; void * callback; } SQLSetConnectOptionStruct;
typedef struct SQLSetConnectOptionWStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLSMALLINT  fOption; SQLPOINTER  vParam; void * callback; } SQLSetConnectOptionWStruct;
typedef struct SQLSetCursorNameStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szCursor; SQLSMALLINT  cbCursor; void * callback; } SQLSetCursorNameStruct;
typedef struct SQLSetCursorNameWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szCursor; SQLSMALLINT  cbCursor; void * callback; } SQLSetCursorNameWStruct;
typedef struct SQLSetDescFieldStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLSMALLINT  fID; SQLPOINTER  Value; SQLINTEGER  buffLen; void * callback; } SQLSetDescFieldStruct;
typedef struct SQLSetDescFieldWStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLSMALLINT  fID; SQLPOINTER  Value; SQLINTEGER  buffLen; void * callback; } SQLSetDescFieldWStruct;
typedef struct SQLSetDescRecStruct { SQLRETURN sqlrc; SQLHDESC  hdesc; SQLSMALLINT  rcdNum; SQLSMALLINT  Type; SQLSMALLINT  subType; SQLINTEGER  fLength; SQLSMALLINT  fPrec; SQLSMALLINT  fScale; SQLPOINTER  Value; SQLINTEGER * sLength; SQLINTEGER * indic; void * callback; } SQLSetDescRecStruct;
typedef struct SQLSetEnvAttrStruct { SQLRETURN sqlrc; SQLHENV  hEnv; SQLINTEGER  fAttribute; SQLPOINTER  pParam; SQLINTEGER  cbParam; void * callback; } SQLSetEnvAttrStruct;
typedef struct SQLSetParamStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  ipar; SQLSMALLINT  fCType; SQLSMALLINT  fSqlType; SQLINTEGER  cbColDef; SQLSMALLINT  ibScale; SQLPOINTER  rgbValue; SQLINTEGER * pcbValue; void * callback; } SQLSetParamStruct;
typedef struct SQLSetStmtAttrStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER  fAttr; SQLPOINTER  pParam; SQLINTEGER  vParam; void * callback; } SQLSetStmtAttrStruct;
typedef struct SQLSetStmtAttrWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER  fAttr; SQLPOINTER  pParam; SQLINTEGER  vParam; void * callback; } SQLSetStmtAttrWStruct;
typedef struct SQLSetStmtOptionStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fOption; SQLPOINTER  vParam; void * callback; } SQLSetStmtOptionStruct;
typedef struct SQLSetStmtOptionWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fOption; SQLPOINTER  vParam; void * callback; } SQLSetStmtOptionWStruct;
typedef struct SQLSpecialColumnsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fColType; SQLCHAR * szTableQual; SQLSMALLINT  cbTableQual; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; SQLSMALLINT  fScope; SQLSMALLINT  fNullable; void * callback; } SQLSpecialColumnsStruct;
typedef struct SQLSpecialColumnsWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLSMALLINT  fColType; SQLWCHAR * szTableQual; SQLSMALLINT  cbTableQual; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; SQLSMALLINT  fScope; SQLSMALLINT  fNullable; void * callback; } SQLSpecialColumnsWStruct;
typedef struct SQLStartTranStruct { SQLRETURN sqlrc; SQLSMALLINT  htype; SQLHENV  henv; SQLINTEGER  mode; SQLINTEGER  clevel; void * callback; } SQLStartTranStruct;
typedef struct SQLStatisticsStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; SQLSMALLINT  fUnique; SQLSMALLINT  fres; void * callback; } SQLStatisticsStruct;
typedef struct SQLStatisticsWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; SQLSMALLINT  fUnique; SQLSMALLINT  fres; void * callback; } SQLStatisticsWStruct;
typedef struct SQLTablePrivilegesStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; void * callback; } SQLTablePrivilegesStruct;
typedef struct SQLTablePrivilegesWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; void * callback; } SQLTablePrivilegesWStruct;
typedef struct SQLTablesStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLCHAR * szTableName; SQLSMALLINT  cbTableName; SQLCHAR * szTableType; SQLSMALLINT  cbTableType; void * callback; } SQLTablesStruct;
typedef struct SQLTablesWStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLWCHAR * szTableQualifier; SQLSMALLINT  cbTableQualifier; SQLWCHAR * szTableOwner; SQLSMALLINT  cbTableOwner; SQLWCHAR * szTableName; SQLSMALLINT  cbTableName; SQLWCHAR * szTableType; SQLSMALLINT  cbTableType; void * callback; } SQLTablesWStruct;
typedef struct SQLTransactStruct { SQLRETURN sqlrc; SQLHENV  henv; SQLHDBC  hdbc; SQLSMALLINT  fType; void * callback; } SQLTransactStruct;
typedef struct SQL400VersionStruct { SQLRETURN sqlrc; SQLPOINTER  outversion; SQLINTEGER  outlen; void * callback; } SQL400VersionStruct;
typedef struct SQL400Stmt2HdbcStruct { SQLRETURN sqlrc; SQLHSTMT  hstmt; SQLINTEGER * ohnd; void * callback; } SQL400Stmt2HdbcStruct;
typedef struct SQL400ConnectStruct { SQLRETURN sqlrc; SQLCHAR * db; SQLCHAR * uid; SQLCHAR * pwd; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400ConnectStruct;
typedef struct SQL400ConnectWStruct { SQLRETURN sqlrc; SQLWCHAR * db; SQLWCHAR * uid; SQLWCHAR * pwd; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400ConnectWStruct;
typedef struct SQL400pConnectStruct { SQLRETURN sqlrc; SQLCHAR * db; SQLCHAR * uid; SQLCHAR * pwd; SQLCHAR * qual; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400pConnectStruct;
typedef struct SQL400pConnectWStruct { SQLRETURN sqlrc; SQLWCHAR * db; SQLWCHAR * uid; SQLWCHAR * pwd; SQLCHAR * qual; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400pConnectWStruct;
typedef struct SQL400ConnectUtf8Struct { SQLRETURN sqlrc; SQLINTEGER  accsid; SQLCHAR * db; SQLCHAR * uid; SQLCHAR * pwd; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400ConnectUtf8Struct;
typedef struct SQL400pConnectUtf8Struct { SQLRETURN sqlrc; SQLINTEGER  accsid; SQLCHAR * db; SQLCHAR * uid; SQLCHAR * pwd; SQLCHAR * qual; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400pConnectUtf8Struct;
typedef struct SQL400ConnectUtf16Struct { SQLRETURN sqlrc; SQLINTEGER  accsid; SQLCHAR * db; SQLCHAR * uid; SQLCHAR * pwd; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400ConnectUtf16Struct;
typedef struct SQL400pConnectUtf16Struct { SQLRETURN sqlrc; SQLINTEGER  accsid; SQLCHAR * db; SQLCHAR * uid; SQLCHAR * pwd; SQLCHAR * qual; SQLINTEGER * ohnd; SQLINTEGER  acommit; SQLCHAR * alibl; SQLCHAR * acurlib; void * callback; } SQL400pConnectUtf16Struct;
typedef struct SQL400CloseStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; void * callback; } SQL400CloseStruct;
typedef struct SQL400pCloseStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; void * callback; } SQL400pCloseStruct;
typedef struct SQL400CloseAllStmtsStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; void * callback; } SQL400CloseAllStmtsStruct;
typedef struct SQL400CmdStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLCHAR * cmd; void * callback; } SQL400CmdStruct;
typedef struct SQL400ChgLiblStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLCHAR * libl; void * callback; } SQL400ChgLiblStruct;
typedef struct SQL400ChgCurLibStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLCHAR * curlib; void * callback; } SQL400ChgCurLibStruct;
typedef struct SQL400ToUtf8Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; void * callback; } SQL400ToUtf8Struct;
typedef struct SQL400FromUtf8Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  outccsid; void * callback; } SQL400FromUtf8Struct;
typedef struct SQL400ToUtf16Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; void * callback; } SQL400ToUtf16Struct;
typedef struct SQL400FromUtf16Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  outccsid; void * callback; } SQL400FromUtf16Struct;
typedef struct SQL400IgnoreNullToUtf8Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; void * callback; } SQL400IgnoreNullToUtf8Struct;
typedef struct SQL400IgnoreNullFromUtf8Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  outccsid; void * callback; } SQL400IgnoreNullFromUtf8Struct;
typedef struct SQL400IgnoreNullToUtf16Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; void * callback; } SQL400IgnoreNullToUtf16Struct;
typedef struct SQL400IgnoreNullFromUtf16Struct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  outccsid; void * callback; } SQL400IgnoreNullFromUtf16Struct;
typedef struct SQL400AnyToAnyStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; SQLINTEGER  outccsid; void * callback; } SQL400AnyToAnyStruct;
typedef struct SQL400AnyFromAnyStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; SQLINTEGER  outccsid; void * callback; } SQL400AnyFromAnyStruct;
typedef struct SQL400IgnoreNullAnyToAnyStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; SQLINTEGER  outccsid; void * callback; } SQL400IgnoreNullAnyToAnyStruct;
typedef struct SQL400IgnoreNullAnyFromAnyStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLPOINTER  inparm; SQLINTEGER  inlen; SQLPOINTER  outparm; SQLINTEGER  outlen; SQLINTEGER  inccsid; SQLINTEGER  outccsid; void * callback; } SQL400IgnoreNullAnyFromAnyStruct;
typedef struct SQL400JsonStruct { SQLRETURN sqlrc; SQLHDBC  hdbc; SQLCHAR * injson; SQLINTEGER  inlen; SQLCHAR * outjson; SQLINTEGER  outlen; void * callback; } SQL400JsonStruct;


/* join async thread                    */
/* flag:                                */
/*   SQL400_FLAG_JOIN_WAIT              */
/*     - block until complete           */
/*   SQL400_FLAG_JOIN_NO_WAIT           */
/*     - no block, NULL still executing */
#define SQL400_FLAG_JOIN_WAIT 0
#define SQL400_FLAG_JOIN_NO_WAIT 1

/* void SQLBindColCallback(SQLBindColStruct* ); */
SQLBindColStruct * SQLBindColJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLBindFileToColCallback(SQLBindFileToColStruct* ); */
SQLBindFileToColStruct * SQLBindFileToColJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLBindFileToParamCallback(SQLBindFileToParamStruct* ); */
SQLBindFileToParamStruct * SQLBindFileToParamJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLBindParamCallback(SQLBindParamStruct* ); */
SQLBindParamStruct * SQLBindParamJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLBindParameterCallback(SQLBindParameterStruct* ); */
SQLBindParameterStruct * SQLBindParameterJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLCancelCallback(SQLCancelStruct* ); */
SQLCancelStruct * SQLCancelJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLCloseCursorCallback(SQLCloseCursorStruct* ); */
SQLCloseCursorStruct * SQLCloseCursorJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColAttributeCallback(SQLColAttributeStruct* ); */
SQLColAttributeStruct * SQLColAttributeJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColAttributeWCallback(SQLColAttributeWStruct* ); */
SQLColAttributeWStruct * SQLColAttributeWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColAttributesCallback(SQLColAttributesStruct* ); */
SQLColAttributesStruct * SQLColAttributesJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColAttributesWCallback(SQLColAttributesWStruct* ); */
SQLColAttributesWStruct * SQLColAttributesWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColumnPrivilegesCallback(SQLColumnPrivilegesStruct* ); */
SQLColumnPrivilegesStruct * SQLColumnPrivilegesJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColumnPrivilegesWCallback(SQLColumnPrivilegesWStruct* ); */
SQLColumnPrivilegesWStruct * SQLColumnPrivilegesWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColumnsCallback(SQLColumnsStruct* ); */
SQLColumnsStruct * SQLColumnsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLColumnsWCallback(SQLColumnsWStruct* ); */
SQLColumnsWStruct * SQLColumnsWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLConnectCallback(SQLConnectStruct* ); */
SQLConnectStruct * SQLConnectJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLConnectWCallback(SQLConnectWStruct* ); */
SQLConnectWStruct * SQLConnectWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLCopyDescCallback(SQLCopyDescStruct* ); */
SQLCopyDescStruct * SQLCopyDescJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDataSourcesCallback(SQLDataSourcesStruct* ); */
SQLDataSourcesStruct * SQLDataSourcesJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDataSourcesWCallback(SQLDataSourcesWStruct* ); */
SQLDataSourcesWStruct * SQLDataSourcesWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDescribeColCallback(SQLDescribeColStruct* ); */
SQLDescribeColStruct * SQLDescribeColJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDescribeColWCallback(SQLDescribeColWStruct* ); */
SQLDescribeColWStruct * SQLDescribeColWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDescribeParamCallback(SQLDescribeParamStruct* ); */
SQLDescribeParamStruct * SQLDescribeParamJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDisconnectCallback(SQLDisconnectStruct* ); */
SQLDisconnectStruct * SQLDisconnectJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDriverConnectCallback(SQLDriverConnectStruct* ); */
SQLDriverConnectStruct * SQLDriverConnectJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLDriverConnectWCallback(SQLDriverConnectWStruct* ); */
SQLDriverConnectWStruct * SQLDriverConnectWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLEndTranCallback(SQLEndTranStruct* ); */
SQLEndTranStruct * SQLEndTranJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLErrorCallback(SQLErrorStruct* ); */
SQLErrorStruct * SQLErrorJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLErrorWCallback(SQLErrorWStruct* ); */
SQLErrorWStruct * SQLErrorWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLExecDirectCallback(SQLExecDirectStruct* ); */
SQLExecDirectStruct * SQLExecDirectJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLExecDirectWCallback(SQLExecDirectWStruct* ); */
SQLExecDirectWStruct * SQLExecDirectWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLExecuteCallback(SQLExecuteStruct* ); */
SQLExecuteStruct * SQLExecuteJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLExtendedFetchCallback(SQLExtendedFetchStruct* ); */
SQLExtendedFetchStruct * SQLExtendedFetchJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLFetchCallback(SQLFetchStruct* ); */
SQLFetchStruct * SQLFetchJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLFetchScrollCallback(SQLFetchScrollStruct* ); */
SQLFetchScrollStruct * SQLFetchScrollJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLForeignKeysCallback(SQLForeignKeysStruct* ); */
SQLForeignKeysStruct * SQLForeignKeysJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLForeignKeysWCallback(SQLForeignKeysWStruct* ); */
SQLForeignKeysWStruct * SQLForeignKeysWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetColCallback(SQLGetColStruct* ); */
SQLGetColStruct * SQLGetColJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetColWCallback(SQLGetColWStruct* ); */
SQLGetColWStruct * SQLGetColWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetConnectAttrCallback(SQLGetConnectAttrStruct* ); */
SQLGetConnectAttrStruct * SQLGetConnectAttrJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetConnectAttrWCallback(SQLGetConnectAttrWStruct* ); */
SQLGetConnectAttrWStruct * SQLGetConnectAttrWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetConnectOptionCallback(SQLGetConnectOptionStruct* ); */
SQLGetConnectOptionStruct * SQLGetConnectOptionJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetConnectOptionWCallback(SQLGetConnectOptionWStruct* ); */
SQLGetConnectOptionWStruct * SQLGetConnectOptionWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetCursorNameCallback(SQLGetCursorNameStruct* ); */
SQLGetCursorNameStruct * SQLGetCursorNameJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetCursorNameWCallback(SQLGetCursorNameWStruct* ); */
SQLGetCursorNameWStruct * SQLGetCursorNameWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDataCallback(SQLGetDataStruct* ); */
SQLGetDataStruct * SQLGetDataJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDescFieldCallback(SQLGetDescFieldStruct* ); */
SQLGetDescFieldStruct * SQLGetDescFieldJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDescFieldWCallback(SQLGetDescFieldWStruct* ); */
SQLGetDescFieldWStruct * SQLGetDescFieldWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDescRecCallback(SQLGetDescRecStruct* ); */
SQLGetDescRecStruct * SQLGetDescRecJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDescRecWCallback(SQLGetDescRecWStruct* ); */
SQLGetDescRecWStruct * SQLGetDescRecWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDiagFieldCallback(SQLGetDiagFieldStruct* ); */
SQLGetDiagFieldStruct * SQLGetDiagFieldJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDiagFieldWCallback(SQLGetDiagFieldWStruct* ); */
SQLGetDiagFieldWStruct * SQLGetDiagFieldWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDiagRecCallback(SQLGetDiagRecStruct* ); */
SQLGetDiagRecStruct * SQLGetDiagRecJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetDiagRecWCallback(SQLGetDiagRecWStruct* ); */
SQLGetDiagRecWStruct * SQLGetDiagRecWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetEnvAttrCallback(SQLGetEnvAttrStruct* ); */
SQLGetEnvAttrStruct * SQLGetEnvAttrJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetFunctionsCallback(SQLGetFunctionsStruct* ); */
SQLGetFunctionsStruct * SQLGetFunctionsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetInfoCallback(SQLGetInfoStruct* ); */
SQLGetInfoStruct * SQLGetInfoJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetInfoWCallback(SQLGetInfoWStruct* ); */
SQLGetInfoWStruct * SQLGetInfoWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetLengthCallback(SQLGetLengthStruct* ); */
SQLGetLengthStruct * SQLGetLengthJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetPositionCallback(SQLGetPositionStruct* ); */
SQLGetPositionStruct * SQLGetPositionJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetPositionWCallback(SQLGetPositionWStruct* ); */
SQLGetPositionWStruct * SQLGetPositionWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetStmtAttrCallback(SQLGetStmtAttrStruct* ); */
SQLGetStmtAttrStruct * SQLGetStmtAttrJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetStmtAttrWCallback(SQLGetStmtAttrWStruct* ); */
SQLGetStmtAttrWStruct * SQLGetStmtAttrWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetStmtOptionCallback(SQLGetStmtOptionStruct* ); */
SQLGetStmtOptionStruct * SQLGetStmtOptionJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetStmtOptionWCallback(SQLGetStmtOptionWStruct* ); */
SQLGetStmtOptionWStruct * SQLGetStmtOptionWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetSubStringCallback(SQLGetSubStringStruct* ); */
SQLGetSubStringStruct * SQLGetSubStringJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetSubStringWCallback(SQLGetSubStringWStruct* ); */
SQLGetSubStringWStruct * SQLGetSubStringWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetTypeInfoCallback(SQLGetTypeInfoStruct* ); */
SQLGetTypeInfoStruct * SQLGetTypeInfoJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLGetTypeInfoWCallback(SQLGetTypeInfoWStruct* ); */
SQLGetTypeInfoWStruct * SQLGetTypeInfoWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLLanguagesCallback(SQLLanguagesStruct* ); */
SQLLanguagesStruct * SQLLanguagesJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLMoreResultsCallback(SQLMoreResultsStruct* ); */
SQLMoreResultsStruct * SQLMoreResultsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLNativeSqlCallback(SQLNativeSqlStruct* ); */
SQLNativeSqlStruct * SQLNativeSqlJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLNativeSqlWCallback(SQLNativeSqlWStruct* ); */
SQLNativeSqlWStruct * SQLNativeSqlWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLNextResultCallback(SQLNextResultStruct* ); */
SQLNextResultStruct * SQLNextResultJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLNumParamsCallback(SQLNumParamsStruct* ); */
SQLNumParamsStruct * SQLNumParamsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLNumResultColsCallback(SQLNumResultColsStruct* ); */
SQLNumResultColsStruct * SQLNumResultColsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLParamDataCallback(SQLParamDataStruct* ); */
SQLParamDataStruct * SQLParamDataJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLParamOptionsCallback(SQLParamOptionsStruct* ); */
SQLParamOptionsStruct * SQLParamOptionsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLPrepareCallback(SQLPrepareStruct* ); */
SQLPrepareStruct * SQLPrepareJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLPrepareWCallback(SQLPrepareWStruct* ); */
SQLPrepareWStruct * SQLPrepareWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLPrimaryKeysCallback(SQLPrimaryKeysStruct* ); */
SQLPrimaryKeysStruct * SQLPrimaryKeysJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLPrimaryKeysWCallback(SQLPrimaryKeysWStruct* ); */
SQLPrimaryKeysWStruct * SQLPrimaryKeysWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLProcedureColumnsCallback(SQLProcedureColumnsStruct* ); */
SQLProcedureColumnsStruct * SQLProcedureColumnsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLProcedureColumnsWCallback(SQLProcedureColumnsWStruct* ); */
SQLProcedureColumnsWStruct * SQLProcedureColumnsWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLProceduresCallback(SQLProceduresStruct* ); */
SQLProceduresStruct * SQLProceduresJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLProceduresWCallback(SQLProceduresWStruct* ); */
SQLProceduresWStruct * SQLProceduresWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLPutDataCallback(SQLPutDataStruct* ); */
SQLPutDataStruct * SQLPutDataJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLReleaseEnvCallback(SQLReleaseEnvStruct* ); */
SQLReleaseEnvStruct * SQLReleaseEnvJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLRowCountCallback(SQLRowCountStruct* ); */
SQLRowCountStruct * SQLRowCountJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetConnectAttrCallback(SQLSetConnectAttrStruct* ); */
SQLSetConnectAttrStruct * SQLSetConnectAttrJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetConnectAttrWCallback(SQLSetConnectAttrWStruct* ); */
SQLSetConnectAttrWStruct * SQLSetConnectAttrWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetConnectOptionCallback(SQLSetConnectOptionStruct* ); */
SQLSetConnectOptionStruct * SQLSetConnectOptionJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetConnectOptionWCallback(SQLSetConnectOptionWStruct* ); */
SQLSetConnectOptionWStruct * SQLSetConnectOptionWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetCursorNameCallback(SQLSetCursorNameStruct* ); */
SQLSetCursorNameStruct * SQLSetCursorNameJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetCursorNameWCallback(SQLSetCursorNameWStruct* ); */
SQLSetCursorNameWStruct * SQLSetCursorNameWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetDescFieldCallback(SQLSetDescFieldStruct* ); */
SQLSetDescFieldStruct * SQLSetDescFieldJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetDescFieldWCallback(SQLSetDescFieldWStruct* ); */
SQLSetDescFieldWStruct * SQLSetDescFieldWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetDescRecCallback(SQLSetDescRecStruct* ); */
SQLSetDescRecStruct * SQLSetDescRecJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetEnvAttrCallback(SQLSetEnvAttrStruct* ); */
SQLSetEnvAttrStruct * SQLSetEnvAttrJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetParamCallback(SQLSetParamStruct* ); */
SQLSetParamStruct * SQLSetParamJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetStmtAttrCallback(SQLSetStmtAttrStruct* ); */
SQLSetStmtAttrStruct * SQLSetStmtAttrJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetStmtAttrWCallback(SQLSetStmtAttrWStruct* ); */
SQLSetStmtAttrWStruct * SQLSetStmtAttrWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetStmtOptionCallback(SQLSetStmtOptionStruct* ); */
SQLSetStmtOptionStruct * SQLSetStmtOptionJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSetStmtOptionWCallback(SQLSetStmtOptionWStruct* ); */
SQLSetStmtOptionWStruct * SQLSetStmtOptionWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSpecialColumnsCallback(SQLSpecialColumnsStruct* ); */
SQLSpecialColumnsStruct * SQLSpecialColumnsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLSpecialColumnsWCallback(SQLSpecialColumnsWStruct* ); */
SQLSpecialColumnsWStruct * SQLSpecialColumnsWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLStartTranCallback(SQLStartTranStruct* ); */
SQLStartTranStruct * SQLStartTranJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLStatisticsCallback(SQLStatisticsStruct* ); */
SQLStatisticsStruct * SQLStatisticsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLStatisticsWCallback(SQLStatisticsWStruct* ); */
SQLStatisticsWStruct * SQLStatisticsWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLTablePrivilegesCallback(SQLTablePrivilegesStruct* ); */
SQLTablePrivilegesStruct * SQLTablePrivilegesJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLTablePrivilegesWCallback(SQLTablePrivilegesWStruct* ); */
SQLTablePrivilegesWStruct * SQLTablePrivilegesWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLTablesCallback(SQLTablesStruct* ); */
SQLTablesStruct * SQLTablesJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLTablesWCallback(SQLTablesWStruct* ); */
SQLTablesWStruct * SQLTablesWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQLTransactCallback(SQLTransactStruct* ); */
SQLTransactStruct * SQLTransactJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400VersionCallback(SQL400VersionStruct* ); */
SQL400VersionStruct * SQL400VersionJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400Stmt2HdbcCallback(SQL400Stmt2HdbcStruct* ); */
SQL400Stmt2HdbcStruct * SQL400Stmt2HdbcJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400ConnectCallback(SQL400ConnectStruct* ); */
SQL400ConnectStruct * SQL400ConnectJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400ConnectWCallback(SQL400ConnectWStruct* ); */
SQL400ConnectWStruct * SQL400ConnectWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400pConnectCallback(SQL400pConnectStruct* ); */
SQL400pConnectStruct * SQL400pConnectJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400pConnectWCallback(SQL400pConnectWStruct* ); */
SQL400pConnectWStruct * SQL400pConnectWJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400ConnectUtf8Callback(SQL400ConnectUtf8Struct* ); */
SQL400ConnectUtf8Struct * SQL400ConnectUtf8Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400pConnectUtf8Callback(SQL400pConnectUtf8Struct* ); */
SQL400pConnectUtf8Struct * SQL400pConnectUtf8Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400ConnectUtf16Callback(SQL400ConnectUtf16Struct* ); */
SQL400ConnectUtf16Struct * SQL400ConnectUtf16Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400pConnectUtf16Callback(SQL400pConnectUtf16Struct* ); */
SQL400pConnectUtf16Struct * SQL400pConnectUtf16Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400CloseCallback(SQL400CloseStruct* ); */
SQL400CloseStruct * SQL400CloseJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400pCloseCallback(SQL400pCloseStruct* ); */
SQL400pCloseStruct * SQL400pCloseJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400CloseAllStmtsCallback(SQL400CloseAllStmtsStruct* ); */
SQL400CloseAllStmtsStruct * SQL400CloseAllStmtsJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400CmdCallback(SQL400CmdStruct* ); */
SQL400CmdStruct * SQL400CmdJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400ChgLiblCallback(SQL400ChgLiblStruct* ); */
SQL400ChgLiblStruct * SQL400ChgLiblJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400ChgCurLibCallback(SQL400ChgCurLibStruct* ); */
SQL400ChgCurLibStruct * SQL400ChgCurLibJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400ToUtf8Callback(SQL400ToUtf8Struct* ); */
SQL400ToUtf8Struct * SQL400ToUtf8Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400FromUtf8Callback(SQL400FromUtf8Struct* ); */
SQL400FromUtf8Struct * SQL400FromUtf8Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400ToUtf16Callback(SQL400ToUtf16Struct* ); */
SQL400ToUtf16Struct * SQL400ToUtf16Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400FromUtf16Callback(SQL400FromUtf16Struct* ); */
SQL400FromUtf16Struct * SQL400FromUtf16Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400IgnoreNullToUtf8Callback(SQL400IgnoreNullToUtf8Struct* ); */
SQL400IgnoreNullToUtf8Struct * SQL400IgnoreNullToUtf8Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400IgnoreNullFromUtf8Callback(SQL400IgnoreNullFromUtf8Struct* ); */
SQL400IgnoreNullFromUtf8Struct * SQL400IgnoreNullFromUtf8Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400IgnoreNullToUtf16Callback(SQL400IgnoreNullToUtf16Struct* ); */
SQL400IgnoreNullToUtf16Struct * SQL400IgnoreNullToUtf16Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400IgnoreNullFromUtf16Callback(SQL400IgnoreNullFromUtf16Struct* ); */
SQL400IgnoreNullFromUtf16Struct * SQL400IgnoreNullFromUtf16Join (pthread_t tid, SQLINTEGER flag);
/* void SQL400AnyToAnyCallback(SQL400AnyToAnyStruct* ); */
SQL400AnyToAnyStruct * SQL400AnyToAnyJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400AnyFromAnyCallback(SQL400AnyFromAnyStruct* ); */
SQL400AnyFromAnyStruct * SQL400AnyFromAnyJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400IgnoreNullAnyToAnyCallback(SQL400IgnoreNullAnyToAnyStruct* ); */
SQL400IgnoreNullAnyToAnyStruct * SQL400IgnoreNullAnyToAnyJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400IgnoreNullAnyFromAnyCallback(SQL400IgnoreNullAnyFromAnyStruct* ); */
SQL400IgnoreNullAnyFromAnyStruct * SQL400IgnoreNullAnyFromAnyJoin (pthread_t tid, SQLINTEGER flag);
/* void SQL400JsonCallback(SQL400JsonStruct* ); */
SQL400JsonStruct * SQL400JsonJoin (pthread_t tid, SQLINTEGER flag);


/* start an async call to DB2 CLI */
/* choose either callback or join */
/* for results returned.          */
/* sqlrc returned in structure.   */

pthread_t SQLBindColAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  iType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLBindFileToColAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * sLen, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLBindFileToParamAsync ( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  iType, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLBindParamAsync ( SQLHSTMT  hstmt, SQLSMALLINT  iparm, SQLSMALLINT  iType, SQLSMALLINT  pType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLBindParameterAsync ( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fParamType, SQLSMALLINT  fCType, SQLSMALLINT  fSQLType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLCancelAsync ( SQLHSTMT  hstmt, void * callback );
pthread_t SQLCloseCursorAsync ( SQLHSTMT  hstmt, void * callback );
pthread_t SQLColAttributeAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc, void * callback );
pthread_t SQLColAttributeWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc, void * callback );
pthread_t SQLColAttributesAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc, void * callback );
pthread_t SQLColAttributesWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLWCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc, void * callback );
pthread_t SQLColumnPrivilegesAsync ( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName, void * callback );
pthread_t SQLColumnPrivilegesWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName, void * callback );
pthread_t SQLColumnsAsync ( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName, void * callback );
pthread_t SQLColumnsWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName, void * callback );
pthread_t SQLConnectAsync ( SQLHDBC  hdbc, SQLCHAR * szDSN, SQLSMALLINT  cbDSN, SQLCHAR * szUID, SQLSMALLINT  cbUID, SQLCHAR * szAuthStr, SQLSMALLINT  cbAuthStr, void * callback );
pthread_t SQLConnectWAsync ( SQLHDBC  hdbc, SQLWCHAR * szDSN, SQLSMALLINT  cbDSN, SQLWCHAR * szUID, SQLSMALLINT  cbUID, SQLWCHAR * szAuthStr, SQLSMALLINT  cbAuthStr, void * callback );
pthread_t SQLCopyDescAsync ( SQLHDESC  sDesc, SQLHDESC  tDesc, void * callback );
pthread_t SQLDataSourcesAsync ( SQLHENV  henv, SQLSMALLINT  fDirection, SQLCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription, void * callback );
pthread_t SQLDataSourcesWAsync ( SQLHENV  henv, SQLSMALLINT  fDirection, SQLWCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLWCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription, void * callback );
pthread_t SQLDescribeColAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable, void * callback );
pthread_t SQLDescribeColWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLWCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable, void * callback );
pthread_t SQLDescribeParamAsync ( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable, void * callback );
pthread_t SQLDisconnectAsync ( SQLHDBC  hdbc, void * callback );
pthread_t SQLDriverConnectAsync ( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion, void * callback );
pthread_t SQLDriverConnectWAsync ( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLWCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion, void * callback );
pthread_t SQLEndTranAsync ( SQLSMALLINT  htype, SQLHENV  henv, SQLSMALLINT  ctype, void * callback );
pthread_t SQLErrorAsync ( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg, void * callback );
pthread_t SQLErrorWAsync ( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLWCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg, void * callback );
pthread_t SQLExecDirectAsync ( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr, void * callback );
pthread_t SQLExecDirectWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr, void * callback );
pthread_t SQLExecuteAsync ( SQLHSTMT  hstmt, void * callback );
pthread_t SQLExtendedFetchAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset, SQLINTEGER * pcrow, SQLSMALLINT * rgfRowStatus, void * callback );
pthread_t SQLFetchAsync ( SQLHSTMT  hstmt, void * callback );
pthread_t SQLFetchScrollAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset, void * callback );
pthread_t SQLForeignKeysAsync ( SQLHSTMT  hstmt, SQLCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLCHAR * szFkTableName, SQLSMALLINT  cbFkTableName, void * callback );
pthread_t SQLForeignKeysWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLWCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLWCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLWCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLWCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLWCHAR * szFkTableName, SQLSMALLINT  cbFkTableName, void * callback );
pthread_t SQLGetColAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen, void * callback );
pthread_t SQLGetColWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen, void * callback );
pthread_t SQLGetConnectAttrAsync ( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen, void * callback );
pthread_t SQLGetConnectAttrWAsync ( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen, void * callback );
pthread_t SQLGetConnectOptionAsync ( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval, void * callback );
pthread_t SQLGetConnectOptionWAsync ( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval, void * callback );
pthread_t SQLGetCursorNameAsync ( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor, void * callback );
pthread_t SQLGetCursorNameWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor, void * callback );
pthread_t SQLGetDataAsync ( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fCType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLGetDescFieldAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength, void * callback );
pthread_t SQLGetDescFieldWAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength, void * callback );
pthread_t SQLGetDescRecAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull, void * callback );
pthread_t SQLGetDescRecWAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLWCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull, void * callback );
pthread_t SQLGetDiagFieldAsync ( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength, void * callback );
pthread_t SQLGetDiagFieldWAsync ( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength, void * callback );
pthread_t SQLGetDiagRecAsync ( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLCHAR * SQLstate, SQLINTEGER * SQLcode, SQLCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength, void * callback );
pthread_t SQLGetDiagRecWAsync ( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLWCHAR * SQLstate, SQLINTEGER * SQLcode, SQLWCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength, void * callback );
pthread_t SQLGetEnvAttrAsync ( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParamMax, SQLINTEGER * pcbParam, void * callback );
pthread_t SQLGetFunctionsAsync ( SQLHDBC  hdbc, SQLSMALLINT  fFunction, SQLSMALLINT * pfExists, void * callback );
pthread_t SQLGetInfoAsync ( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue, void * callback );
pthread_t SQLGetInfoWAsync ( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue, void * callback );
pthread_t SQLGetLengthAsync ( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  locator, SQLINTEGER * sLength, SQLINTEGER * ind, void * callback );
pthread_t SQLGetPositionAsync ( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind, void * callback );
pthread_t SQLGetPositionWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLWCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind, void * callback );
pthread_t SQLGetStmtAttrAsync ( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength, void * callback );
pthread_t SQLGetStmtAttrWAsync ( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength, void * callback );
pthread_t SQLGetStmtOptionAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam, void * callback );
pthread_t SQLGetStmtOptionWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam, void * callback );
pthread_t SQLGetSubStringAsync ( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind, void * callback );
pthread_t SQLGetSubStringWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind, void * callback );
pthread_t SQLGetTypeInfoAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType, void * callback );
pthread_t SQLGetTypeInfoWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType, void * callback );
pthread_t SQLLanguagesAsync ( SQLHSTMT  hstmt, void * callback );
pthread_t SQLMoreResultsAsync ( SQLHSTMT  hstmt, void * callback );
pthread_t SQLNativeSqlAsync ( SQLHDBC  hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr, void * callback );
pthread_t SQLNativeSqlWAsync ( SQLHDBC  hdbc, SQLWCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr, void * callback );
pthread_t SQLNextResultAsync ( SQLHSTMT  hstmt, SQLHSTMT  hstmt2, void * callback );
pthread_t SQLNumParamsAsync ( SQLHSTMT  hstmt, SQLSMALLINT * pcpar, void * callback );
pthread_t SQLNumResultColsAsync ( SQLHSTMT  hstmt, SQLSMALLINT * pccol, void * callback );
pthread_t SQLParamDataAsync ( SQLHSTMT  hstmt, SQLPOINTER * Value, void * callback );
pthread_t SQLParamOptionsAsync ( SQLHSTMT  hstmt, SQLINTEGER  crow, SQLINTEGER * pirow, void * callback );
pthread_t SQLPrepareAsync ( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr, void * callback );
pthread_t SQLPrepareWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr, void * callback );
pthread_t SQLPrimaryKeysAsync ( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, void * callback );
pthread_t SQLPrimaryKeysWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, void * callback );
pthread_t SQLProcedureColumnsAsync ( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName, void * callback );
pthread_t SQLProcedureColumnsWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName, void * callback );
pthread_t SQLProceduresAsync ( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName, void * callback );
pthread_t SQLProceduresWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName, void * callback );
pthread_t SQLPutDataAsync ( SQLHSTMT  hstmt, SQLPOINTER  Data, SQLINTEGER  SLen, void * callback );
pthread_t SQLReleaseEnvAsync ( SQLHENV  henv, void * callback );
pthread_t SQLRowCountAsync ( SQLHSTMT  hstmt, SQLINTEGER * pcrow, void * callback );
pthread_t SQLSetConnectAttrAsync ( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen, void * callback );
pthread_t SQLSetConnectAttrWAsync ( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen, void * callback );
pthread_t SQLSetConnectOptionAsync ( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam, void * callback );
pthread_t SQLSetConnectOptionWAsync ( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam, void * callback );
pthread_t SQLSetCursorNameAsync ( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursor, void * callback );
pthread_t SQLSetCursorNameWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursor, void * callback );
pthread_t SQLSetDescFieldAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen, void * callback );
pthread_t SQLSetDescFieldWAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen, void * callback );
pthread_t SQLSetDescRecAsync ( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  Type, SQLSMALLINT  subType, SQLINTEGER  fLength, SQLSMALLINT  fPrec, SQLSMALLINT  fScale, SQLPOINTER  Value, SQLINTEGER * sLength, SQLINTEGER * indic, void * callback );
pthread_t SQLSetEnvAttrAsync ( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParam, void * callback );
pthread_t SQLSetParamAsync ( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fCType, SQLSMALLINT  fSqlType, SQLINTEGER  cbColDef, SQLSMALLINT  ibScale, SQLPOINTER  rgbValue, SQLINTEGER * pcbValue, void * callback );
pthread_t SQLSetStmtAttrAsync ( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam, void * callback );
pthread_t SQLSetStmtAttrWAsync ( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam, void * callback );
pthread_t SQLSetStmtOptionAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam, void * callback );
pthread_t SQLSetStmtOptionWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam, void * callback );
pthread_t SQLSpecialColumnsAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable, void * callback );
pthread_t SQLSpecialColumnsWAsync ( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLWCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable, void * callback );
pthread_t SQLStartTranAsync ( SQLSMALLINT  htype, SQLHENV  henv, SQLINTEGER  mode, SQLINTEGER  clevel, void * callback );
pthread_t SQLStatisticsAsync ( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres, void * callback );
pthread_t SQLStatisticsWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres, void * callback );
pthread_t SQLTablePrivilegesAsync ( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, void * callback );
pthread_t SQLTablePrivilegesWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, void * callback );
pthread_t SQLTablesAsync ( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szTableType, SQLSMALLINT  cbTableType, void * callback );
pthread_t SQLTablesWAsync ( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szTableType, SQLSMALLINT  cbTableType, void * callback );
pthread_t SQLTransactAsync ( SQLHENV  henv, SQLHDBC  hdbc, SQLSMALLINT  fType, void * callback );
pthread_t SQL400VersionAsync ( SQLPOINTER  outversion, SQLINTEGER  outlen, void * callback );
pthread_t SQL400Stmt2HdbcAsync ( SQLHSTMT  hstmt, SQLINTEGER * ohnd, void * callback );
pthread_t SQL400ConnectAsync ( SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400ConnectWAsync ( SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400pConnectAsync ( SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400pConnectWAsync ( SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400ConnectUtf8Async ( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400pConnectUtf8Async ( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400ConnectUtf16Async ( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400pConnectUtf16Async ( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib, void * callback );
pthread_t SQL400CloseAsync ( SQLHDBC  hdbc, void * callback );
pthread_t SQL400pCloseAsync ( SQLHDBC  hdbc, void * callback );
pthread_t SQL400CloseAllStmtsAsync ( SQLHDBC  hdbc, void * callback );
pthread_t SQL400CmdAsync ( SQLHDBC  hdbc, SQLCHAR * cmd, void * callback );
pthread_t SQL400ChgLiblAsync ( SQLHDBC  hdbc, SQLCHAR * libl, void * callback );
pthread_t SQL400ChgCurLibAsync ( SQLHDBC  hdbc, SQLCHAR * curlib, void * callback );
pthread_t SQL400ToUtf8Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, void * callback );
pthread_t SQL400FromUtf8Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400ToUtf16Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, void * callback );
pthread_t SQL400FromUtf16Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400IgnoreNullToUtf8Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, void * callback );
pthread_t SQL400IgnoreNullFromUtf8Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400IgnoreNullToUtf16Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, void * callback );
pthread_t SQL400IgnoreNullFromUtf16Async ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400AnyToAnyAsync ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400AnyFromAnyAsync ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400IgnoreNullAnyToAnyAsync ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400IgnoreNullAnyFromAnyAsync ( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid, void * callback );
pthread_t SQL400JsonAsync ( SQLHDBC  hdbc, SQLCHAR * injson, SQLINTEGER  inlen, SQLCHAR * outjson, SQLINTEGER  outlen, void * callback );

/* ===================================================
 * ILE CLI interfaces
 * ===================================================
 */

SQLRETURN ILE_SQLAllocConnect( SQLHENV  henv, SQLHDBC * phdbc );
SQLRETURN ILE_SQLAllocEnv( SQLHENV * phenv );
SQLRETURN ILE_SQLAllocHandle( SQLSMALLINT  htype, SQLINTEGER  ihnd, SQLINTEGER * ohnd );
SQLRETURN ILE_SQLAllocStmt( SQLHDBC  hdbc, SQLHSTMT * phstmt );
SQLRETURN ILE_SQLBindCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  iType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLBindFileToCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * sLen, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLBindFileToParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  iType, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLBindParam( SQLHSTMT  hstmt, SQLSMALLINT  iparm, SQLSMALLINT  iType, SQLSMALLINT  pType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLBindParameter( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fParamType, SQLSMALLINT  fCType, SQLSMALLINT  fSQLType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLCancel( SQLHSTMT  hstmt );
SQLRETURN ILE_SQLCloseCursor( SQLHSTMT  hstmt );
SQLRETURN ILE_SQLColAttribute( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
SQLRETURN ILE_SQLColAttributeW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
SQLRETURN ILE_SQLColAttributes( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
SQLRETURN ILE_SQLColAttributesW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLWCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
SQLRETURN ILE_SQLColumnPrivileges( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN ILE_SQLColumnPrivilegesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN ILE_SQLColumns( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN ILE_SQLColumnsW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN ILE_SQLConnect( SQLHDBC  hdbc, SQLCHAR * szDSN, SQLSMALLINT  cbDSN, SQLCHAR * szUID, SQLSMALLINT  cbUID, SQLCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
SQLRETURN ILE_SQLConnectW( SQLHDBC  hdbc, SQLWCHAR * szDSN, SQLSMALLINT  cbDSN, SQLWCHAR * szUID, SQLSMALLINT  cbUID, SQLWCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
SQLRETURN ILE_SQLCopyDesc( SQLHDESC  sDesc, SQLHDESC  tDesc );
SQLRETURN ILE_SQLDataSources( SQLHENV  henv, SQLSMALLINT  fDirection, SQLCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
SQLRETURN ILE_SQLDataSourcesW( SQLHENV  henv, SQLSMALLINT  fDirection, SQLWCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLWCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
SQLRETURN ILE_SQLDescribeCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
SQLRETURN ILE_SQLDescribeColW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLWCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
SQLRETURN ILE_SQLDescribeParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
SQLRETURN ILE_SQLDisconnect( SQLHDBC  hdbc );
SQLRETURN ILE_SQLDriverConnect( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
SQLRETURN ILE_SQLDriverConnectW( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLWCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
SQLRETURN ILE_SQLEndTran( SQLSMALLINT  htype, SQLHENV  henv, SQLSMALLINT  ctype );
SQLRETURN ILE_SQLError( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
SQLRETURN ILE_SQLErrorW( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLWCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
SQLRETURN ILE_SQLExecDirect( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN ILE_SQLExecDirectW( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN ILE_SQLExecute( SQLHSTMT  hstmt );
SQLRETURN ILE_SQLExtendedFetch( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset, SQLINTEGER * pcrow, SQLSMALLINT * rgfRowStatus );
SQLRETURN ILE_SQLFetch( SQLHSTMT  hstmt );
SQLRETURN ILE_SQLFetchScroll( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset );
SQLRETURN ILE_SQLForeignKeys( SQLHSTMT  hstmt, SQLCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
SQLRETURN ILE_SQLForeignKeysW( SQLHSTMT  hstmt, SQLWCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLWCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLWCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLWCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLWCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLWCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
SQLRETURN ILE_SQLFreeConnect( SQLHDBC  hdbc );
SQLRETURN ILE_SQLFreeEnv( SQLHENV  henv );
SQLRETURN ILE_SQLFreeStmt( SQLHSTMT  hstmt, SQLSMALLINT  fOption );
SQLRETURN ILE_SQLFreeHandle( SQLSMALLINT  htype, SQLINTEGER  hndl );
SQLRETURN ILE_SQLGetCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
SQLRETURN ILE_SQLGetColW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
SQLRETURN ILE_SQLGetConnectAttr( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
SQLRETURN ILE_SQLGetConnectAttrW( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
SQLRETURN ILE_SQLGetConnectOption( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
SQLRETURN ILE_SQLGetConnectOptionW( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
SQLRETURN ILE_SQLGetCursorName( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
SQLRETURN ILE_SQLGetCursorNameW( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
SQLRETURN ILE_SQLGetData( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fCType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLGetDescField( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
SQLRETURN ILE_SQLGetDescFieldW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
SQLRETURN ILE_SQLGetDescRec( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
SQLRETURN ILE_SQLGetDescRecW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLWCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
SQLRETURN ILE_SQLGetDiagField( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
SQLRETURN ILE_SQLGetDiagFieldW( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
SQLRETURN ILE_SQLGetDiagRec( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLCHAR * SQLstate, SQLINTEGER * SQLcode, SQLCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
SQLRETURN ILE_SQLGetDiagRecW( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLWCHAR * SQLstate, SQLINTEGER * SQLcode, SQLWCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
SQLRETURN ILE_SQLGetEnvAttr( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParamMax, SQLINTEGER * pcbParam );
SQLRETURN ILE_SQLGetFunctions( SQLHDBC  hdbc, SQLSMALLINT  fFunction, SQLSMALLINT * pfExists );
SQLRETURN ILE_SQLGetInfo( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
SQLRETURN ILE_SQLGetInfoW( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
SQLRETURN ILE_SQLGetLength( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  locator, SQLINTEGER * sLength, SQLINTEGER * ind );
SQLRETURN ILE_SQLGetPosition( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
SQLRETURN ILE_SQLGetPositionW( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLWCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
SQLRETURN ILE_SQLGetStmtAttr( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
SQLRETURN ILE_SQLGetStmtAttrW( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
SQLRETURN ILE_SQLGetStmtOption( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
SQLRETURN ILE_SQLGetStmtOptionW( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
SQLRETURN ILE_SQLGetSubString( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
SQLRETURN ILE_SQLGetSubStringW( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
SQLRETURN ILE_SQLGetTypeInfo( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
SQLRETURN ILE_SQLGetTypeInfoW( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
SQLRETURN ILE_SQLLanguages( SQLHSTMT  hstmt );
SQLRETURN ILE_SQLMoreResults( SQLHSTMT  hstmt );
SQLRETURN ILE_SQLNativeSql( SQLHDBC  hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
SQLRETURN ILE_SQLNativeSqlW( SQLHDBC  hdbc, SQLWCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
SQLRETURN ILE_SQLNextResult( SQLHSTMT  hstmt, SQLHSTMT  hstmt2 );
SQLRETURN ILE_SQLNumParams( SQLHSTMT  hstmt, SQLSMALLINT * pcpar );
SQLRETURN ILE_SQLNumResultCols( SQLHSTMT  hstmt, SQLSMALLINT * pccol );
SQLRETURN ILE_SQLParamData( SQLHSTMT  hstmt, SQLPOINTER * Value );
SQLRETURN ILE_SQLParamOptions( SQLHSTMT  hstmt, SQLINTEGER  crow, SQLINTEGER * pirow );
SQLRETURN ILE_SQLPrepare( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN ILE_SQLPrepareW( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN ILE_SQLPrimaryKeys( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN ILE_SQLPrimaryKeysW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN ILE_SQLProcedureColumns( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN ILE_SQLProcedureColumnsW( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN ILE_SQLProcedures( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName );
SQLRETURN ILE_SQLProceduresW( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName );
SQLRETURN ILE_SQLPutData( SQLHSTMT  hstmt, SQLPOINTER  Data, SQLINTEGER  SLen );
SQLRETURN ILE_SQLReleaseEnv( SQLHENV  henv );
SQLRETURN ILE_SQLRowCount( SQLHSTMT  hstmt, SQLINTEGER * pcrow );
SQLRETURN ILE_SQLSetConnectAttr( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
SQLRETURN ILE_SQLSetConnectAttrW( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
SQLRETURN ILE_SQLSetConnectOption( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN ILE_SQLSetConnectOptionW( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN ILE_SQLSetCursorName( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursor );
SQLRETURN ILE_SQLSetCursorNameW( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursor );
SQLRETURN ILE_SQLSetDescField( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
SQLRETURN ILE_SQLSetDescFieldW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
SQLRETURN ILE_SQLSetDescRec( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  Type, SQLSMALLINT  subType, SQLINTEGER  fLength, SQLSMALLINT  fPrec, SQLSMALLINT  fScale, SQLPOINTER  Value, SQLINTEGER * sLength, SQLINTEGER * indic );
SQLRETURN ILE_SQLSetEnvAttr( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParam );
SQLRETURN ILE_SQLSetParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fCType, SQLSMALLINT  fSqlType, SQLINTEGER  cbColDef, SQLSMALLINT  ibScale, SQLPOINTER  rgbValue, SQLINTEGER * pcbValue );
SQLRETURN ILE_SQLSetStmtAttr( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
SQLRETURN ILE_SQLSetStmtAttrW( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
SQLRETURN ILE_SQLSetStmtOption( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN ILE_SQLSetStmtOptionW( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN ILE_SQLSpecialColumns( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
SQLRETURN ILE_SQLSpecialColumnsW( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLWCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
SQLRETURN ILE_SQLStartTran( SQLSMALLINT  htype, SQLHENV  henv, SQLINTEGER  mode, SQLINTEGER  clevel );
SQLRETURN ILE_SQLStatistics( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
SQLRETURN ILE_SQLStatisticsW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
SQLRETURN ILE_SQLTablePrivileges( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN ILE_SQLTablePrivilegesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN ILE_SQLTables( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szTableType, SQLSMALLINT  cbTableType );
SQLRETURN ILE_SQLTablesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szTableType, SQLSMALLINT  cbTableType );
SQLRETURN ILE_SQLTransact( SQLHENV  henv, SQLHDBC  hdbc, SQLSMALLINT  fType );

/* ===================================================
 * LIBDB400.A CLI interfaces (PASE old CLI driver)
 * ===================================================
 */

SQLRETURN libdb400_SQLAllocConnect( SQLHENV  henv, SQLHDBC * phdbc );
SQLRETURN libdb400_SQLAllocEnv( SQLHENV * phenv );
SQLRETURN libdb400_SQLAllocHandle( SQLSMALLINT  htype, SQLINTEGER  ihnd, SQLINTEGER * ohnd );
SQLRETURN libdb400_SQLAllocStmt( SQLHDBC  hdbc, SQLHSTMT * phstmt );
SQLRETURN libdb400_SQLBindCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  iType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLBindFileToCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * sLen, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLBindFileToParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  iType, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLBindParam( SQLHSTMT  hstmt, SQLSMALLINT  iparm, SQLSMALLINT  iType, SQLSMALLINT  pType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLBindParameter( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fParamType, SQLSMALLINT  fCType, SQLSMALLINT  fSQLType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLCancel( SQLHSTMT  hstmt );
SQLRETURN libdb400_SQLCloseCursor( SQLHSTMT  hstmt );
SQLRETURN libdb400_SQLColAttribute( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
SQLRETURN libdb400_SQLColAttributeW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
SQLRETURN libdb400_SQLColAttributes( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
SQLRETURN libdb400_SQLColAttributesW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLWCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
SQLRETURN libdb400_SQLColumnPrivileges( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN libdb400_SQLColumnPrivilegesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN libdb400_SQLColumns( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN libdb400_SQLColumnsW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN libdb400_SQLConnect( SQLHDBC  hdbc, SQLCHAR * szDSN, SQLSMALLINT  cbDSN, SQLCHAR * szUID, SQLSMALLINT  cbUID, SQLCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
SQLRETURN libdb400_SQLConnectW( SQLHDBC  hdbc, SQLWCHAR * szDSN, SQLSMALLINT  cbDSN, SQLWCHAR * szUID, SQLSMALLINT  cbUID, SQLWCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
SQLRETURN libdb400_SQLCopyDesc( SQLHDESC  sDesc, SQLHDESC  tDesc );
SQLRETURN libdb400_SQLDataSources( SQLHENV  henv, SQLSMALLINT  fDirection, SQLCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
SQLRETURN libdb400_SQLDataSourcesW( SQLHENV  henv, SQLSMALLINT  fDirection, SQLWCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLWCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
SQLRETURN libdb400_SQLDescribeCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
SQLRETURN libdb400_SQLDescribeColW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLWCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
SQLRETURN libdb400_SQLDescribeParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
SQLRETURN libdb400_SQLDisconnect( SQLHDBC  hdbc );
SQLRETURN libdb400_SQLDriverConnect( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
SQLRETURN libdb400_SQLDriverConnectW( SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLWCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
SQLRETURN libdb400_SQLEndTran( SQLSMALLINT  htype, SQLHENV  henv, SQLSMALLINT  ctype );
SQLRETURN libdb400_SQLError( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
SQLRETURN libdb400_SQLErrorW( SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLWCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
SQLRETURN libdb400_SQLExecDirect( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN libdb400_SQLExecDirectW( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN libdb400_SQLExecute( SQLHSTMT  hstmt );
SQLRETURN libdb400_SQLExtendedFetch( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset, SQLINTEGER * pcrow, SQLSMALLINT * rgfRowStatus );
SQLRETURN libdb400_SQLFetch( SQLHSTMT  hstmt );
SQLRETURN libdb400_SQLFetchScroll( SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset );
SQLRETURN libdb400_SQLForeignKeys( SQLHSTMT  hstmt, SQLCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
SQLRETURN libdb400_SQLForeignKeysW( SQLHSTMT  hstmt, SQLWCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLWCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLWCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLWCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLWCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLWCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
SQLRETURN libdb400_SQLFreeConnect( SQLHDBC  hdbc );
SQLRETURN libdb400_SQLFreeEnv( SQLHENV  henv );
SQLRETURN libdb400_SQLFreeStmt( SQLHSTMT  hstmt, SQLSMALLINT  fOption );
SQLRETURN libdb400_SQLFreeHandle( SQLSMALLINT  htype, SQLINTEGER  hndl );
SQLRETURN libdb400_SQLGetCol( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
SQLRETURN libdb400_SQLGetColW( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
SQLRETURN libdb400_SQLGetConnectAttr( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
SQLRETURN libdb400_SQLGetConnectAttrW( SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
SQLRETURN libdb400_SQLGetConnectOption( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
SQLRETURN libdb400_SQLGetConnectOptionW( SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
SQLRETURN libdb400_SQLGetCursorName( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
SQLRETURN libdb400_SQLGetCursorNameW( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
SQLRETURN libdb400_SQLGetData( SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fCType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLGetDescField( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
SQLRETURN libdb400_SQLGetDescFieldW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
SQLRETURN libdb400_SQLGetDescRec( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
SQLRETURN libdb400_SQLGetDescRecW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLWCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
SQLRETURN libdb400_SQLGetDiagField( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
SQLRETURN libdb400_SQLGetDiagFieldW( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
SQLRETURN libdb400_SQLGetDiagRec( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLCHAR * SQLstate, SQLINTEGER * SQLcode, SQLCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
SQLRETURN libdb400_SQLGetDiagRecW( SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLWCHAR * SQLstate, SQLINTEGER * SQLcode, SQLWCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
SQLRETURN libdb400_SQLGetEnvAttr( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParamMax, SQLINTEGER * pcbParam );
SQLRETURN libdb400_SQLGetFunctions( SQLHDBC  hdbc, SQLSMALLINT  fFunction, SQLSMALLINT * pfExists );
SQLRETURN libdb400_SQLGetInfo( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
SQLRETURN libdb400_SQLGetInfoW( SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
SQLRETURN libdb400_SQLGetLength( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  locator, SQLINTEGER * sLength, SQLINTEGER * ind );
SQLRETURN libdb400_SQLGetPosition( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
SQLRETURN libdb400_SQLGetPositionW( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLWCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
SQLRETURN libdb400_SQLGetStmtAttr( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
SQLRETURN libdb400_SQLGetStmtAttrW( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
SQLRETURN libdb400_SQLGetStmtOption( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
SQLRETURN libdb400_SQLGetStmtOptionW( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
SQLRETURN libdb400_SQLGetSubString( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
SQLRETURN libdb400_SQLGetSubStringW( SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
SQLRETURN libdb400_SQLGetTypeInfo( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
SQLRETURN libdb400_SQLGetTypeInfoW( SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
SQLRETURN libdb400_SQLLanguages( SQLHSTMT  hstmt );
SQLRETURN libdb400_SQLMoreResults( SQLHSTMT  hstmt );
SQLRETURN libdb400_SQLNativeSql( SQLHDBC  hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
SQLRETURN libdb400_SQLNativeSqlW( SQLHDBC  hdbc, SQLWCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
SQLRETURN libdb400_SQLNextResult( SQLHSTMT  hstmt, SQLHSTMT  hstmt2 );
SQLRETURN libdb400_SQLNumParams( SQLHSTMT  hstmt, SQLSMALLINT * pcpar );
SQLRETURN libdb400_SQLNumResultCols( SQLHSTMT  hstmt, SQLSMALLINT * pccol );
SQLRETURN libdb400_SQLParamData( SQLHSTMT  hstmt, SQLPOINTER * Value );
SQLRETURN libdb400_SQLParamOptions( SQLHSTMT  hstmt, SQLINTEGER  crow, SQLINTEGER * pirow );
SQLRETURN libdb400_SQLPrepare( SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN libdb400_SQLPrepareW( SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
SQLRETURN libdb400_SQLPrimaryKeys( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN libdb400_SQLPrimaryKeysW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN libdb400_SQLProcedureColumns( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN libdb400_SQLProcedureColumnsW( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
SQLRETURN libdb400_SQLProcedures( SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName );
SQLRETURN libdb400_SQLProceduresW( SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName );
SQLRETURN libdb400_SQLPutData( SQLHSTMT  hstmt, SQLPOINTER  Data, SQLINTEGER  SLen );
SQLRETURN libdb400_SQLReleaseEnv( SQLHENV  henv );
SQLRETURN libdb400_SQLRowCount( SQLHSTMT  hstmt, SQLINTEGER * pcrow );
SQLRETURN libdb400_SQLSetConnectAttr( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
SQLRETURN libdb400_SQLSetConnectAttrW( SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
SQLRETURN libdb400_SQLSetConnectOption( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN libdb400_SQLSetConnectOptionW( SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN libdb400_SQLSetCursorName( SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursor );
SQLRETURN libdb400_SQLSetCursorNameW( SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursor );
SQLRETURN libdb400_SQLSetDescField( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
SQLRETURN libdb400_SQLSetDescFieldW( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
SQLRETURN libdb400_SQLSetDescRec( SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  Type, SQLSMALLINT  subType, SQLINTEGER  fLength, SQLSMALLINT  fPrec, SQLSMALLINT  fScale, SQLPOINTER  Value, SQLINTEGER * sLength, SQLINTEGER * indic );
SQLRETURN libdb400_SQLSetEnvAttr( SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParam );
SQLRETURN libdb400_SQLSetParam( SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fCType, SQLSMALLINT  fSqlType, SQLINTEGER  cbColDef, SQLSMALLINT  ibScale, SQLPOINTER  rgbValue, SQLINTEGER * pcbValue );
SQLRETURN libdb400_SQLSetStmtAttr( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
SQLRETURN libdb400_SQLSetStmtAttrW( SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
SQLRETURN libdb400_SQLSetStmtOption( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN libdb400_SQLSetStmtOptionW( SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
SQLRETURN libdb400_SQLSpecialColumns( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
SQLRETURN libdb400_SQLSpecialColumnsW( SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLWCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
SQLRETURN libdb400_SQLStartTran( SQLSMALLINT  htype, SQLHENV  henv, SQLINTEGER  mode, SQLINTEGER  clevel );
SQLRETURN libdb400_SQLStatistics( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
SQLRETURN libdb400_SQLStatisticsW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
SQLRETURN libdb400_SQLTablePrivileges( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN libdb400_SQLTablePrivilegesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
SQLRETURN libdb400_SQLTables( SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szTableType, SQLSMALLINT  cbTableType );
SQLRETURN libdb400_SQLTablesW( SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szTableType, SQLSMALLINT  cbTableType );
SQLRETURN libdb400_SQLTransact( SQLHENV  henv, SQLHDBC  hdbc, SQLSMALLINT  fType );

/* ===================================================
 * trace dump
 * ===================================================
 */

void dump_SQLAllocConnect(SQLRETURN sqlrc,  SQLHENV  henv, SQLHDBC * phdbc );
void dump_SQLAllocEnv(SQLRETURN sqlrc,  SQLHENV * phenv );
void dump_SQLAllocHandle(SQLRETURN sqlrc,  SQLSMALLINT  htype, SQLINTEGER  ihnd, SQLINTEGER * ohnd );
void dump_SQLAllocStmt(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLHSTMT * phstmt );
void dump_SQLBindCol(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  iType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
void dump_SQLBindFileToCol(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * sLen, SQLINTEGER * pcbValue );
void dump_SQLBindFileToParam(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  iType, SQLCHAR * fName, SQLSMALLINT * fNameLen, SQLINTEGER * fOptions, SQLSMALLINT  fValueMax, SQLINTEGER * pcbValue );
void dump_SQLBindParam(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  iparm, SQLSMALLINT  iType, SQLSMALLINT  pType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER * pcbValue );
void dump_SQLBindParameter(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fParamType, SQLSMALLINT  fCType, SQLSMALLINT  fSQLType, SQLINTEGER  pLen, SQLSMALLINT  pScale, SQLPOINTER  pData, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
void dump_SQLCancel(SQLRETURN sqlrc,  SQLHSTMT  hstmt );
void dump_SQLCloseCursor(SQLRETURN sqlrc,  SQLHSTMT  hstmt );
void dump_SQLColAttribute(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
void dump_SQLColAttributeW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLPOINTER  rgbDesc, SQLSMALLINT  cbDescMax, SQLSMALLINT * pcbDesc, SQLPOINTER  pfDesc );
void dump_SQLColAttributes(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
void dump_SQLColAttributesW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fDescType, SQLWCHAR * rgbDesc, SQLINTEGER  cbDescMax, SQLINTEGER * pcbDesc, SQLINTEGER * pfDesc );
void dump_SQLColumnPrivileges(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
void dump_SQLColumnPrivilegesW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
void dump_SQLColumns(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
void dump_SQLColumnsW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
void dump_SQLConnect(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLCHAR * szDSN, SQLSMALLINT  cbDSN, SQLCHAR * szUID, SQLSMALLINT  cbUID, SQLCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
void dump_SQLConnectW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLWCHAR * szDSN, SQLSMALLINT  cbDSN, SQLWCHAR * szUID, SQLSMALLINT  cbUID, SQLWCHAR * szAuthStr, SQLSMALLINT  cbAuthStr );
void dump_SQLCopyDesc(SQLRETURN sqlrc,  SQLHDESC  sDesc, SQLHDESC  tDesc );
void dump_SQLDataSources(SQLRETURN sqlrc,  SQLHENV  henv, SQLSMALLINT  fDirection, SQLCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
void dump_SQLDataSourcesW(SQLRETURN sqlrc,  SQLHENV  henv, SQLSMALLINT  fDirection, SQLWCHAR * szDSN, SQLSMALLINT  cbDSNMax, SQLSMALLINT * pcbDSN, SQLWCHAR * szDescription, SQLSMALLINT  cbDescriptionMax, SQLSMALLINT * pcbDescription );
void dump_SQLDescribeCol(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
void dump_SQLDescribeColW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLWCHAR * szColName, SQLSMALLINT  cbColNameMax, SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
void dump_SQLDescribeParam(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT * pfSqlType, SQLINTEGER * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable );
void dump_SQLDisconnect(SQLRETURN sqlrc,  SQLHDBC  hdbc );
void dump_SQLDriverConnect(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
void dump_SQLDriverConnectW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT  cbConnStrin, SQLWCHAR * szConnStrOut, SQLSMALLINT  cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLSMALLINT  fDriverCompletion );
void dump_SQLEndTran(SQLRETURN sqlrc,  SQLSMALLINT  htype, SQLHENV  henv, SQLSMALLINT  ctype );
void dump_SQLError(SQLRETURN sqlrc,  SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
void dump_SQLErrorW(SQLRETURN sqlrc,  SQLHENV  henv, SQLHDBC  hdbc, SQLHSTMT  hstmt, SQLWCHAR * szSqlState, SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg, SQLSMALLINT  cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg );
void dump_SQLExecDirect(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
void dump_SQLExecDirectW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
void dump_SQLExecute(SQLRETURN sqlrc,  SQLHSTMT  hstmt );
void dump_SQLExtendedFetch(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset, SQLINTEGER * pcrow, SQLSMALLINT * rgfRowStatus );
void dump_SQLFetch(SQLRETURN sqlrc,  SQLHSTMT  hstmt );
void dump_SQLFetchScroll(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOrient, SQLINTEGER  fOffset );
void dump_SQLForeignKeys(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
void dump_SQLForeignKeysW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szPkTableQualifier, SQLSMALLINT  cbPkTableQualifier, SQLWCHAR * szPkTableOwner, SQLSMALLINT  cbPkTableOwner, SQLWCHAR * szPkTableName, SQLSMALLINT  cbPkTableName, SQLWCHAR * szFkTableQualifier, SQLSMALLINT  cbFkTableQualifier, SQLWCHAR * szFkTableOwner, SQLSMALLINT  cbFkTableOwner, SQLWCHAR * szFkTableName, SQLSMALLINT  cbFkTableName );
void dump_SQLFreeConnect(SQLRETURN sqlrc,  SQLHDBC  hdbc );
void dump_SQLFreeEnv(SQLRETURN sqlrc,  SQLHENV  henv );
void dump_SQLFreeStmt(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOption );
void dump_SQLFreeHandle(SQLRETURN sqlrc,  SQLSMALLINT  htype, SQLINTEGER  hndl );
void dump_SQLGetCol(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
void dump_SQLGetColW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  itype, SQLPOINTER  tval, SQLINTEGER  blen, SQLINTEGER * olen );
void dump_SQLGetConnectAttr(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
void dump_SQLGetConnectAttrW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLINTEGER  attr, SQLPOINTER  oval, SQLINTEGER  ilen, SQLINTEGER * olen );
void dump_SQLGetConnectOption(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
void dump_SQLGetConnectOptionW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  iopt, SQLPOINTER  oval );
void dump_SQLGetCursorName(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
void dump_SQLGetCursorNameW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursorMax, SQLSMALLINT * pcbCursor );
void dump_SQLGetData(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  icol, SQLSMALLINT  fCType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * pcbValue );
void dump_SQLGetDescField(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
void dump_SQLGetDescFieldW(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fieldID, SQLPOINTER  fValue, SQLINTEGER  fLength, SQLINTEGER * stLength );
void dump_SQLGetDescRec(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
void dump_SQLGetDescRecW(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLWCHAR * fname, SQLSMALLINT  bufLen, SQLSMALLINT * sLength, SQLSMALLINT * sType, SQLSMALLINT * sbType, SQLINTEGER * fLength, SQLSMALLINT * fprec, SQLSMALLINT * fscale, SQLSMALLINT * fnull );
void dump_SQLGetDiagField(SQLRETURN sqlrc,  SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
void dump_SQLGetDiagFieldW(SQLRETURN sqlrc,  SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLSMALLINT  diagID, SQLPOINTER  dValue, SQLSMALLINT  bLength, SQLSMALLINT * sLength );
void dump_SQLGetDiagRec(SQLRETURN sqlrc,  SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLCHAR * SQLstate, SQLINTEGER * SQLcode, SQLCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
void dump_SQLGetDiagRecW(SQLRETURN sqlrc,  SQLSMALLINT  hType, SQLINTEGER  hndl, SQLSMALLINT  rcdNum, SQLWCHAR * SQLstate, SQLINTEGER * SQLcode, SQLWCHAR * msgText, SQLSMALLINT  bLength, SQLSMALLINT * SLength );
void dump_SQLGetEnvAttr(SQLRETURN sqlrc,  SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParamMax, SQLINTEGER * pcbParam );
void dump_SQLGetFunctions(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  fFunction, SQLSMALLINT * pfExists );
void dump_SQLGetInfo(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
void dump_SQLGetInfoW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  fInfoType, SQLPOINTER  rgbInfoValue, SQLSMALLINT  cbInfoValueMax, SQLSMALLINT * pcbInfoValue );
void dump_SQLGetLength(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  locator, SQLINTEGER * sLength, SQLINTEGER * ind );
void dump_SQLGetPosition(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
void dump_SQLGetPositionW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  srchLocator, SQLWCHAR * srchLiteral, SQLINTEGER  srchLiteralLen, SQLINTEGER  fPosition, SQLINTEGER * located, SQLINTEGER * ind );
void dump_SQLGetStmtAttr(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
void dump_SQLGetStmtAttrW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pvParam, SQLINTEGER  bLength, SQLINTEGER * SLength );
void dump_SQLGetStmtOption(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
void dump_SQLGetStmtOptionW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  pvParam );
void dump_SQLGetSubString(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
void dump_SQLGetSubStringW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  locType, SQLINTEGER  srceLocator, SQLINTEGER  fPosition, SQLINTEGER  length, SQLSMALLINT  tType, SQLPOINTER  rgbValue, SQLINTEGER  cbValueMax, SQLINTEGER * StringLength, SQLINTEGER * ind );
void dump_SQLGetTypeInfo(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
void dump_SQLGetTypeInfoW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fSqlType );
void dump_SQLLanguages(SQLRETURN sqlrc,  SQLHSTMT  hstmt );
void dump_SQLMoreResults(SQLRETURN sqlrc,  SQLHSTMT  hstmt );
void dump_SQLNativeSql(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
void dump_SQLNativeSqlW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLWCHAR * szSqlStrIn, SQLINTEGER  cbSqlStrIn, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStrMax, SQLINTEGER * pcbSqlStr );
void dump_SQLNextResult(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLHSTMT  hstmt2 );
void dump_SQLNumParams(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT * pcpar );
void dump_SQLNumResultCols(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT * pccol );
void dump_SQLParamData(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLPOINTER * Value );
void dump_SQLParamOptions(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER  crow, SQLINTEGER * pirow );
void dump_SQLPrepare(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
void dump_SQLPrepareW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szSqlStr, SQLINTEGER  cbSqlStr );
void dump_SQLPrimaryKeys(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
void dump_SQLPrimaryKeysW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
void dump_SQLProcedureColumns(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName, SQLCHAR * szColumnName, SQLSMALLINT  cbColumnName );
void dump_SQLProcedureColumnsW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName, SQLWCHAR * szColumnName, SQLSMALLINT  cbColumnName );
void dump_SQLProcedures(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLCHAR * szProcName, SQLSMALLINT  cbProcName );
void dump_SQLProceduresW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szProcQualifier, SQLSMALLINT  cbProcQualifier, SQLWCHAR * szProcOwner, SQLSMALLINT  cbProcOwner, SQLWCHAR * szProcName, SQLSMALLINT  cbProcName );
void dump_SQLPutData(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLPOINTER  Data, SQLINTEGER  SLen );
void dump_SQLReleaseEnv(SQLRETURN sqlrc,  SQLHENV  henv );
void dump_SQLRowCount(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER * pcrow );
void dump_SQLSetConnectAttr(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
void dump_SQLSetConnectAttrW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLINTEGER  attrib, SQLPOINTER  vParam, SQLINTEGER  inlen );
void dump_SQLSetConnectOption(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
void dump_SQLSetConnectOptionW(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLSMALLINT  fOption, SQLPOINTER  vParam );
void dump_SQLSetCursorName(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szCursor, SQLSMALLINT  cbCursor );
void dump_SQLSetCursorNameW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szCursor, SQLSMALLINT  cbCursor );
void dump_SQLSetDescField(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
void dump_SQLSetDescFieldW(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  fID, SQLPOINTER  Value, SQLINTEGER  buffLen );
void dump_SQLSetDescRec(SQLRETURN sqlrc,  SQLHDESC  hdesc, SQLSMALLINT  rcdNum, SQLSMALLINT  Type, SQLSMALLINT  subType, SQLINTEGER  fLength, SQLSMALLINT  fPrec, SQLSMALLINT  fScale, SQLPOINTER  Value, SQLINTEGER * sLength, SQLINTEGER * indic );
void dump_SQLSetEnvAttr(SQLRETURN sqlrc,  SQLHENV  hEnv, SQLINTEGER  fAttribute, SQLPOINTER  pParam, SQLINTEGER  cbParam );
void dump_SQLSetParam(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  ipar, SQLSMALLINT  fCType, SQLSMALLINT  fSqlType, SQLINTEGER  cbColDef, SQLSMALLINT  ibScale, SQLPOINTER  rgbValue, SQLINTEGER * pcbValue );
void dump_SQLSetStmtAttr(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
void dump_SQLSetStmtAttrW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER  fAttr, SQLPOINTER  pParam, SQLINTEGER  vParam );
void dump_SQLSetStmtOption(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
void dump_SQLSetStmtOptionW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fOption, SQLPOINTER  vParam );
void dump_SQLSpecialColumns(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
void dump_SQLSpecialColumnsW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLSMALLINT  fColType, SQLWCHAR * szTableQual, SQLSMALLINT  cbTableQual, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fScope, SQLSMALLINT  fNullable );
void dump_SQLStartTran(SQLRETURN sqlrc,  SQLSMALLINT  htype, SQLHENV  henv, SQLINTEGER  mode, SQLINTEGER  clevel );
void dump_SQLStatistics(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
void dump_SQLStatisticsW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLSMALLINT  fUnique, SQLSMALLINT  fres );
void dump_SQLTablePrivileges(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName );
void dump_SQLTablePrivilegesW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName );
void dump_SQLTables(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLCHAR * szTableName, SQLSMALLINT  cbTableName, SQLCHAR * szTableType, SQLSMALLINT  cbTableType );
void dump_SQLTablesW(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLWCHAR * szTableQualifier, SQLSMALLINT  cbTableQualifier, SQLWCHAR * szTableOwner, SQLSMALLINT  cbTableOwner, SQLWCHAR * szTableName, SQLSMALLINT  cbTableName, SQLWCHAR * szTableType, SQLSMALLINT  cbTableType );
void dump_SQLTransact(SQLRETURN sqlrc,  SQLHENV  henv, SQLHDBC  hdbc, SQLSMALLINT  fType );
void dump_SQL400Version(SQLRETURN sqlrc,  SQLPOINTER  outversion, SQLINTEGER  outlen );
void dump_SQLOverrideCCSID400(SQLRETURN sqlrc,  SQLINTEGER  newCCSID );
void dump_SQL400Stmt2Hdbc(SQLRETURN sqlrc,  SQLHSTMT  hstmt, SQLINTEGER * ohnd );
void dump_SQL400Connect(SQLRETURN sqlrc,  SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400ConnectW(SQLRETURN sqlrc,  SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400pConnect(SQLRETURN sqlrc,  SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400pConnectW(SQLRETURN sqlrc,  SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400ConnectUtf8(SQLRETURN sqlrc,  SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400pConnectUtf8(SQLRETURN sqlrc,  SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400ConnectUtf16(SQLRETURN sqlrc,  SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400pConnectUtf16(SQLRETURN sqlrc,  SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
void dump_SQL400Close(SQLRETURN sqlrc,  SQLHDBC  hdbc );
void dump_SQL400pClose(SQLRETURN sqlrc,  SQLHDBC  hdbc );
void dump_SQL400CloseAllStmts(SQLRETURN sqlrc,  SQLHDBC  hdbc );
void dump_SQL400Cmd(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLCHAR * cmd );
void dump_SQL400ChgLibl(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLCHAR * libl );
void dump_SQL400ChgCurLib(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLCHAR * curlib );
void dump_SQL400ToUtf8(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
void dump_SQL400FromUtf8(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
void dump_SQL400ToUtf16(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
void dump_SQL400FromUtf16(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
void dump_SQL400IgnoreNullToUtf8(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
void dump_SQL400IgnoreNullFromUtf8(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
void dump_SQL400IgnoreNullToUtf16(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
void dump_SQL400IgnoreNullFromUtf16(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
void dump_SQL400AnyToAny(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
void dump_SQL400AnyFromAny(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
void dump_SQL400IgnoreNullAnyToAny(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
void dump_SQL400IgnoreNullAnyFromAny(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
void dump_SQL400Json(SQLRETURN sqlrc,  SQLHDBC  hdbc, SQLCHAR * injson, SQLINTEGER  inlen, SQLCHAR * outjson, SQLINTEGER  outlen );

/* ===================================================
 * INTERNAL USE
 * ===================================================
 */
#ifndef SQL_ATTR_SERVERMODE_SUBSYSTEM
#define SQL_ATTR_SERVERMODE_SUBSYSTEM 10204
#endif

SQLRETURN custom_SQL400Version( SQLPOINTER  outversion, SQLINTEGER  outlen );
SQLRETURN custom_SQLOverrideCCSID400( SQLINTEGER  newCCSID );
SQLRETURN custom_SQL400Stmt2Hdbc( SQLHSTMT  hstmt, SQLINTEGER * ohnd );
SQLRETURN custom_SQL400Connect( SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400ConnectW( SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400pConnect( SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400pConnectW( SQLWCHAR * db, SQLWCHAR * uid, SQLWCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400ConnectUtf8( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400pConnectUtf8( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400ConnectUtf16( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400pConnectUtf16( SQLINTEGER  accsid, SQLCHAR * db, SQLCHAR * uid, SQLCHAR * pwd, SQLCHAR * qual, SQLINTEGER * ohnd, SQLINTEGER  acommit, SQLCHAR * alibl, SQLCHAR * acurlib );
SQLRETURN custom_SQL400Close( SQLHDBC  hdbc );
SQLRETURN custom_SQL400pClose( SQLHDBC  hdbc );
SQLRETURN custom_SQL400CloseAllStmts( SQLHDBC  hdbc );
SQLRETURN custom_SQL400Cmd( SQLHDBC  hdbc, SQLCHAR * cmd );
SQLRETURN custom_SQL400ChgLibl( SQLHDBC  hdbc, SQLCHAR * libl );
SQLRETURN custom_SQL400ChgCurLib( SQLHDBC  hdbc, SQLCHAR * curlib );
SQLRETURN custom_SQL400ToUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN custom_SQL400FromUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400ToUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN custom_SQL400FromUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400IgnoreNullToUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN custom_SQL400IgnoreNullFromUtf8( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400IgnoreNullToUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid );
SQLRETURN custom_SQL400IgnoreNullFromUtf16( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400AnyToAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400AnyFromAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400IgnoreNullAnyToAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400IgnoreNullAnyFromAny( SQLHDBC  hdbc, SQLPOINTER  inparm, SQLINTEGER  inlen, SQLPOINTER  outparm, SQLINTEGER  outlen, SQLINTEGER  inccsid, SQLINTEGER  outccsid );
SQLRETURN custom_SQL400Json( SQLHDBC  hdbc, SQLCHAR * injson, SQLINTEGER  inlen, SQLCHAR * outjson, SQLINTEGER  outlen );
SQLRETURN custom_SQLSetEnvCCSID( SQLHANDLE env, int myccsid);


#ifdef __cplusplus 
}
#endif /* __cplusplus */
#endif /* _PASECLIASYNC_H */
