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
#include "XAResourceManager.h"
#include "ThreadLocalStorage.h"

static int compareXids(XID * xid1, XID * xid2)
{
	if (xid1 == xid2)
		return 0;

	if ((xid1->formatID == xid2->formatID)
		&& (xid1->gtrid_length == xid2->gtrid_length)
		&& (xid1->bqual_length == xid2->bqual_length))
	{
		for (int i = 0; i < (xid1->gtrid_length + xid1->bqual_length); i++)
			if (xid1->data[i] != xid2->data[i])
				return -1;

		return 0;	// XIDs are equal
	} else {
		return 1;
	}
}

XAResourceManager::XAResourceManager(
        CORBA_CONNECTION* connection,
        const char * name,
        const char * openString,
        const char * closeString,
        CORBA::Long rmid,
        struct xa_switch_t * xa_switch) throw (RMException) :

        poa_(NULL), connection_(connection), name_(name), openString_(openString), closeString_(closeString),
		rmid_(rmid), xa_switch_(xa_switch) {

        if (name == NULL) {
                RMException ex("Invalid RM name", EINVAL);
                throw ex;
        }

        int rv = xa_switch_->xa_open_entry((char *) openString, rmid, TMNOFLAGS);

        if (rv != XA_OK) {
                RMException ex("xa_open", rv);
                throw ex;
        }

	// each RM has its own POA
        createPOA();
}

XAResourceManager::~XAResourceManager() {
	int rv = xa_switch_->xa_close_entry((char *) closeString_, rmid_, TMNOFLAGS);

	if (rv != XA_OK)
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getWarn(),
			(char *) " close RM " << name_ << " failed: " << rv);

        if (!CORBA::is_nil(poa_))
                poa_ = NULL;
}

void XAResourceManager::createPOA() {
        //CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
        //PortableServer::POA_var rpoa = PortableServer::POA::_narrow(obj);

        PortableServer::POAManager_ptr poa_manager = (PortableServer::POAManager_ptr) connection_->root_poa_manager;
        PortableServer::POA_ptr parent_poa = (PortableServer::POA_ptr) connection_->root_poa;
	PortableServer::LifespanPolicy_var p1 = parent_poa->create_lifespan_policy(PortableServer::PERSISTENT);

        CORBA::PolicyList policies;
        policies.length(1);

	// the servant object references must survive failure of the ORB in order to support recover of 
	// transaction branches (the default orb policy for servants is transient)
        policies[0] = PortableServer::LifespanPolicy::_duplicate(p1);

	// create a new POA for this RM
	try {
		ACE_TCHAR name[32];
		ACE_OS::sprintf(name, ACE_TEXT("%s%02d"), "ATMI_RM_" + rmid_);

        	this->poa_ = parent_poa->create_POA(name, poa_manager, policies);
	} catch (PortableServer::POA::AdapterAlreadyExists &) {
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getWarn(), (char *) "Duplicate RM POA");
                RMException ex("Duplicate RM POA", EINVAL);
                throw ex;

	} catch (PortableServer::POA::InvalidPolicy &) {
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getWarn(), (char *) "Invalid RM POA policy");
                RMException ex("Invalid RM POA policy", EINVAL);
                throw ex;
	}

	policies[0]->destroy();

	// take the POA out of its holding state
	PortableServer::POAManager_var mgr = this->poa_->the_POAManager();
	mgr->activate();
}

int XAResourceManager::createServant(XID * xid)
{
	XAResourceAdaptorImpl * ra;
	CosTransactions::Control_ptr curr = (CosTransactions::Control_ptr) getSpecific(TSS_KEY);
	if (CORBA::is_nil(curr))
		return XAER_NOTA;

	CosTransactions::Coordinator_ptr c = curr->get_coordinator();

	// create a servant to represent the new branch identified by xid
	try {
		ra = new XAResourceAdaptorImpl(this, xid, rmid_, xa_switch_);
	} catch (RMException& ex) {
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getWarn(),
			(char*) "unable to create resource adaptor for transaction branch: " << ex.what());

		return XAER_RMFAIL;
	}
	// and activate it
	PortableServer::ObjectId_var objId = poa_->activate_object(ra);

	try {
		// get a CORBA reference to the servant so that it can be enlisted in the OTS transaction
        	CORBA::Object_ptr ref = poa_->servant_to_reference(ra);
		CosTransactions::Resource_var v = CosTransactions::Resource::_narrow(ref);

		// enlist it with the transaction
		CosTransactions::RecoveryCoordinator_ptr rc = c->register_resource(v);
		if (CORBA::is_nil(rc)) {
			LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getTrace(),
				(char*) "createServant: nill RecoveryCoordinator ");
		} else {
			ra->setRecoveryCoordinator(rc);
        		branches_[xid] = ra;

			return XA_OK;
		}
	} catch (PortableServer::POA::ServantNotActive&) {
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getError(),
			(char*) "createServant: poa inactive");
	} catch (CosTransactions::Inactive&) {
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getTrace(),
			(char*) "createServant: tx inactive (too late for registration)");
	} catch (const CORBA::SystemException& ex) {
		ex._tao_print_exception("Resource registration error: ");
		LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getTrace(),
			(char*) "createServant: unexpected error");
	}

	delete ra;

	return XAER_NOTA;
}

void XAResourceManager::setComplete(XID * xid)
{
	XABranchMap::iterator iter;

	for (XABranchMap::iterator i = branches_.begin(); i != branches_.end(); ++i)
	{
		if (compareXids(i->first, xid) == 0) {
			XAResourceAdaptorImpl * r = i->second;

			LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getTrace(),
				(char*) "RM removing branch");
			branches_.erase(i);
			delete r;

			return;
		}
	}

	LOG4CXX_LOGLS(xaResourceLogger, log4cxx::Level::getTrace(), (char*) "RM cannot remove unknown branch");
}

int XAResourceManager::xa_start (XID * xid, int rmid, long flags)
{
	XAResourceAdaptorImpl * resource = locateBranch(xid);
	int rv;

	if (resource == NULL) {
		if ((rv = createServant(xid)) != XA_OK)
			return rv;

		if ((resource = locateBranch(xid)) == NULL)	// cannot be NULL
			return XAER_RMERR;

        	return resource->xa_start(xid, rmid, TMNOFLAGS);
	}

        if (flags | (TMRESUME & TMJOIN))
		return  resource->xa_start(xid, rmid, flags);
	else
		return  resource->xa_start(xid, rmid, TMJOIN);
}

int XAResourceManager::xa_end (XID * xid, int rmid, long flags)
{
	XAResourceAdaptorImpl * resource = locateBranch(xid);

        return resource ? resource->xa_end(xid, rmid, flags) : XAER_NOTA;
}

XAResourceAdaptorImpl * XAResourceManager::locateBranch(XID * xid)
{
	XABranchMap::iterator iter;

	for (iter = branches_.begin(); iter != branches_.end(); ++iter)
		if (compareXids((*iter).first, xid) == 0)
			return (*iter).second;

	return NULL;
}
