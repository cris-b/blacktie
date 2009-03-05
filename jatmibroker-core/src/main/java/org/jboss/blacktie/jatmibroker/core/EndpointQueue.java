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
package org.jboss.blacktie.jatmibroker.core;

import java.util.ArrayList;
import java.util.List;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.blacktie.jatmibroker.core.proxy.Queue;
import org.omg.CORBA.Object;
import org.omg.CORBA.Policy;
import org.omg.CosNaming.NameComponent;
import org.omg.PortableServer.LifespanPolicyValue;
import org.omg.PortableServer.POA;
import org.omg.PortableServer.POAPackage.AdapterAlreadyExists;
import org.omg.PortableServer.POAPackage.AdapterNonExistent;
import org.omg.PortableServer.POAPackage.InvalidPolicy;
import org.omg.PortableServer.POAPackage.ServantAlreadyActive;
import org.omg.PortableServer.POAPackage.ServantNotActive;
import org.omg.PortableServer.POAPackage.WrongPolicy;

import AtmiBroker.EndpointQueuePOA;

public class EndpointQueue extends EndpointQueuePOA implements Queue {
	private static final Logger log = LogManager.getLogger(EndpointQueue.class);
	private POA m_default_poa;
	private String callbackIOR;
	private List<Message> returnData = new ArrayList<Message>();
	private byte[] activate_object;
	private String queueName;

	public EndpointQueue(String queueName) throws JAtmiBrokerException {
		this.queueName = queueName;
		int numberOfPolicies = 1;
		Policy[] policiesArray = new Policy[numberOfPolicies];
		List<Policy> policies = new ArrayList<Policy>();
		policies.add(AtmiBrokerServerImpl.root_poa.create_lifespan_policy(LifespanPolicyValue.PERSISTENT));
		// policies.add(AtmiBrokerServerImpl.root_poa.create_thread_policy(ThreadPolicyValue.SINGLE_THREAD_MODEL));
		policies.toArray(policiesArray);

		try {
			this.m_default_poa = AtmiBrokerServerImpl.root_poa.create_POA(queueName, AtmiBrokerServerImpl.root_poa.the_POAManager(), policiesArray);
		} catch (Throwable t) {
			try {
				this.m_default_poa = AtmiBrokerServerImpl.root_poa.find_POA(queueName, true);
			} catch (AdapterNonExistent e) {
				throw new JAtmiBrokerException("Could not find POA:" + queueName, e);
			}
		}
		try {
			activate_object = m_default_poa.activate_object(this);
			Object servant_to_reference = m_default_poa.servant_to_reference(this);
			NameComponent[] name = AtmiBrokerServerImpl.nce.to_name(queueName);
			AtmiBrokerServerImpl.nc.bind(name, servant_to_reference);
		} catch (Throwable t) {
			throw new JAtmiBrokerException("Could not bind service factory" + queueName, t);
		}
	}

	public EndpointQueue(POA poa, String aServerName) throws AdapterNonExistent, InvalidPolicy, ServantAlreadyActive, WrongPolicy, ServantNotActive {
		super();
		log.debug("ClientCallbackImpl constructor ");
		int numberOfPolicies = 0;
		Policy[] policiesArray = new Policy[numberOfPolicies];
		List<Policy> policies = new ArrayList<Policy>();
		// policies.add(AtmiBrokerServerImpl.root_poa.create_thread_policy(ThreadPolicyValue.SINGLE_THREAD_MODEL));
		policies.toArray(policiesArray);

		try {
			m_default_poa = poa.create_POA(aServerName, poa.the_POAManager(), policiesArray);
		} catch (AdapterAlreadyExists e) {
			m_default_poa = poa.find_POA(aServerName, true);
		}
		log.debug("JABSession createCallbackObject ");
		m_default_poa.activate_object(this);
		log.debug("activated this " + this);

		org.omg.CORBA.Object tmp_ref = m_default_poa.servant_to_reference(this);
		log.debug("created reference " + tmp_ref);
		AtmiBroker.EndpointQueue clientCallback = AtmiBroker.EndpointQueueHelper.narrow(tmp_ref);
		log.debug("narrowed reference " + clientCallback);
		callbackIOR = AtmiBrokerServerImpl.orb.object_to_string(clientCallback);
		log.debug(" created ClientCallback ior " + callbackIOR);
	}

	public POA _default_POA() {
		log.debug("ClientCallbackImpl _default_POA");
		return m_default_poa;
	}

	// client_callback() -- Implements IDL operation
	// "AtmiBroker.ClientCallback.client_callback".
	//
	public void send(String replyto_ior, short rval, int rcode, byte[] idata, int ilen, int flags, int revent) {
		log.debug("client_callback(): called.");
		log.debug("    idata = " + new String(idata));
		log.debug("    ilen = " + ilen);
		log.debug("    flags = " + flags);
		log.debug("client_callback(): returning.");
		Message message = new Message();
		message.replyTo = replyto_ior;
		message.data = idata;
		message.len = ilen;
		message.flags = flags;
		message.control = null;// TODO
		message.rval = rval;
		message.rcode = rcode;
		message.event = revent;
		synchronized (this) {
			returnData.add(message);
			notify();
		}
	}

	public String getReplyTo() {
		return callbackIOR;
	}

	public Message receive(long flags) {
		synchronized (this) {
			while (returnData.isEmpty()) {
				try {
					wait();
				} catch (InterruptedException e) {
					log.error("Caught exception", e);
				}
			}
			if (returnData.isEmpty()) {
				return null;
			} else {
				return returnData.remove(0);
			}
		}
	}

	public void disconnect() {
		if (queueName != null) {
			try {
				NameComponent[] name = AtmiBrokerServerImpl.nce.to_name(queueName);
				AtmiBrokerServerImpl.nc.unbind(name);
				m_default_poa.deactivate_object(activate_object);
			} catch (Throwable t) {
				log.error("Could not unbind service factory" + queueName, t);
			}
		}
	}

	public void close() {
		// TODO Auto-generated method stub

	}
}
