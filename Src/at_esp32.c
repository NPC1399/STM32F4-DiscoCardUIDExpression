#include <at_esp32.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart1;
void espCheck() 
{
    /*
        This function sends check command to ESP32 ("AT")
     */
    char command[] = "AT\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*) "Trying to connect to ESP32...\r\n", sizeof("Trying to connect to ESP32...\r\n") - 1, 100);
    HAL_UART_Transmit(&huart5, (uint8_t*) command, sizeof(command) - 1, 100);
}

void resetEsp() 
{
    /*
        This function resets the esp32 module in case normal operation fails or watchdog was triggered
     */
    HAL_UART_Transmit(&huart1, (uint8_t*) "Resetting ESP32...\r\n", sizeof("Resetting ESP32...\r\n") - 1, 100);
    HAL_UART_Transmit(&huart5, (uint8_t*) "AT+RST\r\n", sizeof("AT+RST\r\n") - 1, 100);
}

void configEspEcho(int mode)
{
    /*
        This function configs the ESP32 echo configuration.
        params:
            mode = 1: turn on echo
            mode = 0: turn off echo
     */
    if (mode == 1) {
        // turn on echo
        HAL_UART_Transmit(&huart1, (uint8_t*) "Turning on echo...\r\n", sizeof("Turning on echo...\r\n") - 1, 100);
        HAL_UART_Transmit(&huart5, (uint8_t*) "ATE1\r\n", 6, 100);
    }
    else if (mode == 0) {
        // turn off echo
        HAL_UART_Transmit(&huart1, (uint8_t*) "Turning off echo...\r\n", sizeof("Turning off echo...\r\n") - 1, 100);
        HAL_UART_Transmit(&huart5, (uint8_t*) "ATE0\r\n", 6, 100);
    }
    else {
        HAL_UART_Transmit(&huart1, (uint8_t*) "ConfigEcho bad argument.\r\n", sizeof("ConfigEcho bad argument.\r\n") - 1, 100);
    }
}

void setWifiMode(int mode)
{
    /*
        This function sets ESP module WiFi mode.
        params :
            mode = 0: Null, WiFi RF will be disabled.
            mode = 1: Station Mode
            mode = 2: SoftAP Mode
            mode = 3: Station + SoftAp Mode 
     */

    char temp[13] = "AT+CWMODE=1\r\n"; // default mode is Station Mode
    if (mode == 0) {
        temp[10] = '0';
        HAL_UART_Transmit(&huart1, (uint8_t*) "Turning off Wi-Fi RF...\r\n", sizeof("Turning off Wi-Fi RF...\r\n") - 1, 100);
    }
    else if (mode == 1) {
        temp[10] = '1';
        HAL_UART_Transmit(&huart1, (uint8_t*) "Setting Wi-Fi to Station Mode...\r\n", sizeof("Setting Wi-Fi to Station Mode...\r\n") - 1, 100);
    }
    else if (mode == 2) {
        temp[10] = '2';
        HAL_UART_Transmit(&huart1, (uint8_t*) "Setting Wi-Fi to SoftAP Mode...\r\n", sizeof("Setting Wi-Fi to SoftAP Mode...\r\n") - 1, 100);
    }
    else if (mode == 3) {
        temp[10] = '3';
        HAL_UART_Transmit(&huart1, (uint8_t*) "Setting Wi-Fi to Station+SoftAP Mode...\r\n", sizeof("Setting Wi-Fi to Station+SoftAP Mode...\r\n") - 1, 100);
    }
    else {
        HAL_UART_Transmit(&huart1, (uint8_t*) "setWifiMode bad argument. Setting Wi-Fi Mode to Station Mode...\r\n", sizeof("setWifiMode bad argument. Setting Wi-Fi Mode to Station Mode...\r\n") - 1, 100);
    }
    HAL_UART_Transmit(&huart5, (uint8_t*) temp, 13, 100);

}

