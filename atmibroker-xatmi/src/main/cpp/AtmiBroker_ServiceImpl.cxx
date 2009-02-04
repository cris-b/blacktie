/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat Middleware LLC, and others contributors as indicated
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
/*
 * BREAKTHRUIT PROPRIETARY - NOT TO BE DISCLOSED OUTSIDE BREAKTHRUIT, LLC.
 */
// copyright 2006, 2008 BreakThruIT

#include "AtmiBroker_ServiceImpl.h"

#include "SenderImpl.h"
#include "AtmiBrokerServer.h"
#include "EndpointQueue.h"
#include "xatmi.h"
#include "userlog.h"
#include "ThreadLocalStorage.h"

#include "log4cxx/logger.h"
using namespace log4cxx;
using namespace log4cxx::helpers;
LoggerPtr loggerAtmiBroker_ServiceImpl(Logger::getLogger("AtmiBroker_ServiceImpl"));

// AtmiBroker_ServiceImpl constructor
//
AtmiBroker_ServiceImpl::AtmiBroker_ServiceImpl(char *serviceName, void(*func)(TPSVCINFO *)) :
	m_serviceName(serviceName), m_func(func), m_buffer(NULL) {

	EndpointQueue* endpointQueue = new EndpointQueue(server_callback_poa);
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "tmp_servant %p", (void*) endpointQueue);
	server_callback_poa->activate_object(endpointQueue);
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "activated tmp_servant %p", endpointQueue);
	CORBA::Object_ptr tmp_ref = server_callback_poa->servant_to_reference(endpointQueue);
	AtmiBroker::EndpointQueue_var queue = AtmiBroker::EndpointQueue::_narrow(tmp_ref);
	endpointQueue->setReplyTo(server_orb->object_to_string(queue));
	queueReceiver = endpointQueue;
}

// ~AtmiBroker_ServiceImpl destructor.
//
AtmiBroker_ServiceImpl::~AtmiBroker_ServiceImpl() {
	// Intentionally empty.
	//
}

void AtmiBroker_ServiceImpl::onMessage(MESSAGE message) {
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "svc()");
	m_buffer = message.data;
	queueSender = new SenderImpl(server_orb, (char*) message.replyto);
	char* idata = message.data;
	long ilen = message.len;
	long flags = message.flags;
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "   idata = %p", idata);
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "   ilen = %d", ilen);
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "   flags = %d", flags);

	TPSVCINFO tpsvcinfo;
	memset(&tpsvcinfo, '\0', sizeof(tpsvcinfo));
	strcpy(tpsvcinfo.name, m_serviceName);
	tpsvcinfo.flags = flags;
	tpsvcinfo.data = idata;
	tpsvcinfo.len = ilen;

	setSpecific(SVC_KEY, this);
	tx_open();
	m_func(&tpsvcinfo);
	tx_close();
	destroySpecific(SVC_KEY);
}

// tpreturn()
//
void AtmiBroker_ServiceImpl::tpreturn(int rval, long rcode, char* data, long len, long flags) {
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "tpreturn()");
	MESSAGE message;
	message.rval = rval;
	message.rcode = rcode;
	message.data = data;
	message.len = len;
	message.flags = flags;
	getSender()->send(message);
	userlog(Level::getDebug(), loggerAtmiBroker_ServiceImpl, (char*) "Calling back ");
}

bool AtmiBroker_ServiceImpl::sameBuffer(char* toCheck) {
	bool toReturn = false;
	if (!toCheck || toCheck == NULL) {
		tperrno = TPEINVAL; // TO MESSAGE TO CHECK
	} else if (m_buffer == NULL) {
		tperrno = TPESVCERR; // NO INBOUND MESSAGE
	} else if (toCheck == m_buffer) {
		toReturn = true;
	}
	return toReturn;
}

Sender* AtmiBroker_ServiceImpl::getSender() {
	return queueSender;
}

Receiver* AtmiBroker_ServiceImpl::getReceiver() {
	return queueReceiver;
}

void AtmiBroker_ServiceImpl::setReplyTo(char * replyTo) {

}

void AtmiBroker_ServiceImpl::getId(int& id) {

}
