/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat, Inc., and others contributors as indicated
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
#include "TestAssert.h"

#include "BaseServerTest.h"

#include "xatmi.h"

#include "TestTPGetRply.h"
#include "Sleeper.h"

#include "malloc.h"

extern void testtpgetrply_service(TPSVCINFO *svcinfo);
extern void test_tpgetrply_TPNOBLOCK(TPSVCINFO *svcinfo);
extern void test_tpgetrply_TPGETANY_one(TPSVCINFO *svcinfo);
extern void test_tpgetrply_TPGETANY_two(TPSVCINFO *svcinfo);

void TestTPGetRply::setUp() {
	userlogc((char*) "TestTPGetRply::setUp");
	sendbuf = NULL;
	rcvbuf = NULL;
	testingTPGETANY = false;

	// Setup server
	BaseServerTest::setUp();

	// Do local work
	sendlen = strlen("grply") + 1;
	rcvlen = 22;
	BT_ASSERT((sendbuf = (char *) tpalloc((char*) "X_OCTET", NULL, sendlen))
			!= NULL);
	BT_ASSERT((rcvbuf = (char *) tpalloc((char*) "X_OCTET", NULL, rcvlen))
			!= NULL);
	strcpy(sendbuf, "grply");
	BT_ASSERT(tperrno == 0);
}

void TestTPGetRply::tearDown() {
	userlogc((char*) "TestTPGetRply::tearDown");
	// Do local work
	::tpfree( sendbuf);
	::tpfree( rcvbuf);
	if (testingTPGETANY) {
		int toCheck = tpunadvertise((char*) "DEBIT");
		BT_ASSERT(tperrno == 0);
		BT_ASSERT(toCheck != -1);
		toCheck = tpunadvertise((char*) "CREDIT");
		BT_ASSERT(tperrno == 0);
		BT_ASSERT(toCheck != -1);
	} else {
		int toCheck = tpunadvertise((char*) "TestTPGetrply");
		BT_ASSERT(tperrno == 0);
		BT_ASSERT(toCheck != -1);
	}

	// Clean up server
	BaseServerTest::tearDown();
}

void TestTPGetRply::test_tpgetrply() {
	userlogc((char*) "test_tpgetrply");

	int toCheck = tpadvertise((char*) "TestTPGetrply", testtpgetrply_service);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(toCheck != -1);

	int cd = ::tpacall((char*) "TestTPGetrply", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd != -1);
	BT_ASSERT(tperrno == 0);

	// RETRIEVE THE RESPONSE
	int toTest = ::tpgetrply(&cd, (char **) &rcvbuf, &rcvlen, 0);
	BT_ASSERT(tperrno != TPEINVAL);
	BT_ASSERT(tperrno != TPEBADDESC);
	BT_ASSERT(tperrno != TPEOTYPE);
	BT_ASSERT(tperrno != TPETIME);
	BT_ASSERT(tperrno != TPESVCFAIL);
	BT_ASSERT(tperrno != TPESVCERR);
	BT_ASSERT(tperrno != TPEBLOCK);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(strcmp(rcvbuf, "testtpgetrply_service") == 0);
	char* toTestS = (char*) malloc(110);
	sprintf(toTestS, "%d", toTest);
	BT_ASSERT_MESSAGE(toTestS, toTest == 0);
	free(toTestS);
}

// 8.5
void TestTPGetRply::test_tpgetrply_baddesc() {
	userlogc((char*) "test_tpgetrply_baddesc");

	int toCheck = tpadvertise((char*) "TestTPGetrply", testtpgetrply_service);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(toCheck != -1);

	int cd = 2;
	int valToTest = ::tpgetrply(&cd, (char **) &rcvbuf, &rcvlen, 0);
	BT_ASSERT(valToTest == -1);
	BT_ASSERT(tperrno != 0);
	BT_ASSERT(tperrno != TPEINVAL);
	BT_ASSERT(tperrno != TPEOTYPE);
	BT_ASSERT(tperrno != TPETIME);
	BT_ASSERT(tperrno != TPESVCFAIL);
	BT_ASSERT(tperrno != TPESVCERR);
	BT_ASSERT(tperrno != TPEBLOCK);
	BT_ASSERT(tperrno == TPEBADDESC);
}

