package org.jboss.blacktie.jatmibroker.xatmi;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;

public class TestTTLService implements BlacktieService {
	private static final Logger log = LogManager
			.getLogger(TestTTLService.class);
	private static int n = 0;

	public Response tpservice(TPSVCINFO svcinfo) {
		log.info("test_ttl_service");

		try {
			int timeout = 45;
			Buffer dptr = svcinfo.getBuffer();
			String data = new String(dptr.getData());
			log.info("test_ttl_service get data: " + data);

			int len = 60;
			Buffer toReturn = new Buffer("X_OCTET", null);

			if (data.contains("counter")) {
				toReturn.setData(String.valueOf(n).getBytes());
			} else {
				n ++;
				try {
					log.info("test_ttl_service sleep for " + timeout + " seconds");
					Thread.sleep(timeout * 1000);
					log.info("test_ttl_service slept for " + timeout + " seconds");
					toReturn.setData("test_ttl_service".getBytes());
				} catch (Exception e) {
					log.error("sleep failed with " + e);
				}
			}
			return new Response(Connection.TPSUCCESS, 22, toReturn, len, 0);
		} catch (ConnectionException e) {
			return new Response(Connection.TPFAIL, 0, null, 0, 0);
		}
	}
}