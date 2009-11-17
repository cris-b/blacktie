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
#include <cppunit/extensions/HelperMacros.h>

#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "xatmi.h"
#include "userlogc.h"
#include "TestAdvertise.h"

void TestAdvertise::setUp() {
	userlogc((char*) "TestAdvertise::setUp");
	BaseServerTest::setUp();
}

void TestAdvertise::tearDown() {
	userlogc((char*) "TestAdvertise::tearDown");
	BaseServerTest::tearDown();
}

int TestAdvertise::callBAR() {
	long  sendlen = strlen((char*)"test") + 1;
	char* sendbuf = tpalloc((char*) "X_OCTET", NULL, sendlen);
	strcpy(sendbuf, (char*) "test");

	int cd = ::tpacall((char*) "BAR", (char *) sendbuf, sendlen, TPNOREPLY);
	return cd;
}

int TestAdvertise::calladmin(char* command, char expect) {
	long  sendlen = strlen(command) + 1;
	char* sendbuf = tpalloc((char*) "X_OCTET", NULL, sendlen);
	strcpy(sendbuf, command);

	char* recvbuf = tpalloc((char*) "X_OCTET", NULL, 1);
	long  recvlen = 1;

	int cd = ::tpcall((char*) "default_ADMIN_1", (char *) sendbuf, sendlen, (char**)&recvbuf, &recvlen, TPNOTRAN);
	CPPUNIT_ASSERT(recvlen == 1);
	CPPUNIT_ASSERT(recvbuf[0] == expect);

	return cd;
}

void TestAdvertise::testService() {
	int cd;

	cd = calladmin((char*)"advertise,BAR,", '1');
	CPPUNIT_ASSERT(cd == 0);
	CPPUNIT_ASSERT(tperrno == 0);
}

void TestAdvertise::testUnknowService() {
	int   cd;

	cd = calladmin((char*)"advertise,UNKNOW,", '0');
	CPPUNIT_ASSERT(cd == 0);
	CPPUNIT_ASSERT(tperrno == 0);
}

void TestAdvertise::testAdvertise() {
	int   cd;

	userlogc((char*) "unadvertise BAR");
	cd = calladmin((char*)"unadvertise,BAR,", '1');
	CPPUNIT_ASSERT(cd == 0);
	CPPUNIT_ASSERT(tperrno == 0);

	userlogc((char*) "tpacall BAR after unadvertise");
	cd = callBAR();
	CPPUNIT_ASSERT(cd != 0);
	CPPUNIT_ASSERT(tperrno != 0);

	userlogc((char*) "advertise BAR again");
	cd = calladmin((char*)"advertise,BAR,", '1');
	CPPUNIT_ASSERT(cd == 0);
	CPPUNIT_ASSERT(tperrno == 0);

	userlogc((char*) "tpacall BAR after advertise");
	cd = callBAR();
	CPPUNIT_ASSERT(cd == 0);
	CPPUNIT_ASSERT(tperrno == 0);
}
