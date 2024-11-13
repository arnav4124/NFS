#ifndef COMMONHEADERS_H
#define COMMONHEADERS_H
#define NAME_SERVER_PORT 8082
typedef enum {
   READ,
   WRITE,
   CREATE,
   DELETE,
   COPY,    
   LIST,
   STREAM,
   INITSS
} requestType;

typedef enum{
    IP,
    PORT,
} ssFields;

typedef enum{
    PATH,
    DATA,
} nsFields;

#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 1024
#define MAX_DATA_LENGTH 4096

typedef struct{
    requestType requestType;
    char data[MAX_DATA_LENGTH];
} request;

#endif
