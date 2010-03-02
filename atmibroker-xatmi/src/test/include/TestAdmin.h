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
#ifndef TEST_ADMIN_H
#define TEST_ADMIN_H

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "BaseAdminTest.h"

class TestAdmin: public BaseAdminTest {
	CPPUNIT_TEST_SUITE( TestAdmin );
	//CPPUNIT_TEST( testServerdone );
	//CPPUNIT_TEST( testStatus );
	//CPPUNIT_TEST( testMessageCounter );
	CPPUNIT_TEST( testErrorCounter );
	//CPPUNIT_TEST( testServerPauseAndResume );
	CPPUNIT_TEST_SUITE_END();

public:
	void testServerdone();
	void testStatus();
	void testMessageCounter();
	void testErrorCounter();
	void testServerPauseAndResume();
	char* getBARCounter(char*);
	virtual void setUp();
	virtual void tearDown();
};

#endif
