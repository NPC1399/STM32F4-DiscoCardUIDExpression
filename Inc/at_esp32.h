#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"

enum State {
    ESP_OFFLINE,
    ESP_ALIVE,
    WIFI_CONNECTED,
    TCP_CONNECTED,
    QUIT_AP,
    PUBLISHER,
    SUBSCRIBER,
    QUIT_TCP,
    TEST_STATE
};

void espCheck();
void resetEsp();
void configEspEcho(int mode);
void setWifiMode(int mode);
void checkWifiStatus();
void connectToWifi(char ssid[], char password[]);
void disconnectWifi();
void autoConnectWifi(int mode);
void connectToTCP(char remoteIP[], char port[], char keepAlive[]);
void closeTCP();
void publishdata(char msg[],char topic[],int Type_Flag,int topic_len);
void tcpSendDataRequest(char length[]);
void tcpSendData(char data[], int size);
void ping(char ip[]);
