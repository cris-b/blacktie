package org.jboss.blacktie.jatmibroker.conf;

public class ConfigurationException extends Exception {
	private static final long serialVersionUID = 1L;

	public ConfigurationException(String string, Throwable t) {
		super(string, t);
	}

	public ConfigurationException(String string) {
		super(string);
	}
}