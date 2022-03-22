#!/bin/bash
# Martina Salvati
#
#

if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root (use sudo)" 1>&2
	exit 1
fi


echo "----------------------------------------------------------------------------------------------------"
echo "        WELCOME TO THE LINUX MULTIFLOW DEVICE DRIVER TEST SCRIPT BY MARTINA SALVATI                 "
echo "----------------------------------------------------------------------------------------------------"
echo "        Enter the major number of driver:                                                          "
echo "----------------------------------------------------------------------------------------------------"

read -r MAJ; 

echo "        The major number is $MAJ"

echo "----------------------------------------------------------------------------------------------------"
echo "        Enter the command  :                                              "
echo "        1    Run simple_test                                                                            "
echo "        2    Run param_test                                                                             "
echo "        3    Run enable_disable_test                                                                    "
echo "        4    Run concurrency_test                                                                       "
echo "        5    Run timeout_test                                                                           "
echo "        6    Run write-and_read_test                                                                    "
echo "        7    Exit Test Program                                                                           "
echo "----------------------------------------------------------------------------------------------------"


while read -r VAR; do

if [ $VAR = 1 ]; then
echo
echo "        Enter the minor number :                                                          "
read -r MIN; 
if [[ $MIN -gt 128 ]]; then
	echo "The minor number can't be greather than 128" 
	break
	fi
echo "        The minor number is $MIN                             "
make simple_test 
sudo ./simple_test /dev/mfdev $MAJ $MIN
break

elif [ $VAR = 2 ]; then
echo
echo
echo "        Enter the minor number :                                                          "
read -r MIN; 
 if [[ $MIN -gt 128 ]]; then
	echo "The minor number can't be greather than 128" 
	break
	fi
echo "        The minor number is $MIN       "
make param_test 
sudo ./param_test /dev/mfdev $MAJ $MIN        
break

elif [ $VAR = 3 ]; then
echo
echo
echo "        Enter the minor number :                                                          "
read -r MIN; 
if [[ $MIN -gt 128 ]]; then
	echo "The minor number can't be greather than 128" 
	break 
fi 
echo "        The minor number is $MIN                             "
make enable_disable_test
sudo ./enable_disable_test /dev/mfdev $MAJ $MIN  
break

elif [ $VAR = 4 ]; then
echo
echo
echo "        Enter the minor number :                                                          "
read -r MIN; 
if [[ $MIN -gt 128 ]]; then
	echo "The minor number can't be greather than 128" 
	break
 fi
echo "        The minor number is $MIN                             "
make concurrency_test
sudo ./concurrency_test /dev/mfdev $MAJ $MIN       
break

elif [ $VAR = 5 ]; then
echo9
echo
echo "        Enter the minor number :                                                          "
read -r MIN; 
 if [[ $MIN -gt 128 ]]; then
	echo "The minor number can't be greather than 128" 
	break
 fi
echo "        The minor number is $MIN                             "
 make timeout_test
 sudo ./timeout_test /dev/mfdev $MAJ $MIN   
break

elif [ $VAR = 6 ]; then
echo
echo
echo "        Enter the minor number :                                                          "
read -r MIN; 
if [[ $MIN -gt 128 ]]; then
	echo "The minor number can't be greather than 128" 
	break
fi
echo "        The minor number is $MIN                             "
make write_and_read_test
sudo ./write_and_read_test /dev/mfdev $MAJ $MIN    
break

elif [ $VAR = 7 ]; then
echo
echo "Goobye"
break

else 
    echo "Command not found!!                         "
fi

done

exit 0