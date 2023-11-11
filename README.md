# glimpse

This project aims to monitor important Linux processes in a clear and concise way when connecting remotely through a terminal.
This aims to give the user the most important processes per component at a glimpse.


## Install
**make** and **g++** are used to build the program  
**ncurses** is used for the interface  
**read-edid** is used to get the info of the connected monitor (if it exists)  
**dmidecode** is used to get info about installed RAM slots and sticks  
**wireless-tools** is used to get the name/SSID of the connected WiFi network  
**lsof** is used to get the processes with open connections to the internet

Install on Ubuntu 22.04:
``` bash
sudo apt install -y make g++ libncurses5-dev read-edid dmidecode wireless-tools lsof
```
## Build
``` bash
make
```
Can also be cleaned with:
``` bash
make clean
```

## Run
The program requires sudo priviledges to read some sysfs files  
``` bash
sudo ./glimpse
```