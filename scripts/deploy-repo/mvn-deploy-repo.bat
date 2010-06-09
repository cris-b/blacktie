set CLASSIFIER=vc9x32
set DEPLOY_SETTINGS="-s c:\blacktie\deploy-settings.xml"

set J_ARTIFACT="mvn %DEPLOY_SETTINGS% deploy:deploy-file -DrepositoryId=jboss-releases -Durl=https://repository.jboss.org/nexus/service/local/staging/deploy/maven2"
set C_ARTIFACT="%J_ARTIFACT% -Dclassifier=%CLASSIFIER%"

(cd c:\Documents and Settings\hudson\.m2\repository\apache-log4cxx\contrib\log4cxx\902683\ && %C_ARTIFACT% -Dfile=log4cxx-902683-%CLASSIFIER%.jar -DpomFile=log4cxx-902683.pom -Dversion=902683)
(cd c:\Documents and Settings\hudson\.m2\repository\expat\contrib\expat\2.0.1\ && %C_ARTIFACT% -DpomFile=expat-2.0.1.pom -Dverion=2.0.1 -Dfile=expat-2.0.1-%CLASSIFIER%.jar)
(cd c:\Documents and Settings\hudson\.m2\repository\cppunit\contrib\cppunit\1.12\ && %C_ARTIFACT% -DpomFile=cppunit-1.12.pom -Dverion=1.12 -Dfile=cppunit-1.12-%CLASSIFIER%.jar)
(cd c:\Documents and Settings\hudson\.m2\repository\apr-1\contrib\apr-1\1.3.3\ && %C_ARTIFACT% -DpomFile=apr-1-1.3.3.pom -Dverion=1-1.3.3 -Dfile=apr-1-1.3.3-%CLASSIFIER%.jar)
(cd c:\Documents and Settings\hudson\.m2\repository\org\codehaus\stomp\stompconnect\1.0-BT\ && %J_ARTIFACT% -DpomFile=stompconnect-1.0-BT.pom -Dverion=1.0-BT -Dfile=stompconnect-1.0-BT.jar)
(cd c:\Documents and Settings\hudson\.m2\repository\apache-xercesc\contrib\xercesc\3.0.1\ && %C_ARTIFACT% -DpomFile=xercesc-3.0.1.pom -Dverion=3.0.1 -Dfile=xercesc-3.0.1-%CLASSIFIER%.zip)
(cd c:\Documents and Settings\hudson\.m2\repository\ace\contrib\ace\5.7.6\ && %C_ARTIFACT% -DpomFile=ace-5.7.6.pom -Dverion=5.7.6 -Dfile=ace-5.7.6-%CLASSIFIER%.zip)
