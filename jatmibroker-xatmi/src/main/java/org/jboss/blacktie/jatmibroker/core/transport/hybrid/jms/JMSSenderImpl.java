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
package org.jboss.blacktie.jatmibroker.core.transport.hybrid.jms;

import javax.jms.BytesMessage;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.naming.NamingException;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.blacktie.jatmibroker.core.transport.JtsTransactionImple;
import org.jboss.blacktie.jatmibroker.core.transport.Sender;
import org.jboss.blacktie.jatmibroker.xatmi.Connection;
import org.jboss.blacktie.jatmibroker.xatmi.ConnectionException;

public class JMSSenderImpl implements Sender {
	private static final Logger log = LogManager.getLogger(JMSSenderImpl.class);
	private MessageProducer sender;
	private Session session;
	private String name;
	private boolean closed;
	private Destination destination;

	private int pad = 0;

	public JMSSenderImpl(JMSManagement management, String serviceName,
			boolean conversational) throws NamingException, JMSException {
		Destination destination = management
				.lookup(serviceName, conversational);
		log.trace("Resolved destination");

		this.session = management.createSession();
		sender = session.createProducer(destination);
		this.name = ((Queue) destination).getQueueName();
		this.destination = destination;
		log.debug("Sender Created: " + name);
	}

	public void send(Object replyTo, short rval, int rcode, byte[] data,
			int len, int correlationId, int flags, int ttl, String type,
			String subtype) throws ConnectionException {
		if (closed) {
			throw new ConnectionException(Connection.TPEPROTO, "Sender closed");
		}

		if (data == null) {
			data = new byte[1];
			len = 1;
		}
		if (len < 1) {
			throw new ConnectionException(Connection.TPEINVAL,
					"Length of buffer must be greater than 0");
		}

		log.debug("Sender sending: " + name);
		try {
			BytesMessage message = session.createBytesMessage();
			String ior = JtsTransactionImple.getTransactionIOR();

			message.setStringProperty("messagecontrol", ior);
			log.debug("Sender sending IOR: " + ior);
			if (replyTo != null) {
				message.setStringProperty("messagereplyto", (String) replyTo);
			}
			message.setStringProperty("servicename", name);
			message.setStringProperty("messagecorrelationId",
					String.valueOf(correlationId));
			message.setStringProperty("messageflags", String.valueOf(flags));
			message.setStringProperty("messagerval", String.valueOf(rval));
			message.setStringProperty("messagercode", String.valueOf(rcode));
			message.setStringProperty("messagetype", type == null ? "" : type);
			message.setStringProperty("messagesubtype", subtype == null ? ""
					: subtype);

			byte[] toSend = new byte[len + pad];
			if (data != null) {
				int min = Math.min(toSend.length, data.length);
				System.arraycopy(data, 0, toSend, 0, min);
			}
			message.writeBytes(toSend, 0, toSend.length);
			if (ttl > 0) {
				int deliveryMode = message.getJMSDeliveryMode();
				int priority = message.getJMSPriority();

				log.debug("send message with time-to-live " + ttl);
				sender.send(message, deliveryMode, priority, ttl);
			} else {
				sender.send(message);
			}
			log.debug("sent message");
		} catch (Throwable e) {
			throw new ConnectionException(Connection.TPESYSTEM,
					"Could not send the message: " + e.getMessage(), e);
		}
	}

	public void close() throws ConnectionException {
		if (closed) {
			throw new ConnectionException(Connection.TPEPROTO,
					"Sender already closed");
		}
		try {
			log.debug("Sender closing: " + name);
			sender.close();
			session.close();
			sender = null;
			closed = true;
			log.debug("Sender closed: " + name);
		} catch (Throwable t) {
			throw new ConnectionException(Connection.TPESYSTEM,
					"Could not send the message", t);
		}
	}

	public Object getSendTo() {
		return destination;
	}
}