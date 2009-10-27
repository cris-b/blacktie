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
#include "OrbManagement.h"

using namespace atmibroker::tx;

log4cxx::LoggerPtr txmclogger(log4cxx::Logger::getLogger("TxLogManagerc"));

/* Blacktie tx interface additions */
int txx_rollback_only()
{
	FTRACE(txmclogger, "ENTER");
	return TxManager::get_instance()->rollback_only();
}

void txx_stop(void)
{
	FTRACE(txmclogger, "ENTER");
	TxManager::discard_instance();
	FTRACE(txmclogger, "<");
}

int txx_associate_serialized(char *orbname, char* ctrlIOR)
{
	FTRACE(txmclogger, "ENTER" << orbname);
	CORBA::Object_ptr p = atmi_string_to_object(ctrlIOR, orbname);

	LOG4CXX_DEBUG(txmclogger, (char*) "tx_resume orb=" << orbname << (char *) " IOR=" << ctrlIOR << " ptr=" << p);

	if (!CORBA::is_nil(p)) {
		CosTransactions::Control_ptr cptr = CosTransactions::Control::_narrow(p);
		CORBA::release(p); // dispose of it now that we have narrowed the object reference

		return TxManager::get_instance()->tx_resume(cptr, TMJOIN);
	} else {
		LOG4CXX_WARN(txmclogger, (char*) "tx_resume: invalid control IOR: " << ctrlIOR);
	}

	return TMER_INVAL;
}

void *txx_unbind(bool rollback)
{
	FTRACE(txmclogger, "ENTER rollback=" << rollback);
	if (rollback)
		(void) TxManager::get_instance()->rollback_only();

	return (void *) TxManager::get_instance()->tx_suspend((TMSUSPEND | TMMIGRATE));
//	return (void *) TxManager::get_instance()->tx_suspend(TMSUCCESS);
}

void *txx_get_control()
{
	FTRACE(txmclogger, "ENTER");
	void *ctrl = (void *) TxManager::get_ots_control();
	FTRACE(txmclogger, "< with control " << ctrl);
	return ctrl;
}

void txx_release_control(void *control)
{
	FTRACE(txmclogger, "ENTER");
	CosTransactions::Control_ptr cp = (CosTransactions::Control_ptr) control;

	try {
		if (!CORBA::is_nil(cp))
			CORBA::release(cp);
		else
			FTRACE(txmclogger, "< nothing to release");
	} catch (...) {
	}
}

char* txx_serialize(char *orbname)
{
	FTRACE(txmclogger, "ENTER " << orbname);
	CORBA::ORB_ptr orb = find_orb(orbname);
	CosTransactions::Control_ptr ctrl = TxManager::get_ots_control();

	if (!CORBA::is_nil(orb) && !CORBA::is_nil(ctrl)) {
		CORBA::String_var cs = orb->object_to_string(ctrl);
		char *cstr = ACE_OS::strdup(cs);
		CORBA::release(ctrl);

		return cstr;
	}

	FTRACE(txmclogger, "< No tx ior");
	return NULL;
}

int txx_suspend(int cd)
{
	FTRACE(txmclogger, "ENTER");
	return TxManager::get_instance()->suspend(cd);
}

int txx_resume(int cd)
{
	FTRACE(txmclogger, "ENTER");
	return TxManager::get_instance()->resume(cd);
}

bool txx_isCdTransactional(int cd)
{
	return TxManager::get_instance()->isCdTransactional(cd);
}
