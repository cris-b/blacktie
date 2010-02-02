/*
 * RHQ Management Platform
 * Copyright (C) 2005-2008 Red Hat, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.rhq.plugins.blacktie;

import java.io.InputStream;
import java.io.StringWriter;
import java.util.List;
import java.util.Properties;
import java.util.Set;

import javax.management.MBeanServerConnection;
import javax.management.ObjectName;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.jboss.blacktie.jatmibroker.core.conf.XMLEnvHandler;
import org.jboss.blacktie.jatmibroker.core.conf.XMLParser;
import org.rhq.core.domain.configuration.Configuration;
import org.rhq.core.domain.configuration.ConfigurationUpdateStatus;
import org.rhq.core.domain.content.PackageType;
import org.rhq.core.domain.content.transfer.DeployPackageStep;
import org.rhq.core.domain.content.transfer.DeployPackagesResponse;
import org.rhq.core.domain.content.transfer.RemovePackagesResponse;
import org.rhq.core.domain.content.transfer.ResourcePackageDetails;
import org.rhq.core.domain.measurement.AvailabilityType;
import org.rhq.core.domain.measurement.MeasurementDataNumeric;
import org.rhq.core.domain.measurement.MeasurementReport;
import org.rhq.core.domain.measurement.MeasurementScheduleRequest;
import org.rhq.core.pluginapi.configuration.ConfigurationFacet;
import org.rhq.core.pluginapi.configuration.ConfigurationUpdateReport;
import org.rhq.core.pluginapi.content.ContentFacet;
import org.rhq.core.pluginapi.content.ContentServices;
import org.rhq.core.pluginapi.inventory.CreateChildResourceFacet;
import org.rhq.core.pluginapi.inventory.CreateResourceReport;
import org.rhq.core.pluginapi.inventory.DeleteResourceFacet;
import org.rhq.core.pluginapi.inventory.ResourceComponent;
import org.rhq.core.pluginapi.inventory.ResourceContext;
import org.rhq.core.pluginapi.measurement.MeasurementFacet;
import org.rhq.core.pluginapi.operation.OperationFacet;
import org.rhq.core.pluginapi.operation.OperationResult;
import org.w3c.dom.Element;

/**
 * This can be the start of your own custom plugin's server component. Review
 * the javadoc for {@link ResourceComponent} and all the facet interfaces to
 * learn what you can do in your resource component. This component has a lot of
 * methods in it because it implements all possible facets. If your resource
 * does not support, for example, configuration, you can remove the
 * {@link ConfigurationFacet} from the <code>implements</code> clause and remove
 * all method implementations that that facet required.
 * 
 * <p>
 * You should not only read the javadoc in each of this class' methods, but you
 * should also read the javadocs linked by their "see" javadoc tags since those
 * additional javadocs will contain a good deal of additional information you
 * will need to know.
 * </p>
 * 
 * @author John Mazzitelli
 */
