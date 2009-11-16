/*
 * JBoss, Home of Professional Open Source
 * Copyright 2009, Red Hat, Inc., and others contributors as indicated
 * by the @authors tag. All rights reserved.
 * See the copyright.txt in the distribution for a
 * full listing of individual contributors.
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License, v. 2.1.
 * This program is distributed in the hope that it will be useful, but WITHOUT A
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License,
 * v.2.1 along with this distribution; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
#include "AtmiBrokerServerControl.h"

#include <stdlib.h>

#include "xatmi.h"
#include "userlogc.h"
#include "string.h"

#include "ace/Thread.h"
#include "ace/Synch.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include "tx.h"
#endif

static ACE_Mutex mutex_;
const char *MSG1 = "CLIENT REQUEST		";
const char *MSG2 = "PAUSE - CLIENT REQUEST";

// data type for controlling the work done by each thread
typedef struct thr_arg {
	int failonerror;
	const char *data;
	const char *msg;
	const char *svc;
	const char *sndtype;
	const char *rcvtype;
	long flags;
	int expect;
	long expect2;
	int result;
} thr_arg_t;

// arbitray subtype for testing C type buffers
typedef struct subtype {
	char data[64];
	int id;
	char op;
} subtype_t;

static void do_assert(int failonerror, int* res, int cond, const char *fmt, ...) {
	char str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, 1024, fmt, args);
	va_end(args);

	if (!cond) {
		*res = 1;
		userlogc((char*) "UNSUCCESSFULL assert %s", str);
		if (failonerror)
			_exit(1);
	}

//	userlogc((char*) "successful assert for: cond=%d (%s)", cond, str);
}

//static int do_tpcall(int failonerror, const char *data, const char *msg, const char *svc, const char *sndtype, long flags, int expect) {
static int do_tpcall(thr_arg_t *args) {
	int tpstatus;
	char *rbuf;
	char *sbuf;
	char type[20];
	char subtype[20];
	int isSbCType = (strcmp(args->sndtype, X_C_TYPE) == 0); // is send buffer is X_C_TYPE
	int isRbCType = (strcmp(args->rcvtype, X_C_TYPE) == 0); // is recv buffer is X_C_TYPE
	long sbufsz = (isSbCType ? 0 : strlen(args->data) + 1);	// api sets the buffer size if X_C_TYPE
	long rbufsz = (isRbCType ? 0 : 64);
	int res;

	sbuf = tpalloc((char *) args->sndtype, (isSbCType ? (char*) "sub_type" : NULL), sbufsz);
	rbuf = tpalloc((char *) args->rcvtype, (isRbCType ? (char*) "sub_type" : NULL), rbufsz);

	do_assert(args->failonerror, &args->result, sbuf != 0, "tpalloc send buf failed tperrno=%d", tperrno);
	do_assert(args->failonerror, &args->result, rbuf != 0, "tpalloc recv buf failed tperrno=%d", tperrno);

	strcpy(sbuf, args->data);
	memset(rbuf, 0, rbufsz);

	tptypes(sbuf, type, subtype);
	userlogc((char *) "sbuf type: %s rbuf type: %s type: %s subtype: %s %d vrs %d",
		args->sndtype, args->rcvtype, type, subtype, tpstatus, args->expect);

	tpstatus = tpcall((char *) args->svc, sbuf, sbufsz, (char **) &rbuf, &rbufsz, args->flags);
	res = (tperrno == args->expect ? 0 : 1);
	if (tpstatus)
		userlogc((char *) "tpcall returned %d tperrno=%d expect=%d", tpstatus, tperrno, args->expect);
	do_assert(args->failonerror, &args->result, tperrno == args->expect,
		"%s: wrong response from tpcall %s %s tpstatus=%d flags=%d expect=%d tperrno=%d",
		args->msg, args->svc, args->sndtype, tpstatus, args->flags, args->expect, tperrno);
	do_assert(args->failonerror, &args->result, tpurcode == args->expect2,
			"tpurcode: expected=%d tpurcode=%d",
			args->expect2, tpurcode);

	tpfree(sbuf);
	tpfree(rbuf);

	return res;
}

static int lotsofwork(int nthreads, ACE_THR_FUNC tfunc, thr_arg_t* arg) {
	ACE_thread_t *tids = new ACE_thread_t[nthreads];
	ACE_hthread_t *handles = new ACE_hthread_t[nthreads];
	int i;

	for (i = 0; i < nthreads; i++)
		handles[i] = 0;

	// spawn nthreads threads
	if (ACE_Thread::spawn_n(tids, // return thread id for each thread
		nthreads,
		tfunc, // entry point for new thread
		(void *) arg,	// args for thread entry point
		THR_JOINABLE | THR_NEW_LWP,
		ACE_DEFAULT_THREAD_PRIORITY,
		0, 0, handles) != (size_t) nthreads) {
				userlogc("Unable to start request number of threads\n");
	}

	for (int i = 0; i < nthreads; i++)
		if (handles[i] != 0)
			ACE_Thread::join(handles[i]);

	return arg->result;
}

// thread entry point
static void* work(void *args)
{
	(void) do_tpcall((thr_arg_t *) args);
	return args;
}

// another thread entry point
static int tcnt_ = 0;
static void* work2(void *args)
{
	thr_arg_t *params = (thr_arg_t *) args;
	char *s1, *s2;
	int ncalls = 2;
	int okcalls = 0;
	int rv;

	s1 = (char *) "BAR";
	s2 = (char *) "BAR";

	mutex_.acquire();
	tcnt_ += 1;
	tpfree(tpalloc((char *) params->sndtype, 0, 10));
#if 0	/* I've disabled using multiple service since it fails with just one service */
	if (tcnt_  % 2 == 0)
		s2 = (char *) "TestTPCall";
	else
		s1 = (char *) "TestTPCall";
