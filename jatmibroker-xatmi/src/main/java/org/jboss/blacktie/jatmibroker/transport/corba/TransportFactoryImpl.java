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
package org.jboss.blacktie.jatmibroker.transport.corba;

import java.util.Properties;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.blacktie.jatmibroker.JAtmiBrokerException;
import org.jboss.blacktie.jatmibroker.transport.OrbManagement;
import org.jboss.blacktie.jatmibroker.transport.Transport;
import org.jboss.blacktie.jatmibroker.transport.TransportFactory;

public class TransportFactoryImpl extends TransportFactory {

	private static final Logger log = LogManager
			.getLogger(TransportFactoryImpl.class);
	private OrbManagement orbManagement;

	protected void setProperties(Properties properties)
			throws JAtmiBrokerException {
		log.debug("Creating OrbManagement");
		try {
			orbManagement = new OrbManagement(properties, false);
		} catch (Throwable t) {
			throw new JAtmiBrokerException(
					"Could not create the orb management function", t);
		}
		log.debug("Created OrbManagement");
	}

	public Transport createTransport(String username, String password)
			throws JAtmiBrokerException {
		log.debug("Creating");
		TransportImpl instance = null;
		try {
			instance = new TransportImpl(orbManagement, username, password);
		} catch (Throwable t) {
			throw new JAtmiBrokerException("Could not connect to server", t);
		}
		log.debug("Created");
		return instance;
	}
}