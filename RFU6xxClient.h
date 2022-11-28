/*
* Created on 21.01.2022
*
* @author: Jakob Vollmer (DH-Student at SICK AG)
* @author: Sebastian Heidepriem (SICK AG)
* @contact: sebastian.heidepriem@sick.de
* 
* This library communicates with the RFU6xx via opcua.
* To use this library you have to install the open62541 implementation of OPCUA.
*/

#ifndef RFU6xxCLIENT_H
#define RFU6xxCLIENT_H

    // Open62541 Library
    #include <open62541/client_config_default.h>
    #include <open62541/client_highlevel.h>
    #include <open62541/plugin/log_stdout.h>

    #include <stdio.h>
    #include <stdlib.h>

    // Extention Object Node Ids
    #define RFU6xx_START_SCAN_E_O_TYPE_ID 3010
    #define RFU6xx_TAG_ID_E_O_TYPE_ID 5030

    // Status codes returned by the server
    typedef uint32_t RFU6xx_StatusCode;
    #define RFU6xx_STATUSCODE_SUCCESS 0
    #define RFU6xx_STATUSCODE_REGION_NOT_FOUND 5
    #define RFU6xx_STATUSCODE_READ_OUT_OF_RANGE 7
    #define RFU6xx_STATUSCODE_READ_ERROR 10
    #define RFU6xx_STATUSCODE_WRITE_ERROR 14
    #define RFU6xx_STATUSCODE_NOT_SUPPORTED_BY_DEVICE 15

    // Device status of the scanner
    typedef uint32_t RFU6xx_DeviceStatusCode;
    #define RFU6xx_DEVICESTATUSCODE_IDLE 0
    #define RFU6xx_DEVICESTATUSCODE_SCANNING 2
    #define RFU6xx_DEVICESTATUSCODE_BUSY 3

    // Different namespace index
    UA_Int16 nsAutoID;
    UA_Int16 nsOpcDI;
    UA_Int16 nsRfu;

    // Different node ids
    UA_Int16 ndDeviceSetID;
    UA_Int16 ndRfu6xxNodeID;

    UA_Int16 ndLastScanDataID;
    UA_Int16 ndWriteTagID;
    UA_Int16 ndReadTagID;
    UA_Int16 ndScanStartID;
    UA_Int16 ndScanStopID;
    UA_Int16 ndDeviceStatusID;

    /*
    * Function:  serialize32Bit 
    * Function:  serialize64Bit 
    * --------------------
    * Serialize a 32 bit int (64 bit long)
    * 
    *  parameters: 
    *               -> char ** pBit 
    *               -> unsigned int value  or  unsigned long value
    * 
    *  returns: 
    */
    void serialize32Bit(char** pBit, unsigned int value);
    void serialize64Bit(char** pBit, unsigned long value);

    /*
    * Function:  encodeDouble 
    * --------------------
    * Converts a double into an uint64_t value
    * 
    *  parameters: 
    *               -> double value
    * 
    *  returns: 
    *               -> uint64_t
    */
    uint64_t encodeDouble(double value);

    /*
    * Function:  getChildNodeIdByString 
    * --------------------
    * The function searches the children of a starting node and 
    * tries to get the node ID of a child by the browse name.
    * It is therefore checked for each child of the specified node 
    * whether the browse name of the child matches the searched name. 
    * If so, the child's node id is returned. (stored in ndID)
    * 
    *  parameters: 
    *               -> UA_Client* client
    *               -> UA_Int16 nsStartNode                     /-> Namespace of the starting node (mother node)
    *               -> UA_Int16 idStartNode                     /-> Node id of the starting node (mother node)
    *               -> char* searchNodeName                     /-> Browse name of the searched node
    *               -> UA_Int16* ndID                           /-> Node id of the searched node
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode getChildNodeIdByString (UA_Client* client, UA_Int16 nsStartNode, UA_Int16 idStartNode,  char* searchNodeName, UA_Int16* ndID);

    /*
    * Function:  tagIdToExtentionObject
    * --------------------
    * Converts the id of a tag into a byte array and stores it in an extension object.
    *  parameters: 
    *               -> UA_String id                             /-> ID of the tag in the form of a string 
    *               -> UA_ExtensionObject* eo                   /-> Extension object in which the id is stored as a byte string 
    *               -> unsigned char sendBuffer[]               /-> Buffer pointer in which the id of the tag is stored as a byte array
    *               -> int sendBuffSize                         /-> Size of the sendBuffer
    * 
    *  returns: 
    *               -> int                                      /-> Errorcode -1 == Failed to translate hex numbers from id string; 0 == everything's OK
    */
    int tagIdToExtentionObject (UA_String id, UA_ExtensionObject* eo, unsigned char sendBuffer[], int sendBuffSize);

    /*
    * Function:  get_node_ids
    * --------------------
    * Searches the server for important node ids that will be used later.
    *  parameters: 
    *               -> UA_Client* client
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode get_node_ids(UA_Client* client);

    /*
    * Function:  get_namespace_index
    * --------------------
    * Searches the server for the namespaces index that will be used later.
    *  parameters: 
    *               -> UA_Client* client
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode get_namespace_index(UA_Client* client);

    /*
    * Function:  init 
    * --------------------
    * Initializes the program for communication with the Rfu6xx.
    * First, the indices of the namespaces are retrieved. 
    * After that, different node ids of important nodes are retrieved and saved.
    * 
    *  parameters: 
    *               -> UA_Client* client
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode init (UA_Client* client);

    /*
    * Function:  readLastScanData 
    * --------------------
    * Asks the RFU6xx server for the ID of the last code scanned and stores it in char* lastScanData
    *
    *  parameters: 
    *               -> UA_Client* client
    *               -> UA_String* lastScanData                  /-> Returns the ID of the last scanned tag
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode readLastScanData (UA_Client* client, UA_String* lastScanData);

    /*
    * Function:  readDeviceStatus 
    * --------------------
    * Asks the RFU6xx server current device status and stores it in char* lastScanData
    *
    *  parameters: 
    *               -> UA_Client* client
    *               -> UA_Int32* deviceStatus                   /-> Returns the device status
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode readDeviceStatus (UA_Client* client, UA_Int32* deviceStatus);

    /*
    * Function:  stopScan 
    * --------------------
    * Stops the Rfu6xx sensor for scanning
    *
    *  parameters: 
    *               -> UA_Client* client
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode stopScan (UA_Client* client);

    /*
    * Function:  startScan 
    * --------------------
    * Starts the Rfu6xx sensor for scanning
    *
    *  parameters: 
    *               -> UA_Client* client
    *               -> UA_Double duration                       /->
    *               -> UA_Int32 cycle                           /->
    *               -> UA_Bool dataAvailable                    /->
    * 
    *  returns: 
    *              -> UA_StatusCode
    */
    UA_StatusCode startScan (UA_Client* client, UA_Double duration, UA_Int32 cycle, UA_Boolean dataAvailable);

    /*
    * Function:  readTag 
    * --------------------
    * Reads data of a tag
    *
    *  parameters: 
    *               -> UA_Client* client
    *               -> UA_String id                             /-> Id string from the tag (coded in hex numbers)
    *               -> UA_Int32 bank                            /-> Bank from which the data is to be read
    *               -> UA_Int32 offset                          /-> Reading start offset
    *               -> UA_Int32 length                          /-> Number of bytes to be read
    *               -> UA_String* readData                      /-> Returns the data of the tag
    *               -> RFU6xx_StatusCode* serverResponseCode    /-> Status code returned from the rfu6xx server
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode readTag (UA_Client* client, UA_String id, UA_Int32 bank, UA_Int32 offset, UA_Int32 length, UA_String* readData, RFU6xx_StatusCode* serverResponseCode);

    /*
    * Function:  writeTag 
    * --------------------
    * Writes data to a tag
    *
    *  parameters: 
    *               -> UA_Client* client
    *               -> UA_String id                             /-> Id string from the tag (coded in hex numbers)
    *               -> UA_Int32 bank                            /-> Bank from which the data is to be read
    *               -> UA_Int32 offset                          /-> Reading start offset
    *               -> UA_String writeData                      /-> Data to be written
    *               -> RFU6xx_StatusCode* serverResponseCode    /-> Status code returned from the rfu6xx server
    * 
    *  returns: 
    *               -> UA_StatusCode
    */
    UA_StatusCode writeTag (UA_Client* client, UA_String id, UA_Int32 bank, UA_Int32 offset, UA_String writeData, RFU6xx_StatusCode* serverResponseCode);

#endif