#endif
	mutex_.release();

	ACE_OS::sleep(4);	// yield to ensure that all threads have initialised env (see bug BLACKTIE-211)

	for (int i = 0; i < ncalls; i++) {
		params->svc = s1;
		if ((rv = do_tpcall(params)))
			userlogc((char*) "%s: tpcall %d error: %d", params->svc, i, rv);
		else
			okcalls += 1;
		params->svc = s2;
		if ((rv = do_tpcall(params)))
			userlogc((char*) "%s: tpcall %d error: %d", params->svc, i, rv);
		else
			okcalls += 1;
	}

	userlogc("Thread (t) finished %d out of %d calls successful\n", okcalls, ncalls * 2);

	params->result = ((okcalls == ncalls * 2)?0:1);
	return args;
}

// XsdValidator is not thread safe
static int bug211() {
	thr_arg_t args = {1, MSG1, "bug211: two threads reading env", "BAR", X_OCTET, X_OCTET, 0, 0, 99};
	return lotsofwork(2, ACE_THR_FUNC(&work), &args);
}

// tpcall should return TPEINVAL if the service name is invalid
static int bug213() {
	thr_arg_t args = {1, MSG1, "bug213: NULL service name", NULL, X_OCTET, X_OCTET, 0, TPEINVAL, 0};
	return do_tpcall(&args);
}

// tpcall incorrectly returns TPNOTIME whenever the TPNOBLOCK or TPNOTIME flags are specified
static int bug212a() {
	// Specifying TPNOTIME means the caller wants to be imune to blocking conditions (such
	// as no output buffers). Thus if such a condition does not exist the call should succeed as normal.
	// However if bug 212 is present then the call returns TPNOTIME
	long flags2 = TPNOTRAN | TPNOTIME;
	thr_arg_t arg1 = {1, MSG1, "bug212a: TPNOTIME", "BAR", X_OCTET, X_OCTET, flags2, 0, 99};

	return lotsofwork(1, ACE_THR_FUNC(&work), &arg1);
}
static int bug212b() {
	// Similarly specifying TPNOBLOCK means that if a blocking condition does exist then the caller
	// should get the error TPEBLOCK
	// However if bug 212 is present then the call returns TPNOTIME
	long flags3 = TPNOTRAN | TPNOBLOCK;

	thr_arg_t args = {1, MSG1, "bug212b: TPNOBLOCK", "BAR", X_OCTET, X_OCTET, flags3, 0, 99};

	return lotsofwork(1, ACE_THR_FUNC(&work), &args);
}

