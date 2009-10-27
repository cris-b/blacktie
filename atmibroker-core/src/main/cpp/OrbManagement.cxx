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
#include <string.h>
#include <tao/ORB.h>
#include <tao/Object.h>
//includes for looking up orbs
#include "tao/ORB_Core.h"
#include "tao/ORB_Table.h"
#include "tao/ORB_Core_Auto_Ptr.h"

#include <orbsvcs/CosNamingS.h>
#include "OrbManagement.h"
#include "AtmiBrokerEnv.h"
#include "AtmiBrokerEnvXml.h"
#include "log4cxx/logger.h"
#include "Worker.h"
#include "AtmiBrokerPoaFac.h"

log4cxx::LoggerPtr loggerOrbManagement(log4cxx::Logger::getLogger(
		"OrbManagment"));

extern BLACKTIE_CORE_DLL CORBA::ORB_ptr find_other_orb(const char *);
extern BLACKTIE_CORE_DLL void print_orb_ids();

CORBA_CONNECTION* initOrb(char* connectionName) {
	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "initOrb" << connectionName);

	CORBA_CONNECTION* connection = new CORBA_CONNECTION;
	connection->connectionName = connectionName;
	connection->orbRef = NULL;
	connection->root_poa = NULL;
	connection->root_poa_manager = NULL;
	connection->default_ctx = NULL;
	connection->name_ctx = NULL;
	connection->callback_poa = NULL;
	connection->worker = NULL;
	connection->poaFactory = NULL;

	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "initOrb initing ORB");
	AtmiBrokerEnv::get_instance();
	std::string values = orbConfig.opt;
	LOG4CXX_TRACE(loggerOrbManagement, (char*) "initOrb OPT: " << values);
	char * cstr, *p;
	cstr = new char[values.size() + 1];
	strcpy(cstr, values.c_str());
	char** vals = (char**) malloc(sizeof(char) * values.size() * 2);
	p = strtok(cstr, " ");
	int i = 0;
	while (p != NULL) {
		vals[i] = strdup(p);
		p = strtok(NULL, " ");
		i++;
	}

	int n = i;
	//when ORB_init return, i is 0
	connection->orbRef = CORBA::ORB_init(i, vals, connectionName);

	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "Created orb: "
			<< connection->orbRef);
	int j;
	for (j = 0; j < n; j++) {
		free(vals[j]);
	}
	free(vals);
	delete[] cstr;

	LOG4CXX_DEBUG(loggerOrbManagement,
			(char*) "getNamingServiceAndContext getting Naming Service Ext");
	{
		CORBA::Object_var tmp_ref =
				connection->orbRef->resolve_initial_references("NameService");
		LOG4CXX_DEBUG(
				loggerOrbManagement,
				(char*) "getNamingServiceAndContext got orbRef->resolve_initial_references, tmp_ref = %p"
						<< (void*) tmp_ref);
		connection->default_ctx = CosNaming::NamingContextExt::_narrow(tmp_ref);
	}
	LOG4CXX_DEBUG(
			loggerOrbManagement,
			(char*) "getNamingServiceAndContext narrowed tmp_ref, default_context = "
					<< connection->default_ctx);

	try {
		LOG4CXX_DEBUG(loggerOrbManagement,
				(char*) "getNamingServiceAndContext domain is " << domain);
		{
			CORBA::Object_var tmp_ref = connection->default_ctx->resolve_str(
					domain);
			connection->name_ctx = CosNaming::NamingContext::_narrow(tmp_ref);
			LOG4CXX_DEBUG(
					loggerOrbManagement,
					(char*) "getNamingServiceAndContext found domain naming context");
		}
	} catch (const CosNaming::NamingContext::NotFound&) {
		LOG4CXX_DEBUG(loggerOrbManagement,
				(char*) "Find failed, trying to create");
		CosNaming::Name_var name = connection->default_ctx->to_name(domain);
		LOG4CXX_DEBUG(loggerOrbManagement, (char*) "Got name: " << domain);
		connection->name_ctx = connection->default_ctx->bind_new_context(name);
		LOG4CXX_DEBUG(
				loggerOrbManagement,
				(char*) "getNamingServiceAndContext created domain naming context");
	} catch (const CORBA::SystemException& ex) {
		LOG4CXX_ERROR(loggerOrbManagement, (char*) "Got unexpected : "
				<< ex._name());
		throw ;
	}

	LOG4CXX_DEBUG(loggerOrbManagement,
			(char*) "getNamingServiceAndContext got Naming Service Instance ");

	connection->worker = new Worker(connection->orbRef, connectionName);
	if (((Worker*) connection->worker)->activate(THR_NEW_LWP | THR_JOINABLE, 1,
					0, ACE_DEFAULT_THREAD_PRIORITY, -1, 0, 0, 0, 0, 0, 0) != 0) {
		delete ((Worker*) connection->worker);
		connection->worker = NULL;
		LOG4CXX_ERROR(loggerOrbManagement,
				(char*) "Could not start thread pool");
	}

	//try {
	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "resolving the root POA");
	CORBA::Object_var tmp_ref = connection->orbRef->resolve_initial_references("RootPOA");
	connection->root_poa = PortableServer::POA::_narrow(tmp_ref);
	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "resolved the root POA: "
			<< connection->root_poa);

	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "getting the root POA manager");
	connection->root_poa_manager = connection->root_poa->the_POAManager();
	LOG4CXX_DEBUG(loggerOrbManagement,
			(char*) "getRootPOAAndManager got the root POA manager: "
			<< connection->root_poa_manager);

	LOG4CXX_DEBUG(loggerOrbManagement, (char*) "creating POA with name client");

	AtmiBrokerPoaFac* poaFac = new AtmiBrokerPoaFac();
	connection->callback_poa = poaFac->createCallbackPoa(connection->orbRef,
			connectionName, connection->root_poa, connection->root_poa_manager);
	connection->poaFactory = poaFac;
	LOG4CXX_DEBUG(loggerOrbManagement,
			(char*) "createClientCallbackPOA created POA: "
			<< connection->callback_poa);

	connection->root_poa_manager->activate();
	LOG4CXX_DEBUG(loggerOrbManagement,
			(char*) "activated poa - started processing requests");

	//} catch (CORBA::SystemException & e) {
	//	LOG4CXX_LOGLS(loggerOrbManagement, log4cxx::Level::getWarn(),
	//                (char*) "initOrb error: CORBA SystemException name: "
	//                         << e._name() << (char *)" minor code: " << e.minor());
	//	e._tao_print_exception("initOrb error:");
	//}


	return connection;
}

