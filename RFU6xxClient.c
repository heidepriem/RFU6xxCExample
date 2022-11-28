/*
* Created on 21.01.2022
*
* @author: Jakob Vollmer (DH-Student at SICK AG)
* @author: Sebastian Heidepriem (SICK AG)
* 
* @contact: sebastian.heidepriem@sick.de
*/

#include "RFU6xxClient.h"
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

// ------------------------------------------------------------------------------------------------------------------------

void serialize32Bit(char** pBit, unsigned int value) 
{
	(*(*pBit)++) =  value        & 0xFF;
    (*(*pBit)++) = (value >>  8) & 0xFF;
    (*(*pBit)++) = (value >> 16) & 0xFF;
    (*(*pBit)++) = (value >> 24) & 0xFF;
}

void serialize64Bit(char** pBit, unsigned long value) 
{   
	(*(*pBit)++) =  value        & 0xFF; 
    (*(*pBit)++) = (value >>  8) & 0xFF;
    (*(*pBit)++) = (value >> 16) & 0xFF;
    (*(*pBit)++) = (value >> 24) & 0xFF;
    (*(*pBit)++) = (value >> 32) & 0xFF;
    (*(*pBit)++) = (value >> 40) & 0xFF;
    (*(*pBit)++) = (value >> 48) & 0xFF;
    (*(*pBit)++) = (value >> 56) & 0xFF;
}

// ------------------------------------------------------------------------------------------------------------------------