// TPSIGRSTRT flag isn't supported on tpcall
static int bug214() {
	thr_arg_t args = {1, MSG1, "bug214: TPSIGRSTRT flag not supported on tpcall", "BAR", X_OCTET, X_OCTET, TPSIGRSTRT, 0, 99};
	return lotsofwork(1, ACE_THR_FUNC(&work), &args);
}

// tpcall failure with multiple threads
static int bug215() {
	thr_arg_t args = {0, MSG1, "bug215: tpcall failure with lots of threads", "BAR", X_OCTET, X_OCTET, 0, 0, 99};
	return lotsofwork(2, ACE_THR_FUNC(&work2), &args);
}

static int bug216a() {
	thr_arg_t args = {1, MSG1, "bug216: tp bufs should morph if they're the wrong type", "BAR", X_OCTET, X_C_TYPE, 0, 0, 99};
	return lotsofwork(1, ACE_THR_FUNC(&work), &args);
}

static int bug216b() {
	thr_arg_t args = {1, MSG1, "bug216: passing the wrong return buffer type with TPNOCHANGE",
		"BAR", X_OCTET, X_C_TYPE, TPNOCHANGE, TPEOTYPE, 99};
	return lotsofwork(1, ACE_THR_FUNC(&work), &args);
}

static int bug217() {
	thr_arg_t args = {1, MSG1, "bug217: make sure tpurcode works", "BAR", X_OCTET, X_OCTET, 0, 0, 99};
	(void) lotsofwork(1, ACE_THR_FUNC(&work), &args);
	return args.result;
}

static int t9001() {
	char *buf = (char *) tpalloc((char *) X_C_TYPE, (char*) "sub_type", 10);
	int res = 0;
	do_assert(1, &res, buf != 0, "tpalloc with X_C_TYPE and non-zero len: tperrno=%d (spec says size is optional)", tperrno);
	tpfree(buf);
	return res;
}

// sanity check
static int t0() {
	thr_arg_t args = {1, MSG1, "ok test", "BAR", X_OCTET, X_OCTET, 0, 0, 99};
	return do_tpcall(&args);
}

// tell the server to set a flag on tpreturn (should generate TPESVCERR)
static int t1() {
	thr_arg_t args = {1, "T1", "set flag on tpreturn should fail", "BAR", X_OCTET, X_OCTET, TPNOTRAN, TPESVCERR, 0};
	return do_tpcall(&args);
}
static int t2() {
	thr_arg_t args = {1, "T2", "tell the service to free the the service buffer", "BAR", X_OCTET, X_OCTET, TPNOTRAN, 0, 99};
	return do_tpcall(&args);
}

// telling the service to not tpreturn should generate an error
static int t3() {
	thr_arg_t args = {1, "T3", "no tpreturn", "BAR", X_OCTET, X_OCTET, 0, TPESVCERR, 0};
	return do_tpcall(&args);
}

// telling service to call tpreturn outside service routine should have no effect
static int t4() {
	thr_arg_t args = {1, "T4", "tpreturn outside service routing", "BAR", X_OCTET, X_OCTET, 0, 0, 99};
	return do_tpcall(&args);
}

static int t5() {
	thr_arg_t args = {1, "T5", "tpreturn TPFAIL", "BAR", X_OCTET, X_OCTET, 0, TPESVCFAIL, 99};
	return do_tpcall(&args);
}

