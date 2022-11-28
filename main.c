/*
* Created on 21.01.2022
*
* @author: Jakob Vollmer (DH-Student at SICK AG)
* @author: Sebastian Heidepriem (SICK AG)
* 
* @contact: sebastian.heidepriem@sick.de
*
*
* Function of the program:
*   1.) A connection is established with the RFU6xx server.
*   2.) The StartScan function of the RFU6xx is called up.
*   3.) The last scanned ID of a tag is read out.
*   4.) The StopScan function of the RFU6xx is called up.
*   5.) The WriteTag function is called and writes data at the last scanned tag.
*   5.) The ReadTag function is called and reads out the previously overwritten memory area.
*   6.) The connection is closed.
*/

#include "RFU6xxClient.h"
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdio.h>
#include <stdlib.h>

int abort_program (UA_Client* client, const char* abortMessage, int abortStatusCode)
{
    UA_Client_delete(client);
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, abortMessage, abortStatusCode);
    return abortStatusCode;
}

int main(int argc, char *argv[]) {
    // URL of the RFU6xx OPCUA server
    //char* serverUrl = "opc.tcp://ip:port"; // Default port is 4840
    char* serverUrl = "opc.tcp://";
    UA_StatusCode retval;
    RFU6xx_StatusCode serverResponseCode;

for (int i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);

    if (argc == 1) 
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "No ip address and no port number was specified. The standard value are used.");
        serverUrl = "opc.tcp://localhost:4840";
    } 
    else if (argc == 2)
    {
        char * urlBuff = (char *) malloc(1 + strlen(argv[1])+ strlen(serverUrl) );
        strcpy(urlBuff, serverUrl);
        strcat(urlBuff, argv[1]);
        serverUrl = urlBuff;
    }
    else 
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Too many parameters received.");
        return UA_STATUSCODE_BAD;
    }

    // Create client and set default configuration settings
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    // Connect to server
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Client connect to server: %s", serverUrl);
    retval = UA_Client_connect(client, serverUrl);
    if(retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Failed to connect to server. ErrorCode: %x", (int)retval);
    }

    // Init client
    retval = init(client);
    if(retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Init Failed. ErrorCode: %x", (int)retval);
    }  

    // Call methode StartScan    
    retval = startScan(client, 0, 0, false);
    if(retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Methode call StartScan failed. ErrorCode: %x", (int)retval);
    }

    // Read last scan data
    UA_String lastScanData;
    retval = readLastScanData(client, &lastScanData);
    if(retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Read Last data Failed. ErrorCode: %x", (int)retval);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Last scan data: %.*s", (int) lastScanData.length, lastScanData.data);
    
    // Call methode StopScan
    retval = stopScan(client);
    if(retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Methode call StopScan failed. ErrorCode: %x", (int)retval);
    }

    // Call methode WriteTag
    UA_String tagWriteData = UA_String_fromChars("affedeafbeadaffe");
    retval = writeTag(client, lastScanData, 3, 0, tagWriteData, &serverResponseCode);    
    if(retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Methode call WriteTag failed. ErrorCode: %x", (int)retval);
    }
    if (serverResponseCode != RFU6xx_STATUSCODE_SUCCESS)
    {
        return abort_program(client, "Something went wrong during the WriteTag process. ServerResponse: %i", (int)retval);
    }

    // Call methode ReadTag
    UA_String tagReadData;
    retval = readTag(client, lastScanData, 3, 0, 16, &tagReadData, &serverResponseCode);    
    if (retval != UA_STATUSCODE_GOOD) 
    {
        return abort_program(client, "Methode call ReadTag failed. ErrorCode: %x", (int)retval);
    }
    if (serverResponseCode != RFU6xx_STATUSCODE_SUCCESS)
    {
        return abort_program(client, "Something went wrong during the ReadTag process. ServerResponse: %i", (int)retval);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Tag read data: %.*s",(int) tagReadData.length, tagReadData.data);

    // Close connection and leave program
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