void checkWifiStatus() {
    HAL_UART_Transmit(&huart5, (uint8_t*) "AT+CWJAP?\r\n", sizeof("AT+CWJAP?\r\n") - 1, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Checking wifi connection\r\n", sizeof("Checking wifi connection\r\n") - 1, 100);
}

void connectToWifi(char ssid[], char password[]) 
{
    /*
        This function sends connect to Wi-Fi command to ESP32
     */
    char temp[80] = "AT+CWJAP=\"";
    strcat(temp, ssid);
    strcat(temp, "\",\"");
    strcat(temp, password);
    strcat(temp, "\"\r\n");
    
    uint8_t command_size = 16 + strlen(ssid) + strlen(password);

    HAL_UART_Transmit(&huart5, (uint8_t*) temp, command_size, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Connecting to an AP...\r\n", sizeof("Connecting to an AP...\r\n") - 1, 100);
}

void disconnectWifi()
{
    HAL_UART_Transmit(&huart5, (uint8_t*) "AT+CWQAP\r\n", sizeof("AT+CWQAP\r\n") - 1, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Disconnecting from Wi-Fi\r\n", sizeof("Disconnecting from Wi-Fi\r\n") - 1, 100);
}

void autoConnectWifi(int mode)
{
    char temp[17] = "AT+CWAUTOCONN=1\r\n";

    if (mode == 0) {
        temp[14] = '0';
    }
    else {
        temp[14] = '1';
    }

    HAL_UART_Transmit(&huart5, (uint8_t*) temp, sizeof(temp) - 1, 100);
}

void connectToTCP(char remoteIP[], char port[], char keepAlive[])
{
    /*
        This function establishes a TCP connection to a remote server.
        params:
            remoteIP[]: IP address of a remote server
            port[]: Port of a remote server
            keepAlive[]: Amount of time (seconds) that will keep the connection alive ranging from 0 to 7200.
                       If keepAlive = 0 then keepAlive function will be disabled.
     */
    char command[80] = "AT+CIPSTART=\"TCP\",\"";
    strcat(command, remoteIP);
    strcat(command, "\",");
    strcat(command, port);
    strcat(command, ",");
    strcat(command, keepAlive);
    strcat(command, "\r\n");

    HAL_UART_Transmit(&huart5, (uint8_t*) command, strlen(command), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Establishing TCP connection...\r\n", sizeof("Establishing TCP connection...\r\n") - 1, 100);
}

void closeTCP(){
    HAL_UART_Transmit(&huart5, (uint8_t*) "AT+CIPCLOSE\r\n", sizeof("AT+CIPCLOSE\r\n") - 1, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Closing TCP connection...\r\n", sizeof("Closing TCP connection...\r\n") - 1, 100);
}
void publishdata(char msg[],char topic[],int Type_Flag,int topic_len){
    int msg_len;
    char TypeMsg[100] = {'x',':'};
    switch(Type_Flag){
        case 0 : //Can't Detect
                msg_len = 2;
                TypeMsg[0] = '0';
                break;
        case 1 : //ISO14443A
                TypeMsg[0] = '1';
                strcat(TypeMsg,msg);
                msg_len = strlen(TypeMsg)+1;
                break;
        case 2 : //ISO14443B
                TypeMsg[0] = '2';
                strcat(TypeMsg,msg);
                msg_len = strlen(TypeMsg)+1;
                break;
        case 3 : //ISO15693
                TypeMsg[0] = '3';
                strcat(TypeMsg,msg);
                msg_len = strlen(TypeMsg)+1;
                break;
        default :
                break;
    }
    uint8_t data[99] = {0x32,2+topic_len+msg_len,0,topic_len-1}; //size of string = num of char + null so we use 2 instead 4 to plus with topic_len and msg_len
    data[3+topic_len] = 0;
    data[4+topic_len] = 1;
    int i = 0; 
    for(i=0;i<sizeof(data);i++){
        if(i<topic_len)
            data[i+4] = topic[i];
        if(i<msg_len)
            data[i+5+topic_len] = TypeMsg[i];
        if(i>=topic_len&&i>=msg_len)
            break;
    }
    int count = 0;
    int countnull = 0;
    while(count<sizeof(data)){
        if(data[count]==NULL)
            countnull++;
        if(countnull==3)
            break;
        count++;
    }
    char DataSize[2];
    sprintf(DataSize,"%d",count);
    uint8_t ReqCom[15] = {'A','T','+','C','I','P','S','E','N','D','='};
    if(count<10){
        ReqCom[11]=DataSize[0];
        ReqCom[12]='\r';
        ReqCom[13]='\n';
    }
    else{
        ReqCom[11]=DataSize[0];
        ReqCom[12]=DataSize[1];
        ReqCom[13]='\r';
        ReqCom[14]='\n';
    }
    //Send Request
    HAL_UART_Transmit(&huart5,ReqCom,sizeof(ReqCom),100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Request sending data...\r\n", sizeof("Request sending data...\r\n") - 1, 100);
    //Send Data
    HAL_UART_Transmit(&huart1, (uint8_t*) "Transmitting Data...\r\n", sizeof("Transmitting Data..\r\n") - 1, 100);
    HAL_UART_Transmit(&huart5,data,count,100);
}
void tcpSendDataRequest(char length[])
{
    /*
        This function sends request sending command to ESP32 specify by data length
        params :
            length: length of data sent  
     */

    char command[20] = "AT+CIPSEND=";
    strcat(command, length);
    strcat(command ,"\r\n");

    HAL_UART_Transmit(&huart5, (uint8_t*) command, strlen(command), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Request sending data...\r\n", sizeof("Request sending data...\r\n") - 1, 100);
}

void tcpSendData(char data[], int size)
{
    /*
        This function sends data through TCP connection.
        ( Must be used after sendRequest function )
        params:
            data: a stream of data
     */

    HAL_UART_Transmit(&huart1, (uint8_t*) "Transmitting Data...\r\n", sizeof("Transmitting Data...\r\n") - 1, 100);
    for (int i = 0; i < size; ++i) {
        HAL_UART_Transmit(&huart5, (uint8_t*) &data[i], 1, 100);
    }
}

void ping(char ip[]) 
{
    /*
        This function pings the target ip.
     */

    char command[80] = "AT+PING=\"";
    strcat(command, ip);
    strcat(command, "\"\r\n");

    HAL_UART_Transmit(&huart1, (uint8_t*) "Sending a ping packet...\r\n", sizeof("Sending a ping packet...\r\n") - 1, 100);
    HAL_UART_Transmit(&huart5, (uint8_t*) command, strlen(command), 100);
} 
