#ifndef COMMONHEADERS_H
#define COMMONHEADERS_H

#define NAME_SERVER_PORT 8082

typedef enum {
   READ,
   WRITESYNC,
   WRITEASYNC,
   CREATEFILE,
   CREATEFOLDER,
   DELETEFILE,
   DELETEFOLDER,
   COPY,    
   LIST,
   INFO,
   STREAM,
   INITSS,
   ACK,
   ERROR
} requestType;

typedef enum{
    IP,
    PORT,
} ssFields;

typedef enum{
    PATH,
    DATA,
} nsFields;

#define MAX_IPOP_LENGTH 20
#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 1024
#define MAX_DATA_LENGTH 4096
#define MAX_STRUCT_LENGTH 5000

typedef struct {
    requestType requestType;
    char data[MAX_STRUCT_LENGTH];
} request;

typedef struct {
    char name[MAX_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
} client_request;


typedef struct{
  requestType requestType;
  char data[MAX_STRUCT_LENGTH];
  int clientID;
} processRequestStruct;

#endif