void TestTPGetRply::test_tpgetrply_nullcd() {
	userlogc((char*) "test_tpgetrply_nullcd");

	int toCheck = tpadvertise((char*) "TestTPGetrply", testtpgetrply_service);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(toCheck != -1);

	int valToTest = ::tpgetrply(NULL, (char **) &rcvbuf, &rcvlen, 0);
	BT_ASSERT(valToTest == -1);
	BT_ASSERT(tperrno != 0);
	BT_ASSERT(tperrno != TPEBADDESC);
	BT_ASSERT(tperrno != TPEOTYPE);
	BT_ASSERT(tperrno != TPETIME);
	BT_ASSERT(tperrno != TPESVCFAIL);
	BT_ASSERT(tperrno != TPESVCERR);
	BT_ASSERT(tperrno != TPEBLOCK);
	BT_ASSERT(tperrno == TPEINVAL);
}

void TestTPGetRply::test_tpgetrply_nullrcvbuf() {
	userlogc((char*) "test_tpgetrply_nullrcvbuf");

	int toCheck = tpadvertise((char*) "TestTPGetrply", testtpgetrply_service);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(toCheck != -1);

	int cd = ::tpacall((char*) "TestTPGetrply", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd != -1);
	BT_ASSERT(tperrno == 0);

	int valToTest = ::tpgetrply(&cd, NULL, &rcvlen, 0);
	BT_ASSERT(valToTest == -1);
	BT_ASSERT(tperrno != 0);
	BT_ASSERT(tperrno != TPEOTYPE);
	BT_ASSERT(tperrno != TPETIME);
	BT_ASSERT(tperrno != TPESVCFAIL);
	BT_ASSERT(tperrno != TPESVCERR);
	BT_ASSERT(tperrno != TPEBLOCK);
	BT_ASSERT(tperrno != TPEBADDESC);
	BT_ASSERT(tperrno == TPEINVAL);
}

void TestTPGetRply::test_tpgetrply_nullrcvlen() {
	userlogc((char*) "test_tpgetrply_nullrcvlen");

	int toCheck = tpadvertise((char*) "TestTPGetrply", testtpgetrply_service);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(toCheck != -1);

	int cd = 2;
	int valToTest = ::tpgetrply(&cd, (char **) &rcvbuf, NULL, 0);
	BT_ASSERT(valToTest == -1);
	BT_ASSERT(tperrno != 0);
	BT_ASSERT(tperrno != TPEBADDESC);
	BT_ASSERT(tperrno != TPEOTYPE);
	BT_ASSERT(tperrno != TPETIME);
	BT_ASSERT(tperrno != TPESVCFAIL);
	BT_ASSERT(tperrno != TPESVCERR);
	BT_ASSERT(tperrno != TPEBLOCK);
	BT_ASSERT(tperrno == TPEINVAL);
}

void TestTPGetRply::test_tpgetrply_with_TPNOBLOCK() {
	userlogc((char*) "test_tpgetrply_with_TPNOBLOCK");
	tpadvertise((char*) "TestTPGetrply", test_tpgetrply_TPNOBLOCK);

	int cd = ::tpacall((char*) "TestTPGetrply", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd != -1);
	BT_ASSERT(tperrno == 0);

	// RETRIEVE THE RESPONSE
	int valToTest = ::tpgetrply(&cd, (char **) &rcvbuf, &rcvlen, TPNOBLOCK);
	char* tperrnoS = (char*) malloc(110);
	sprintf(tperrnoS, "%d", tperrno);
	BT_ASSERT_MESSAGE(tperrnoS, tperrno == 0);
	free(tperrnoS);
	BT_ASSERT(valToTest == -1);
	BT_ASSERT(strcmp(rcvbuf, "test_tpgetrply_TPNOBLOCK") == -1);
}

void TestTPGetRply::test_tpgetrply_without_TPNOBLOCK() {
	userlogc((char*) "test_tpgetrply_without_TPNOBLOCK");
	tpadvertise((char*) "TestTPGetrply", test_tpgetrply_TPNOBLOCK);

	int cd = ::tpacall((char*) "TestTPGetrply", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd != -1);
	BT_ASSERT(tperrno == 0);

	// RETRIEVE THE RESPONSE
	int toTest = ::tpgetrply(&cd, (char **) &rcvbuf, &rcvlen, 0);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT_MESSAGE(rcvbuf, strcmp(rcvbuf, "test_tpgetrply_TPNOBLOCK") == 0);
	char* toTestS = (char*) malloc(110);
	sprintf(toTestS, "%d", toTest);
	BT_ASSERT_MESSAGE(toTestS, toTest == 0);
	free(toTestS);
}

