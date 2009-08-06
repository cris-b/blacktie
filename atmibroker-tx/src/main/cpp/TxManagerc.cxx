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

#include "TxManager.h"
#include "txClient.h"
#include "OrbManagement.h"

using namespace atmibroker::tx;

/* X/Open tx interface */

int tx_open(void) {
	return TxManager::get_instance()->open();
}

int tx_begin(void) {
	return TxManager::get_instance()->begin();
}

int tx_commit(void) {
	return TxManager::get_instance()->commit();
}

int tx_rollback(void) {
	return TxManager::get_instance()->rollback();
}

int tx_close(void) {
	return TxManager::get_instance()->close();
}

int tx_set_commit_return(COMMIT_RETURN when_return) {
	return TxManager::get_instance()->set_commit_return(when_return);
}
int tx_set_transaction_control(TRANSACTION_CONTROL control) {
	return TxManager::get_instance()->set_transaction_control(control);
}
int tx_set_transaction_timeout(TRANSACTION_TIMEOUT timeout) {
	return TxManager::get_instance()->set_transaction_timeout(timeout);
}
int tx_info(TXINFO *info) {
	return TxManager::get_instance()->info(info);
}

/* Blacktie tx interface additions */
int set_rollback_only()
{
    return TxManager::get_instance()->rollback_only();
}

void * start_tx_orb(char* connectionName)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "TxManagerc: start_tx_orb");
    return TxManager::init_orb(connectionName);
}

void shutdown_tx_broker(void)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "TxManagerc: shutdown_tx_broker");
    TxManager::discard_instance();
}

int associate_tx(void *control)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "TxManagerc: associate_tx");
    return atmibroker::tx::TxManager::tx_resume((CosTransactions::Control_ptr) control, TMRESUME);
}

int associate_tx(void *control, int tid)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "TxManagerc: associate_tx: tid=" << tid);
    return atmibroker::tx::TxManager::tx_resume((CosTransactions::Control_ptr) control, tid, TMRESUME);
}

int associate_serialized_tx(char *orbname, char* ctrlIOR)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "associate_serialized_tx orb=" << orbname);
    return atmibroker::tx::TxManager::tx_resume(ctrlIOR, orbname, TMRESUME);
}

void * disassociate_tx(void)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "disassociate_tx");
    return (void *) atmibroker::tx::TxManager::tx_suspend(TMSUSPEND | TMMIGRATE);
}

void * disassociate_tx_if_not_owner(void)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "disassociate_tx_if_not_owner");
    return (void *) atmibroker::tx::TxManager::tx_suspend(ACE_OS::thr_self(), TMSUSPEND | TMMIGRATE);
}

void * get_control()
{
    return (void *) atmibroker::tx::TxManager::get_ots_control();
}

void release_control(void *control)
{
    CosTransactions::Control_ptr cp = (CosTransactions::Control_ptr) control;

	try {
		if (!CORBA::is_nil(cp))
			CORBA::release(cp);
	} catch (...) {
	}
}

char* serialize_tx(char *orbname)
{
	LOG4CXX_LOGLS(txlogger, log4cxx::Level::getTrace(), (char*) "serialize_tx orb=" << orbname);
    CORBA::ORB_ptr orb = find_orb(orbname);
    CosTransactions::Control_ptr ctrl = atmibroker::tx::TxManager::get_ots_control();

    if (!CORBA::is_nil(orb) && !CORBA::is_nil(ctrl))
        return ACE_OS::strdup(orb->object_to_string(ctrl));

    return NULL;
}