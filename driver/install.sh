#!/bin/bash
# Martina Salvati
#
#


if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root (use sudo)" 1>&2
	exit 1
fi


echo "----------------------------------------------------------------------------------------------------"
echo  "     WELCOME TO THE LINUX MULTIFLOW DEVICE DRIVER INSTALL SCRIPT BY MARTINA SALVATI       "
echo "----------------------------------------------------------------------------------------------------"
echo "      Enter command :                                                                          "
echo "        1  To install the program enter                                                  "
echo "        2  To uninstall the program enter                                              "
echo "        3  To close the install script enter                                                "
echo "        4  To install and call the test script                                                "
echo "----------------------------------------------------------------------------------------------------"

while read -r VAR; do

if [ $VAR = 1 ]; then
echo
make clean
make
sudo make install

echo "####################################################################################################"
echo "                      Multi-flow Linux Driver installation complete     !!!!!"
echo "####################################################################################################"
break

elif [ $VAR = 2 ]; then
echo
make
sudo rm /dev/mfdev*
sudo make uninstall

echo "####################################################################################################"
echo "                     Multi-flow Linux Driver uninstallation complete     !!!!!"
echo "####################################################################################################"
break

elif [ $VAR = 3 ]; then

echo "####################################################################################################"
echo "                                           Good bye                                                 "
echo "####################################################################################################"
break

elif [ $VAR = 4 ]; then
make clean
make
sudo make install

echo "####################################################################################################"
echo "                      Multi-flow Linux Driver installation complete     !!!!!"
echo "####################################################################################################"
cd .. 
cd test
sudo chmod +x test.sh
sudo ./test.sh
break

else 
    echo "Command not found!!                         "
fi

done