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
package org.jboss.blacktie.jatmibroker.transport;

import java.util.Properties;

import junit.framework.TestCase;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.blacktie.jatmibroker.RunServer;
import org.jboss.blacktie.jatmibroker.core.conf.AtmiBrokerClientXML;
import org.jboss.blacktie.jatmibroker.core.transport.Message;
import org.jboss.blacktie.jatmibroker.core.transport.Receiver;
import org.jboss.blacktie.jatmibroker.core.transport.Sender;
import org.jboss.blacktie.jatmibroker.core.transport.Transport;
import org.jboss.blacktie.jatmibroker.core.transport.TransportFactory;

public class ConnectionImplTest extends TestCase {
	private static final Logger log = LogManager
			.getLogger(ConnectionImplTest.class);
	private RunServer server = new RunServer();

	public void setUp() throws InterruptedException {
		server.serverinit();
	}

	public void tearDown() {
		server.serverdone();
	}

	public void test() throws Exception {
		AtmiBrokerClientXML xml = new AtmiBrokerClientXML();
		Properties properties = null;
		properties = xml.getProperties();

		Transport proxy = TransportFactory.loadTransportFactory("BAR",
				properties).createTransport();
		Sender serviceFactory = proxy.getSender("BAR");

		String aString = "Hello from Java Land";
		Receiver endpoint = proxy.createReceiver();
		serviceFactory.send(endpoint.getReplyTo(), (short) 0, 0, aString
				.getBytes(), aString.getBytes().length, 0, 0, 0, "X_OCTET", "");
		Message receive = endpoint.receive(0);

		assertNotNull(receive);
		String string = new String(receive.data).intern();
		String expectedResponse = "BAR SAYS HELLO";
		log.debug("Bar ServiceManager service_request response is " + string);
		log.debug("Bar ServiceManager service_request size of response is "
				+ receive.len);
		assertEquals(string, expectedResponse);
		proxy.close();
	}
}