public class ServiceComponent implements ResourceComponent, MeasurementFacet,
OperationFacet, ConfigurationFacet, ContentFacet, DeleteResourceFacet,
CreateChildResourceFacet {
	private final Log log = LogFactory.getLog(ServiceComponent.class);

	/**
	 * Represents the resource configuration of the custom product being
	 * managed.
	 */
	private Configuration resourceConfiguration;

	private ResourceContext resourceContext;

	private Properties prop = new Properties();

	private MBeanServerConnection beanServerConnection;

	private String serverName = null;
	private String serviceName = null;

	private ObjectName blacktieAdmin = null;
	
	private String[]  times = null;

	/**
	 * This is called when your component has been started with the given
	 * context. You normally initialize some internal state of your component as
	 * well as attempt to make a stateful connection to your managed resource.
	 * 
	 * @see ResourceComponent#start(ResourceContext)
	 */
	public void start(ResourceContext context) {
		try {
			XMLEnvHandler handler = new XMLEnvHandler("", prop);
			XMLParser xmlenv = new XMLParser(handler, "Environment.xsd");
			xmlenv.parse("Environment.xml", true);
			JMXServiceURL u = new JMXServiceURL((String) prop.get("JMXURL"));
			JMXConnector c = JMXConnectorFactory.connect(u);
			beanServerConnection = c.getMBeanServerConnection();

			serviceName = context.getResourceKey();
			serverName = prop.getProperty("blacktie." + serviceName + ".server");
			blacktieAdmin = new ObjectName("jboss.blacktie:service=Admin");
		} catch (Exception e) {
			log.error("start server " + serviceName + " plugin error with " + e);
		}
		log.debug("start resource: " + serviceName);
	}

	/**
	 * This is called when the component is being stopped, usually due to the
	 * plugin container shutting down. You can perform some cleanup here; though
	 * normally not much needs to be done here.
	 * 
	 * @see ResourceComponent#stop()
	 */
	public void stop() {
	}

	/**
	 * All resource components must be able to tell the plugin container if the
	 * managed resource is available or not. This method is called by the plugin
	 * container when it needs to know if the managed resource is actually up
	 * and available.
	 * 
	 * @see ResourceComponent#getAvailability()
	 */
	public AvailabilityType getAvailability() {
		AvailabilityType status = AvailabilityType.DOWN;

		try {
			ObjectName objName = new ObjectName(
					"jboss.messaging.destination:service=Queue,name=" + 
					serviceName);
			beanServerConnection.getAttribute(objName, "ConsumerCount");
			status = AvailabilityType.UP;
		} catch (Exception e) {

		}
		return status;
	}

	/**
	 * The plugin container will call this method when your resource component
	 * has been scheduled to collect some measurements now. It is within this
	 * method that you actually talk to the managed resource and collect the
	 * measurement data that is has emitted.
	 * 
	 * @see MeasurementFacet#getValues(MeasurementReport, Set)
	 */
	public void getValues(MeasurementReport report,
			Set<MeasurementScheduleRequest> requests) {
		for (MeasurementScheduleRequest request : requests) {
			String name = request.getName();
			
			try {
				if (name.equals("messageCounter")) {
					Number value = (Long)beanServerConnection.invoke(blacktieAdmin, 
							"getServiceCounter",
							new Object[] { serverName, serviceName}, 
							new String[] {"java.lang.String", "java.lang.String"});
					
					report.addData(new MeasurementDataNumeric(request, value
							.doubleValue()));
				} else if (name.equals("minResponseTime")) {
					String responseTime = (String)beanServerConnection.invoke(blacktieAdmin, 
							"getResponseTime",
							new Object[] { serverName, serviceName}, 
							new String[] {"java.lang.String", "java.lang.String"});
					times = responseTime.split(",");
					Number value = Long.parseLong(times[0]);
					report.addData(new MeasurementDataNumeric(request, value
							.doubleValue()));
				} else if (name.equals("avgResponseTime")) {
					if(times != null) {
						Number value = Long.parseLong(times[1]);
						report.addData(new MeasurementDataNumeric(request, value
								.doubleValue()));
					}
				} else if (name.equals("maxResponseTime")) {
					if(times != null) {
						Number value = Long.parseLong(times[2]);
						report.addData(new MeasurementDataNumeric(request, value
								.doubleValue()));
					}
				} else if (name.equals("queueDepth")) {
					Number value = (Integer)beanServerConnection.invoke(blacktieAdmin, 
							"getQueueDepth",
							new Object[] { serverName, serviceName}, 
							new String[] {"java.lang.String", "java.lang.String"});
					report.addData(new MeasurementDataNumeric(request, value
							.doubleValue()));
				}
			} catch (Exception e) {
				log.error("Failed to obtain measurement [" + name
						+ "]. Cause: " + e);
				e.printStackTrace();
			}
		}

		return;
	}

	/**
	 * The plugin container will call this method when it wants to invoke an
	 * operation on your managed resource. Your plugin will connect to the
	 * managed resource and invoke the analogous operation in your own custom
	 * way.
	 * 
	 * @see OperationFacet#invokeOperation(String, Configuration)
	 */
	public OperationResult invokeOperation(String name, Configuration params) {
		OperationResult result = new OperationResult();
		int id = Integer.parseInt(params.getSimpleValue("id", "0"));

		if (name.equals("advertise") || name.equals("unadvertise")) {		
			if(serviceName == null) {
				result.setErrorMessage("service name can not empty");
			} else {
				try{
					Boolean r =  (Boolean)beanServerConnection.invoke(blacktieAdmin, 
							name,
							new Object[] { serverName, serviceName}, 
							new String[] {"java.lang.String", "java.lang.String"});
					if(r){
						result.setSimpleResult(name + " OK");
					} else {
						result.setErrorMessage(name + " FAIL");
					}
				} catch (Exception e) {
					log.error("call " + name + " service " + serviceName
							+ " failed with " + e);
					result.setErrorMessage("call " + name + " service " + serviceName
							+ " failed with " + e);
				}
			}
		} else if (name.equals("listServiceStatus")) {
			try {
				Element status;
				if(id == 0) {
					status = (Element)beanServerConnection.invoke(blacktieAdmin, 
							"listServiceStatus",
							new Object[] { serverName, serviceName}, 
							new String[] {"java.lang.String", "java.lang.String"});
				} else {
					status = (Element)beanServerConnection.invoke(blacktieAdmin, 
							"listServiceStatusById",
							new Object[] { serverName, id, serviceName}, 
							new String[] {"java.lang.String", "int", "java.lang.String"});
				}

				if (status != null) {
					try {
						// Set up the output transformer
						TransformerFactory transfac = TransformerFactory.newInstance();
						Transformer trans = transfac.newTransformer();
						trans.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
						trans.setOutputProperty(OutputKeys.INDENT, "yes");

						StringWriter sw = new StringWriter();
						StreamResult sr = new StreamResult(sw);
						DOMSource source = new DOMSource(status);
						trans.transform(source, sr);
						result.setSimpleResult(sw.toString());					
					} catch (TransformerException e) {
						log.error(e);
					}
				} else {
					result.setErrorMessage("no service status");
				}
			} catch (Exception e) {
				log.error("call status failed with "+ e);
				result.setErrorMessage("call status failed with "+ e);
			}
		} else if (name.equals("getServiceCounter")) {
			try {
				if(serviceName == null) {
					result.setErrorMessage("service name can not empty");
				} else {
					Long counter;
					if(id == 0) {
						counter = (Long)beanServerConnection.invoke(blacktieAdmin, 
								"getServiceCounter",
								new Object[] { serverName, serviceName}, 
								new String[] {"java.lang.String", "java.lang.String"});
					} else {
						counter = (Long)beanServerConnection.invoke(blacktieAdmin, 
								"getServiceCounterById",
								new Object[] { serverName, id, serviceName}, 
								new String[] {"java.lang.String", "int", "java.lang.String"});
					}
					result.setSimpleResult(counter.toString());
				}
			} catch (Exception e) {
				log.error("call get counter of " + serviceName 
						+ " failed with " + e);
				result.setErrorMessage("call get counter of " + serviceName 
						+ " failed with " + e);
			}
		}
		return result;
	}

	/**
	 * The plugin container will call this method and it needs to obtain the
	 * current configuration of the managed resource. Your plugin will obtain
	 * the managed resource's configuration in your own custom way and populate
	 * the returned Configuration object with the managed resource's
	 * configuration property values.
	 * 
	 * @see ConfigurationFacet#loadResourceConfiguration()
	 */
	public Configuration loadResourceConfiguration() {
		// here we simulate the loading of the managed resource's configuration

		if (resourceConfiguration == null) {
			// for this example, we will create a simple dummy configuration to
			// start with.
			// note that it is empty, so we're assuming there are no required
			// configs in the plugin descriptor.
			resourceConfiguration = new Configuration();
		}

		Configuration config = resourceConfiguration;

		return config;
	}

	/**
	 * The plugin container will call this method when it has a new
	 * configuration for your managed resource. Your plugin will re-configure
	 * the managed resource in your own custom way, setting its configuration
	 * based on the new values of the given configuration.
	 * 
	 * @see ConfigurationFacet#updateResourceConfiguration(ConfigurationUpdateReport)
	 */
	public void updateResourceConfiguration(ConfigurationUpdateReport report) {
		// this simulates the plugin taking the new configuration and
		// reconfiguring the managed resource
		resourceConfiguration = report.getConfiguration().deepCopy();

		report.setStatus(ConfigurationUpdateStatus.SUCCESS);
	}

	/**
	 * When this is called, the plugin is responsible for scanning its managed
	 * resource and look for content that need to be managed for that resource.
	 * This method should only discover packages of the given package type.
	 * 
	 * @see ContentFacet#discoverDeployedPackages(PackageType)
	 */
	public Set<ResourcePackageDetails> discoverDeployedPackages(PackageType type) {
		return null;
	}

	/**
	 * The plugin container calls this method when new packages need to be
	 * deployed/installed on resources.
	 * 
	 * @see ContentFacet#deployPackages(Set, ContentServices)
	 */
	public DeployPackagesResponse deployPackages(
			Set<ResourcePackageDetails> packages,
			ContentServices contentServices) {
		return null;
	}

	/**
	 * When a remote client wants to see the actual data content for an
	 * installed package, this method will be called. This method must return a
	 * stream of data containing the full content of the package.
	 * 
	 * @see ContentFacet#retrievePackageBits(ResourcePackageDetails)
	 */
	public InputStream retrievePackageBits(ResourcePackageDetails packageDetails) {
		return null;
	}

	/**
	 * This is the method that is used when the component has to create the
	 * installation steps and their results.
	 * 
	 * @see ContentFacet#generateInstallationSteps(ResourcePackageDetails)
	 */
	public List<DeployPackageStep> generateInstallationSteps(
			ResourcePackageDetails packageDetails) {
		return null;
	}

	/**
	 * This is called when the actual content of packages should be deleted from
	 * the managed resource.
	 * 
	 * @see ContentFacet#removePackages(Set)
	 */
	public RemovePackagesResponse removePackages(
			Set<ResourcePackageDetails> packages) {
		return null;
	}

	/**
	 * When called, the plugin container is asking the plugin to create a new
	 * managed resource. The new resource's details need to be added to the
	 * given report.
	 * 
	 * @see CreateChildResourceFacet#createResource(CreateResourceReport)
	 */
	public CreateResourceReport createResource(CreateResourceReport report) {
		return null;
	}

	/**
	 * When called, the plugin container is asking the plugin to delete a
	 * managed resource.
	 * 
	 * @see DeleteResourceFacet#deleteResource()
	 */
	public void deleteResource() {
	}
}