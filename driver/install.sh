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
echo "        ->  To install the program enter : INSTALL                                                  "
echo "        ->  To uninstall the program enter : UNINSTALL                                              "
echo "        ->  To close the install script enter : CLOSE                                               "
echo "----------------------------------------------------------------------------------------------------"

while read -r VAR; do

if [ $VAR = "INSTALL" ]; then
echo
make
sudo make install

echo "####################################################################################################"
echo "                      Multi-flow Linux Driver installation complete     !!!!!"
echo "####################################################################################################"
break

elif [ $VAR = "UNINSTALL" ]; then
echo
make
sudo rm /dev/mfdev*
sudo make uninstall

echo "####################################################################################################"
echo "                     Multi-flow Linux Driver uninstallation complete     !!!!!"
echo "####################################################################################################"
break

elif [ $VAR = "CLOSE" ]; then

echo "####################################################################################################"
echo "                                           Good bye                                                 "
echo "####################################################################################################"
break

else 
    echo "Command not found!!                         "
fi

done