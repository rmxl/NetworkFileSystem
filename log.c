#include "./commonheaders.h"

void logrecvfrom(char *from, char *ip, int port, char *buffer)
{
    FILE *fp = fopen("logs.txt", "a");
    if (fp == NULL)
    {
        perror("Failed to open log file");
        return; 
    }
    fprintf(fp, "\nResponse received from %s, ip: %s, port: %d\n", from, ip, port);
    fprintf(fp, "Data received: %s\n", buffer);
    fclose(fp);
}

void logsentto(char *to, char *ip, int port, char *buffer)
{
    FILE *fp = fopen("logs.txt", "a");
    if (fp == NULL)
    {
        perror("Failed to open log file");
        return; 
    }
    fprintf(fp, "\nReponse sent to %s, ip: %s, port: %d\n", to, ip, port);
    fprintf(fp, "Data sent: %s\n", buffer);
    fclose(fp);
}

void logerror(char *error)  {
   FILE *fp = fopen("logs.txt", "a");
   if (fp == NULL) {
       perror("Failed to open log file");
       return;
   }
  fprintf(fp, "\n%s\n", error);
}