void shutdownBindings(CORBA_CONNECTION* connection) {
	LOG4CXX_DEBUG(loggerOrbManagement, "Closing Bindings");

	if (connection) {
		if (connection->orbRef) {
			if (!CORBA::is_nil(connection->orbRef)) {
				LOG4CXX_DEBUG(loggerOrbManagement,
						"shutdownBindings shutting down ORB: "
								<< connection->orbRef);
				try {
					connection->orbRef->shutdown(1);
					LOG4CXX_DEBUG(loggerOrbManagement,
							"shutdownBindings shut down ORB");
				} catch (CORBA::Exception &ex) {
					LOG4CXX_ERROR(
							loggerOrbManagement,
							(char*) "shutdownBindings Unexpected CORBA exception shutting down orb: "
									<< ex._name());
				} catch (...) {
					LOG4CXX_FATAL(
							loggerOrbManagement,
							(char*) "shutdownBindings Unexpected fatal exception");
				}

				if (connection->worker != NULL) {
					//					ACE_Thread_Manager::instance()->wait_task(((Worker*) connection->worker));
					((Worker*) connection->worker)->wait();
					delete ((Worker*) connection->worker);
					connection->worker = NULL;
				}

				if (connection->poaFactory != NULL) {
					delete connection->poaFactory;
					connection->poaFactory = NULL;
				}

				try {
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroying ORB");
					connection->orbRef->destroy();
					CORBA::release(connection->orbRef);
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroyed ORB");
				} catch (CORBA::Exception &ex) {
					LOG4CXX_ERROR(
							loggerOrbManagement,
							(char*) "shutdownBindings Unexpected CORBA exception destroying orb: "
									<< ex._name());
				}

				///*
				if (connection->callback_poa) {
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroying callback_poa");
					//		delete innerPoa;
					connection->callback_poa = NULL;
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroyed callback_poa");
				}
				if (connection->default_ctx) {
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroying default_ctx");
					//		delete ctx;
					connection->default_ctx = NULL;
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroyed default_ctx");
				}
				if (connection->name_ctx) {
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroying name_ctx");
					//		delete nameCtx;
					connection->name_ctx = NULL;
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroyed name_ctx");
				}
				if (connection->root_poa_manager) {
					LOG4CXX_DEBUG(
							loggerOrbManagement,
							(char*) "shutdownBindings destroying root_poa_manager");
					//		delete poa_manager;
					connection->root_poa_manager = NULL;
					LOG4CXX_DEBUG(
							loggerOrbManagement,
							(char*) "shutdownBindings destroyed root_poa_manager");
				}
				if (connection->root_poa) {
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroying root_poa");
					//		delete poa;
					//connection->root_poa->destroy(false, false); // hmm it doesn't like this
					//destroy(in boolean etherealize_objects,  in boolean wait_for_completion)

					connection->root_poa = NULL;
					LOG4CXX_DEBUG(loggerOrbManagement,
							(char*) "shutdownBindings destroyed root_poa");
				}
				//*/
				LOG4CXX_DEBUG(loggerOrbManagement, (char*) "Closed Bindings");
			}
		}
	}
}