void TestTPGetRply::test_tpgetrply_with_TPGETANY() {
	userlogc((char*) "test_tpgetrply_with_TPGETANY");
	testingTPGETANY = true;
	tpadvertise((char*) "DEBIT", test_tpgetrply_TPGETANY_one);
	tpadvertise((char*) "CREDIT", test_tpgetrply_TPGETANY_two);

	int cd1 = ::tpacall((char*) "DEBIT", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd1 != -1);
	BT_ASSERT(tperrno == 0);

	int cd2 = ::tpacall((char*) "CREDIT", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd2 != -1);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(cd1 != cd2);

	// RETRIEVE THE RESPONSE
	int cdToGet = cd1;
	int toTest = ::tpgetrply(&cdToGet, (char **) &rcvbuf, &rcvlen, TPGETANY);
	BT_ASSERT(tperrno == 0);
	char* toTestS = (char*) malloc(110);
	sprintf(toTestS, "%d", toTest);
	BT_ASSERT_MESSAGE(toTestS, toTest == 0);
	free(toTestS);
	char* gotCdS = (char*) malloc(110);
	sprintf(gotCdS, "got %d expected %d", cdToGet, cd2);
	BT_ASSERT_MESSAGE(gotCdS, cdToGet == cd2);
	free(gotCdS);
	BT_ASSERT_MESSAGE(rcvbuf, strcmp(rcvbuf, "test_tpgetrply_TPGETANY_two")
			== 0);
}

void TestTPGetRply::test_tpgetrply_without_TPGETANY() {
	userlogc((char*) "test_tpgetrply_without_TPGETANY");
	testingTPGETANY = true;
	tpadvertise((char*) "DEBIT", test_tpgetrply_TPGETANY_one);
	tpadvertise((char*) "CREDIT", test_tpgetrply_TPGETANY_two);

	int cd1 = ::tpacall((char*) "DEBIT", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd1 != -1);
	BT_ASSERT(tperrno == 0);

	int cd2 = ::tpacall((char*) "CREDIT", (char *) sendbuf, sendlen, 0);
	BT_ASSERT(cd2 != -1);
	BT_ASSERT(tperrno == 0);
	BT_ASSERT(cd1 != cd2);

	// RETRIEVE THE RESPONSE
	int cdToGet = cd1;
	int toTest = ::tpgetrply(&cdToGet, (char **) &rcvbuf, &rcvlen, 0);
	BT_ASSERT(tperrno == 0);
	char* toTestS = (char*) malloc(110);
	sprintf(toTestS, "%d", toTest);
	BT_ASSERT_MESSAGE(toTestS, toTest == 0);
	free(toTestS);
	char* gotCdS = (char*) malloc(110);
	sprintf(gotCdS, "%d", cdToGet);
	BT_ASSERT_MESSAGE(gotCdS, cdToGet == cd1);
	free(gotCdS);
	BT_ASSERT_MESSAGE(rcvbuf, strcmp(rcvbuf, "test_tpgetrply_TPGETANY_one")
			== 0);
}

void testtpgetrply_service(TPSVCINFO *svcinfo) {
	userlogc((char*) "testtpgetrply_service");
	char * toReturn = ::tpalloc((char*) "X_OCTET", NULL, 22);
	strcpy(toReturn, "testtpgetrply_service");
	tpreturn(TPSUCCESS, 0, toReturn, 22, 0);
}

void test_tpgetrply_TPNOBLOCK(TPSVCINFO *svcinfo) {
	char* response = (char*) "test_tpgetrply_TPNOBLOCK";
	userlogc(response);

	long sendlen = strlen(response) + 1;
	char * toReturn = ::tpalloc((char*) "X_OCTET", NULL, sendlen);
	strcpy(toReturn, response);
	::sleeper(5);
	tpreturn(TPSUCCESS, 0, toReturn, sendlen, 0);
}

void test_tpgetrply_TPGETANY_one(TPSVCINFO *svcinfo) {
	char* response = (char*) "test_tpgetrply_TPGETANY_one";
	userlogc(response);

	long sendlen = strlen(response) + 1;
	char * toReturn = ::tpalloc((char*) "X_OCTET", NULL, sendlen);
	strcpy(toReturn, response);
	::sleeper(10);
	tpreturn(TPSUCCESS, 0, toReturn, sendlen, 0);
}

void test_tpgetrply_TPGETANY_two(TPSVCINFO *svcinfo) {
	char* response = (char*) "test_tpgetrply_TPGETANY_two";
	userlogc(response);

	long sendlen = strlen(response) + 1;
	char * toReturn = ::tpalloc((char*) "X_OCTET", NULL, sendlen);
	strcpy(toReturn, response);
	::sleeper(5);
	tpreturn(TPSUCCESS, 0, toReturn, sendlen, 0);
}
