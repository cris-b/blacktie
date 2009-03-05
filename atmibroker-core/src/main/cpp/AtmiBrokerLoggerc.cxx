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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>

#include "log4cxx/logger.h"
#include "log4cxx/basicconfigurator.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/logmanager.h"

#include "userlog.h"
extern "C" {
#include "userlogc.h"
}
#include "AtmiBrokerEnv.h"

log4cxx::LoggerPtr loggerAtmiBrokerLogc(log4cxx::Logger::getLogger("AtmiBrokerLogc"));

bool loggerInitialized;

extern "C"ATMIBROKER_CORE_DLL
void userlogc(const char * format, ...) {
	if (loggerAtmiBrokerLogc->isEnabledFor(log4cxx::Level::getInfo())) {
		char str[2048];
		va_list args;
		va_start(args, format);
		vsprintf(str, format, args);
		va_end(args);
		LOG4CXX_LOGLS(loggerAtmiBrokerLogc, log4cxx::Level::getInfo(), str);
	}
}

extern void initializeLogger() {
	if (!loggerInitialized) {
		if (AtmiBrokerEnv::get_instance()->getenv((char*) "LOG4CXXCONFIG") != NULL) {
			log4cxx::PropertyConfigurator::configure(AtmiBrokerEnv::get_instance()->getenv((char*) "LOG4CXXCONFIG"));
		} else {
			log4cxx::BasicConfigurator::configure();
		}
		loggerInitialized = true;
	}
}

void userlog(const log4cxx::LevelPtr& level, log4cxx::LoggerPtr& logger, const char * format, ...) {
	if (logger->isEnabledFor(level)) {
		char str[2048];
		va_list args;
		va_start(args, format);
		vsprintf(str, format, args);
		va_end(args);
		LOG4CXX_LOGLS(logger, level, str);
	}
}