CORBA::ORB_ptr find_orb(const char * orbid) {
	TAO::ORB_Table * const orb_table = TAO::ORB_Table::instance();
	::TAO_ORB_Core* oc = (orbid == NULL ? orb_table->first_orb()
			: orb_table->find(orbid));

	LOG4CXX_DEBUG(loggerOrbManagement, (char *) "XXX find_orb "
			<< (orbid ? orbid : "NULL") << (char *) " - using "
			<< (oc ? oc->orbid() : "NULL"));

	return (oc == 0 ? NULL : oc->orb());
}

CORBA::ORB_ptr find_other_orb(const char * orbid) {
	TAO::ORB_Table * const table = TAO::ORB_Table::instance();
	TAO::ORB_Table::iterator const end = table->end();

	for (TAO::ORB_Table::iterator i = table->begin(); i != end; ++i) {
		::TAO_ORB_Core* oc = (*i).second.core();
		if (ACE_OS::strcmp(oc->orbid(), orbid) != 0)
			return oc->orb();
	}

	return NULL;
}

void print_orb_ids() {
	TAO::ORB_Table * const table = TAO::ORB_Table::instance();
	TAO::ORB_Table::iterator const end = table->end();

	for (TAO::ORB_Table::iterator i = table->begin(); i != end; ++i)
		LOG4CXX_LOGLS(loggerOrbManagement, log4cxx::Level::getInfo(),
				(char *) "\torbid: " << ((*i).second.core())->orbid());
}

char* atmi_object_to_string(CORBA::Object_ptr ctrl, char * orbid) {
	if (!CORBA::is_nil(ctrl)) {
		CORBA::ORB_ptr orb = find_orb(orbid);

		if (orb) {
			CORBA::String_var cs = orb->object_to_string(ctrl);
			return ACE_OS::strdup(cs);
		}
	}

	LOG4CXX_LOGLS(loggerOrbManagement, log4cxx::Level::getTrace(),
			(char *) "current is NULL");

	return NULL;
}

CORBA::Object_ptr atmi_string_to_object(char * ior, char * orbid) {
	LOG4CXX_LOGLS(loggerOrbManagement, log4cxx::Level::getTrace(),
			(char *) "\tconverting ior: " << (ior ? ior : "NULL"));

	if (ior != NULL) {
		CORBA::ORB_ptr orb = find_orb(orbid);

		if (orb)
			return orb->string_to_object(ior);
		else
			LOG4CXX_LOGLS(loggerOrbManagement, log4cxx::Level::getTrace(),
					"no such orb");
	} else {
		LOG4CXX_LOGLS(loggerOrbManagement, log4cxx::Level::getTrace(),
				"no such ior");
	}

	return NULL;
}
