cd ../../
rm -rf dist
mkdir dist
cp blacktie/target/blacktie-*-bin.tar.gz dist
cd dist
tar xfz blacktie-*-bin.tar.gz
cd blacktie*/

# MAKE SURE THE ENVIRONMENT VARIABLES ARE SET
export HOSTNAME_TO_USE=`hostname`
export VAR=`pwd`
sed -i "s=REPLACE_WITH_INSTALL_LOCATION=$VAR=g" setenv.sh
for i in `find . -name Environment.xml`; do sed -i "s=REPLACE_WITH_INSTALL_LOCATION=$VAR=g" $i; done
for i in `find . -name Environment.xml`; do sed -i "s=REPLACE_WITH_HOSTNAME=$HOSTNAME_TO_USE=g" $i; done
. setenv.sh

./run_all_samples.sh