static int tx = 0;
static int startTx() {
#ifndef WIN32
	if (tx && (tx_open() != TX_OK || tx_begin() != TX_OK))
		return 1;
#endif
	return 0;
}
static int endTx() {
#ifndef WIN32
	if (tx && (tx_commit() != TX_OK || tx_close() != TX_OK))
		return 1;
#endif
	return 0;
}

int run_client(int argc, char **argv) {
	int res = 0;
	int bug = 217;

	if (argc > 1)
		bug = atoi(argv[1]);

	userlogc((char*) "starting test %d", bug);

	if (startTx() != 0)
		userlogc((char*) "ERROR - Could not open or begin transaction: ");
	else {
		switch (bug) {
		case 211:	res = bug211(); break;
		case 2120:	res = bug212a(); break;
		case 2121:	res = bug212b(); break;
		case 213:	res = bug213(); break;
		case 214:	res = bug214(); break;
		case 215:	res = bug215(); break;
		case 2160:	res = bug216a(); break;
		case 2161:	res = bug216b(); break;

		case 217:	res = bug217(); break;
		case 9001:	res = t9001(); break;
		case 0:		res = t0(); break;
		case 1:		res = t1(); break;
		case 2:		res = t2(); break;
		case 3:		res = t3(); break;
		case 4:		res = t4(); break;
		case 5:		res = t5(); break;
		default: break;
		}

		if (endTx() != 0)
			userlogc((char*) "ERROR - Could not commit or close transaction: ");
	}

	userlogc((char*) "test %d %s with code %d", bug, (res == 0 ? "passed" : "failed"), res);

	return res;
}

void BAR (TPSVCINFO *);
void TestTPCall (TPSVCINFO *);

void BAR(TPSVCINFO * svcinfo) {
	char* buffer;
	int sendlen = 15;
	long rflag = 0L;
	int rval = TPSUCCESS;

	userlogc((char*) "bar called  - svc=%s data=%s len=%d flags=%d rcode=%d tperrno=%d",
		svcinfo->name, svcinfo->data, svcinfo->len, svcinfo->flags, 99, tperrno);

	if (strcmp(svcinfo->data, "T1") == 0)
		rflag = TPEBLOCK;
	else if (strcmp(svcinfo->data, "T2") == 0)
		tpfree(svcinfo->data);
	else if (strcmp(svcinfo->data, "T5") == 0)
		rval = TPFAIL;

	buffer = tpalloc((char *) "X_OCTET", 0, sendlen);
	strcpy(buffer, "BAR SAYS HELLO");

	if (strcmp(svcinfo->data, "T3") != 0)
		tpreturn(rval, 99, buffer, sendlen, rflag);

	if (strcmp(svcinfo->data, "T4") == 0)
		tpreturn(TPFAIL, 99, buffer, sendlen, rflag);

	if (tperrno)
		userlogc((char*) "bar returned: tperrno=%d", tperrno);
}

void TestTPCall(TPSVCINFO * svcinfo) {
	BAR(svcinfo);
}

/* the byte pattern written to file descriptor 1 to indicate that the server has advertised its services */
static const unsigned char HANDSHAKE[] = {83,69,82,86,73,67,69,83,32,82,69,65,68,89};
static const size_t HANDSHAKE_LEN = 14;

int run_server(int argc, char **argv) {
	int exit_status = serverinit(argc, argv);

	if (exit_status != -1) {
		tpadvertise((char *) "BAR", BAR);
		tpadvertise((char *) "TestTPCall", TestTPCall);
		if (write(1, HANDSHAKE, HANDSHAKE_LEN) != HANDSHAKE_LEN) {
			return -1;
		}

		/* flush stdout */
		fprintf(stdout, "\n");
		exit_status = serverrun();
	} else {
		userlogc((char*) "main Unexpected exception in serverrun()");
	}
	serverdone();
	return exit_status;
}

int main(int argc, char **argv) {
	int i;

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0)
            return run_server(argc, argv);
    }

    return run_client(argc, argv);
}