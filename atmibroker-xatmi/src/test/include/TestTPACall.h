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
#ifndef TESTTPACALL_H
#define TESTTPACALL_H

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "BaseServerTest.h"

class TestTPACall: public BaseServerTest {
	CPPUNIT_TEST_SUITE( TestTPACall);
	CPPUNIT_TEST( test_tpacall);
	//TODO READD CPPUNIT_TEST( test_tpacall_systemerr);
	CPPUNIT_TEST( test_tpacall_x_octet);
CPPUNIT_TEST_SUITE_END();

public:
void test_tpacall();
void test_tpacall_systemerr();
void test_tpacall_x_octet();
virtual void setUp();
virtual void tearDown();

private:
char *sendbuf, *rcvbuf;
long sendlen, rcvlen;

};

#endif // TESTTPACALL_H
