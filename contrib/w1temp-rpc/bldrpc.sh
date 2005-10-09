#!/bin/bash

cd /opt/src
rm -rf xmlrpc-c-1.03.05
tar -xzf xmlrpc-c-1.03.05.tgz
cd  xmlrpc-c-1.03.05
./configure --disable-libwww-client --disable-abyss-server --disable-cplusplus
sudo rm /usr/local/lib/libxmlrpc*
sudo rm -rf /usr/local/include/xmlrpc*
make -i
sudo make -i install
cd /usr/local/lib
sudo strip --strip-unneeded libxmlrpc*so.*.*

	
	
