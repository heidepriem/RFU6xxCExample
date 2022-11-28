# RFU6xxCExample
Example controlling a SICK RFU6xx with OPC UA via Open62541 stack.

Author: Jakob Vollmer

Client for RFU6xx in C

# Installation on linux #

More information about the installation and the documentation can be found [here](https://open62541.org/doc/current/installing.html "Open62541 Dokumentation")

-----

## Dependencies ##

 * gcc-multilib
 * cmake
 * git

Installation on linux:
> sudo apt-get update
> 
> sudo apt-get install gcc-multilib cmake git

## Installation option 1 ##

### Build open62541 ###

Create an temp folder to build open62541
> mkdir tmp && cd tmp

Clone open62541 from github
> git clone https://github.com/open62541/open62541.git && cd open62541

Update submodule
> git submodule update --init --recursive

Create folder for the build files
> mkdir build && cd build

Build open62541
> cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL -DUA_ENABLE_AMALGAMATION=ON ..
> 
> make
> 
> sudo make install

Now two files should have been created in the folder:
> open62541.c and open62541.h

Copy these two files into your projekt folder.

### Build and run example project ###

#### Folder structure of example project ####

* YourProjectFolder
    * open62541.h
    * open62541.c
    * RFU6xxClient.h
    * RFU6xxClient.c
    * main.c
    * makefile

#### Commands to run the example project ####

To run the example program, you must first compile the program. 

To do this, run the following command in your project folder:

> make 

When compiling the program for the first time, it may take a bit longer.

The program can then be run with the following command:

> ./main <YOUR_SERVER_IP>:<YOUR_SERVER_PORT>

## Installation option 2 ##

### Build open62541 ###

Create an temp folder to build open62541
> mkdir tmp && cd tmp

Clone open62541 from github
> git clone https://github.com/open62541/open62541.git && cd open62541

Update submodule
> git submodule update --init --recursive

Create folder for the build files
> mkdir build && cd build

Build open62541
> cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL ..
> 
> make
> 
> sudo make install

### Build and run example project ###

To run the example program, you must first compile the program. 

To do this, run the following command in your project folder:

> gcc main.c RFU6xxClient.c -o main -Wl,-rpath,<PATH_TO_YOUR_LIB_FOLDER> <PATH_TO_YOUR_OPEN62541_LIB_FILE> 
>
> Example for linux: gcc main.c RFU6xxClient.c -o main -Wl,-rpath,/usr/local/lib /usr/local/lib/libopen62541.so

The program can then be run with the following command:

> ./main <YOUR_SERVER_IP>:<YOUR_SERVER_PORT>