uint64_t encodeDouble(double value)
{
    union {double d; uint64_t i;}nbr;
    nbr.d = value;
    return nbr.i;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode getChildNodeIdByString (UA_Client* client, UA_Int16 nsStartNode, UA_Int16 idStartNode,  char* searchNodeName, UA_Int16* ndID)
{
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(nsStartNode, idStartNode);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    
    for(size_t i = 0; i < bResp.resultsSize; ++i) {
        for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                if (strcmp(ref->browseName.name.data, searchNodeName) == 0)
                {
                    *ndID = ref->nodeId.nodeId.identifier.numeric;
                    return UA_STATUSCODE_GOOD;
                }
            }
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

// ------------------------------------------------------------------------------------------------------------------------

int tagIdToExtentionObject (UA_String id, UA_ExtensionObject* eo, unsigned char sendBuffer[], int sendBuffSize) 
{
    int idLength = id.length;
    char* pSendBuffer = sendBuffer;

    // Convert UA_String id to char* idString
    char* idString = (char*)UA_malloc(sizeof(char)*id.length+1);
    memcpy(idString, id.data, id.length );
    idString[id.length] = '\0';

    // Convert char* idString to unsigned char sendBuffer
    // -> converts a hex string into bytes
    const char *pos = idString;
    for (int count = 0; count < (id.length/2); count++) {
        int res = sscanf(pos, "%2hhx", &sendBuffer[count + 8]);
        if (res != 1){
            return -1;
        }
        pos += 2;
    }
    free(idString);
	
    serialize32Bit(&pSendBuffer, 2);
    serialize32Bit(&pSendBuffer, (idLength/2));

    eo->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo->content.encoded.typeId = UA_NODEID_NUMERIC(nsAutoID, RFU6xx_TAG_ID_E_O_TYPE_ID);
    eo->content.encoded.body.data = (UA_Byte*)sendBuffer;
    eo->content.encoded.body.length = sendBuffSize;
    return 0;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode get_node_ids(UA_Client* client)
{
    UA_StatusCode retval;

    // Search node ID for DeviceSet node (=> is child of root)
    retval = getChildNodeIdByString(client, 0, UA_NS0ID_OBJECTSFOLDER, "DeviceSet", &ndDeviceSetID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for DeviceSet");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id DeviceSet: %i", ndDeviceSetID);

    // Search node ID for RFU6xx node (=> is child of deviceSet)
    retval = getChildNodeIdByString(client, 2, ndDeviceSetID, "RFU6xx", &ndRfu6xxNodeID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for Rfu6xx");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id DeviceSet: %i", ndRfu6xxNodeID);

    // Search node ID for LastScanData node (=> is child of RFU6xx)
    retval = getChildNodeIdByString(client, nsRfu, ndRfu6xxNodeID, "LastScanData", &ndLastScanDataID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for LastScanData");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id LastScanData: %i", ndLastScanDataID);

    // Search node ID for WriteTag node (=> is child of RFU6xx)
    retval = getChildNodeIdByString(client, nsRfu, ndRfu6xxNodeID, "WriteTag", &ndWriteTagID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for WriteTag");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id WriteTag: %i", ndWriteTagID);

    // Search node ID for ReadTag node (=> is child of RFU6xx)
    retval = getChildNodeIdByString(client, nsRfu, ndRfu6xxNodeID, "ReadTag", &ndReadTagID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for ReadTag");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id ReadTag: %i", ndReadTagID);

    // Search node ID for ScanStart node (=> is child of RFU6xx)
    retval = getChildNodeIdByString(client, nsRfu, ndRfu6xxNodeID, "ScanStart", &ndScanStartID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for ScanStart");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id ScanStart: %i", ndScanStartID);

    // Search node ID for ScanStop node (=> is child of RFU6xx)
    retval = getChildNodeIdByString(client, nsRfu, ndRfu6xxNodeID, "ScanStop", &ndScanStopID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for ScanStop");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id ScanStop: %i", ndScanStopID);

    // Search node ID for DeviceStatus node (=> is child of RFU6xx)
    retval = getChildNodeIdByString(client, nsRfu, ndRfu6xxNodeID, "DeviceStatus", &ndDeviceStatusID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find node id for DeviceStatus");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node id DeviceStatus: %i", ndDeviceStatusID);

    return UA_STATUSCODE_GOOD;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode get_namespace_index(UA_Client* client)
{
    UA_StatusCode retval;

    // Search namespace index for "http://opcfoundation.org/UA/AutoID/"
    UA_String nsAutoIDStr = UA_String_fromChars("http://opcfoundation.org/UA/AutoID/");
    retval = UA_Client_NamespaceGetIndex(client, &nsAutoIDStr, &nsAutoID);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find namespace index for Auto ID");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Namespace index AutoID: %i", nsAutoID);
    
    // Search namespace index for "http://opcfoundation.org/UA/DI/"
    UA_String nsOpcDIStr = UA_String_fromChars("http://opcfoundation.org/UA/DI/");
    retval = UA_Client_NamespaceGetIndex(client, &nsOpcDIStr, &nsOpcDI);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find namespace index for Opc DI");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Namespace index OpcDI: %i", nsOpcDI);

    // Search namespace index for "http://www.sick.com/RFU6xx/"
    UA_String nsRfuStr = UA_String_fromChars("http://www.sick.com/RFU6xx/");
    retval = UA_Client_NamespaceGetIndex(client, &nsRfuStr, &nsRfu);
    if(retval != UA_STATUSCODE_GOOD) 
    { 
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Init failed could not find namespace index for Rfu ID");
        return retval; 
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Namespace index Rfu: %i", nsRfu);

    return UA_STATUSCODE_GOOD;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode init (UA_Client* client) 
{
    UA_StatusCode retval;
    
    retval = get_namespace_index(client);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    retval = get_node_ids(client);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    return UA_STATUSCODE_GOOD;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode readLastScanData (UA_Client* client, UA_String* lastScanData) 
{
    UA_Variant readData;
    UA_StatusCode retval;

    // Read value
    retval = UA_Client_readValueAttribute(client, 
        UA_NODEID_NUMERIC(nsRfu, ndLastScanDataID), 
        &readData);

    // Check if read was successfully
    if(retval == UA_STATUSCODE_GOOD)
    {
        // Check if value type is string
        if (UA_Variant_hasScalarType(&readData, &UA_TYPES[UA_TYPES_STRING])) 
        {
            *lastScanData = *(UA_String *) readData.data;
        } 
        else 
        {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }   
    return retval;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode readDeviceStatus (UA_Client* client, UA_Int32* deviceStatus) 
{
    UA_Variant readData;
    UA_StatusCode retval;

    // Read value
    retval = UA_Client_readValueAttribute(client, 
        UA_NODEID_NUMERIC(nsRfu, ndDeviceStatusID), 
        &readData);

    // Check if read was successfully
    if(retval == UA_STATUSCODE_GOOD)
    {
        // Check if value type is string
        if (UA_Variant_hasScalarType(&readData, &UA_TYPES[UA_TYPES_INT32])) 
        {
            *deviceStatus = *(UA_Int32 *) readData.data;
        } 
        else 
        {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }   
    return retval;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode stopScan (UA_Client* client)
{
    RFU6xx_DeviceStatusCode deviceStatus;
    UA_StatusCode retval = readDeviceStatus(client, &deviceStatus);

    if (retval != UA_STATUSCODE_GOOD)
    {
        return retval;
    } 
    else if (deviceStatus != RFU6xx_DEVICESTATUSCODE_SCANNING)
    {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "The stop scan function was called, but the device is in the status:: %i", deviceStatus);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    // Call StartScan methode with no params
    return UA_Client_call(client, 
        UA_NODEID_NUMERIC(nsRfu, ndRfu6xxNodeID),
        UA_NODEID_NUMERIC(nsRfu, ndScanStopID),
        0 , NULL, NULL, NULL);
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode startScan (UA_Client* client, UA_Double duration, UA_Int32 cycle, UA_Boolean dataAvailable)
{
    size_t sendParamsSize = 1;
    UA_Variant sendParams[sendParamsSize];

    size_t retParamsSize;
    UA_Variant* retParams;

    UA_ExtensionObject eo;

    int sendBuffSize = sizeof(UA_Int32)*2 + sizeof(UA_Int64) + sizeof(UA_Boolean);
    char sendBuffer[sendBuffSize];
	char* pSendBuffer = sendBuffer;

    RFU6xx_DeviceStatusCode deviceStatus;
    UA_StatusCode retval = readDeviceStatus(client, &deviceStatus);

    if (retval != UA_STATUSCODE_GOOD)
    {
        return retval;
    } 
    else if (deviceStatus != RFU6xx_DEVICESTATUSCODE_IDLE)
    {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "The start scan function was called, but the device is in the status:: %i", deviceStatus);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    // Serialize parameters to byte array
    serialize32Bit(&pSendBuffer, (unsigned int) 0);
    serialize64Bit(&pSendBuffer, (unsigned long) encodeDouble(duration));    
    serialize32Bit(&pSendBuffer, (unsigned int) cycle);
    (*pSendBuffer++) = dataAvailable && 0xFF;

    // Convert byte array to extension object
    eo.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo.content.encoded.typeId = UA_NODEID_NUMERIC(nsAutoID, RFU6xx_START_SCAN_E_O_TYPE_ID);
    eo.content.encoded.body.data = (UA_Byte*)sendBuffer;
    eo.content.encoded.body.length = sendBuffSize;
    UA_Variant_setScalarCopy(sendParams, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);        

    // Call StartScan methode with params
    retval = UA_Client_call(client, 
        UA_NODEID_NUMERIC(nsRfu, ndRfu6xxNodeID),
        UA_NODEID_NUMERIC(nsRfu, ndScanStartID), 
        sendParamsSize, sendParams, &retParamsSize, &retParams);

    return retval;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode readTag (UA_Client* client, UA_String id, UA_Int32 bank, UA_Int32 offset, 
    UA_Int32 length, UA_String* readData, RFU6xx_StatusCode* serverResponseCode)
{
    size_t sendParamsSize = 6;
    UA_Variant sendParams[sendParamsSize];

    size_t retParamsSize;
    UA_Variant* retParams;

    UA_ExtensionObject eo;

    UA_String codeType = UA_STRING("RAW:STRING");
    UA_String password = UA_STRING("");

    int idLength = id.length;
    int sendBuffSize = (idLength/2) + 2*sizeof(UA_Int32);
    unsigned char sendBuffer[sendBuffSize];
    if (tagIdToExtentionObject(id, &eo, sendBuffer, sendBuffSize) != 0)
    {
        return UA_STATUSCODE_BAD;
    }
    
    UA_Variant_setScalarCopy(&sendParams[0], &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);    
    UA_Variant_setScalarCopy(&sendParams[1], &codeType, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&sendParams[2], (UA_Int16*) &bank, &UA_TYPES[UA_TYPES_INT16]);
    UA_Variant_setScalarCopy(&sendParams[3], &offset, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalarCopy(&sendParams[4], &length, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalarCopy(&sendParams[5], &password, &UA_TYPES[UA_TYPES_STRING]);
    
    UA_StatusCode retval = UA_Client_call(client, 
        UA_NODEID_NUMERIC(nsRfu, ndRfu6xxNodeID),
        UA_NODEID_NUMERIC(nsRfu, ndReadTagID), 
        sendParamsSize, sendParams, &retParamsSize, &retParams);

    // Check if read was successfully
    if(retval == UA_STATUSCODE_GOOD)
    {
        // Check if response has 2 parameters and the types are correct
        if (retParamsSize == 2
            && UA_Variant_hasScalarType(&retParams[0], &UA_TYPES[UA_TYPES_BYTESTRING]) 
            && UA_Variant_hasScalarType(&retParams[1], &UA_TYPES[UA_TYPES_INT32])) 
        {
            *readData = *(UA_String *) retParams[0].data;
            *serverResponseCode = *(RFU6xx_StatusCode *) retParams[1].data;
        } 
        else 
        {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }   
    return retval;
}

// ------------------------------------------------------------------------------------------------------------------------

UA_StatusCode writeTag (UA_Client* client, UA_String id, UA_Int32 bank, UA_Int32 offset, 
    UA_String writeData, RFU6xx_StatusCode* serverResponseCode)
{
    size_t sendParamsSize = 6;
    UA_Variant sendParams[sendParamsSize];

    size_t retParamsSize;
    UA_Variant* retParams;

    UA_ExtensionObject eo;

    UA_String codeType = UA_STRING("RAW:STRING");
    UA_String password = UA_STRING("");

    int idLength = id.length;
    int sendBuffSize = (idLength/2) + 2*sizeof(UA_Int32);
    unsigned char sendBuffer[sendBuffSize];
    if (tagIdToExtentionObject(id, &eo, sendBuffer, sendBuffSize) != 0)
    {
        return UA_STATUSCODE_BAD;
    }

    UA_Variant_setScalarCopy(&sendParams[0], &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);    
    UA_Variant_setScalarCopy(&sendParams[1], &codeType, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&sendParams[2], (UA_Int16*) &bank, &UA_TYPES[UA_TYPES_INT16]);
    UA_Variant_setScalarCopy(&sendParams[3], &offset, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalarCopy(&sendParams[4], &writeData, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&sendParams[5], &password, &UA_TYPES[UA_TYPES_STRING]);
    
    UA_StatusCode retval = UA_Client_call(client, 
        UA_NODEID_NUMERIC(nsRfu, ndRfu6xxNodeID),
        UA_NODEID_NUMERIC(nsRfu, ndWriteTagID), 
        sendParamsSize, sendParams, &retParamsSize, &retParams);
 
     // Check if write was successfully
    if(retval == UA_STATUSCODE_GOOD)
    {
        // Check if response has 1 parameters and the type is correct
        if (retParamsSize == 1 && UA_Variant_hasScalarType(&retParams[0], &UA_TYPES[UA_TYPES_INT32])) 
        {
            *serverResponseCode = *(RFU6xx_StatusCode *) retParams[0].data;
        } 
        else 
        {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }   

    return retval;
}
