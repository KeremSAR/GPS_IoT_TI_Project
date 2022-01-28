#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <time.h>

// XDCtools Header files
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/std.h>
/* TI-RTOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Idle.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/net/http/httpcli.h>

#include <ti/sysbios/hal/Seconds.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/net/http/httpcli.h>
#include "driverlib/UART.h"
#include <ti/net/sntp/sntp.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "Board.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <ti/net/socket.h>
#include <ti/ndk/nettools/sntp/sntp.h>

#define TASKSTACKSIZE     5000
//#define NTP_PORT            37
#define SERVERPORT        5000
#define SOCKETTEST_IP   "128.138.140.44" //https://tf.nist.gov/tf-cgi/servers.cgi // NIST, Boulder, Colorado
//128.138.140.44
//132.163.96.6
unsigned long int timestamp;
char currentTime[120];

int year =1900, month, day, hour, minutes, seconds;
int ref;

//struct tm  *ts;
//struct tm  *tim;
#define NTP_HOSTNAME     "pool.ntp.org"
#define NTP_PORT         "123"
#define NTP_SERVERS      3
#define NTP_SERVERS_SIZE (NTP_SERVERS * sizeof(struct sockaddr_in))

uint8_t ca[] =
    "<--- add root certificate with blackslash at end of each new line -->";
uint32_t calen = sizeof(ca);
unsigned char ntpServers[NTP_SERVERS_SIZE];




extern Swi_Handle swi0;
extern Swi_Handle swi1;
extern Semaphore_Handle semaphore0;
extern Semaphore_Handle semaphore2;
extern Semaphore_Handle semaphore3;
extern Event_Handle event0;
extern Mailbox_Handle mailbox0;

char  GPSValues[100],latitudeResult[40],longitudeResult[40],*token,Date[9];
char c0;
const char parseValue[12][20];


void printError(char *errString, int code)
{
    System_printf("Error! code = %d, desc = %s\n", code, errString);
    BIOS_exit(code);
}
Void timerISR(UArg arg1){


        Swi_post(swi1);


}

Void swifunc(UArg arg1){
    seconds++;
    Event_post(event0,Event_Id_00);  // to send values every seconds
     if(seconds==60){

      seconds=0;
      minutes++;
    }
     if(minutes==60){
      minutes=0;
      hour++;
    }



    /*System_printf("Date: %d-%lu-%d Time: %02d:%02d:%02d\n", day,month,year,hour,minutes ,seconds);
     System_flush();
*/

}
Void swi1_func(UArg arg1,UArg arg2){
    ref++;
    if (ref ==10) {             // fix time every 10 seconds.
            Swi_post(swi0);
            Semaphore_post(semaphore3);  // posts to SNTP
            ref=0;
        }
    /*if (year==1900 ||year == 0) {
        Swi_post(swi0);
        Semaphore_post(semaphore0);

    }*/
    else {
        Swi_post(swi0);
      //  Semaphore_post(semaphore1);
    }

}

void GPS_Init(void)
{


    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA6_U2RX);        // PA6 RX ----> Tx
    GPIOPinConfigure(GPIO_PA7_U2TX);        // PA7 TX ----> RX
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    UARTEnable(UART2_BASE);

    UART_Handle uart;
    UART_Params uartParams;
    UART_Params_init(&uartParams);
   // uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.dataLength = UART_LEN_8;
    uartParams.parityType = UART_PAR_NONE;
    uartParams.stopBits = UART_STOP_ONE;
    uartParams.baudRate = 9600;
    uart = UART_open(Board_UART2, &uartParams);

    if (uart == NULL) {
            // something terribly went wrong. abort!!!
            //
            System_abort("Error opening the UART");
        }

}


/*GPS task that recv Latitude and Longitude values from the GPS*/
Void GPS_Location(UArg arg1, UArg arg2){

    Semaphore_pend(semaphore2, BIOS_WAIT_FOREVER);   // waits to start SNTP

    int index=0,degrees;
    double latitude=0.0,longitude=0.0,seconds=0.0,converted_result=0.0;
    char Location[120];
    const char comma[2] = ",";
    while(1){
                                                /*Returns true if there is data in the receive FIFO or false if there is no data in the receive FIFO.*/
        while(!UARTCharsAvail(UART2_BASE));     // Determines if there are any characters in the receive FIFO.
         c0=UARTCharGetNonBlocking(UART2_BASE); // reads the character and puts into buffer for scanning
        //Is data $GPRMC ?
        if(c0=='$'){
            while(!UARTCharsAvail(UART2_BASE));
            char c1=UARTCharGetNonBlocking(UART2_BASE);
            if(c1=='G'){
                while(!UARTCharsAvail(UART2_BASE));
                char c2=UARTCharGetNonBlocking(UART2_BASE);
                if(c2=='P'){
                    while(!UARTCharsAvail(UART2_BASE));
                    char c3=UARTCharGetNonBlocking(UART2_BASE);
                    if(c3=='R'){
                        while(!UARTCharsAvail(UART2_BASE));
                        char c4=UARTCharGetNonBlocking(UART2_BASE);
                        if(c4=='M'){
                            while(!UARTCharsAvail(UART2_BASE));
                            char c5=UARTCharGetNonBlocking(UART2_BASE);
                            if(c5=='C'){
                                while(!UARTCharsAvail(UART2_BASE));
                                char c6=UARTCharGetNonBlocking(UART2_BASE);
                                if(c6==','){
                                    while(!UARTCharsAvail(UART2_BASE));
                                    char c7=UARTCharGetNonBlocking(UART2_BASE);
                                    // Read GPS value until *
                                    while(c7!='*'){
                                        GPSValues[index]=c7;
                                        while(!UARTCharsAvail(UART2_BASE));
                                        c7=UARTCharGetNonBlocking(UART2_BASE);
                                        index++;}


                                    //Parse the values between commas.
                                    index=0;
                                    token = strtok(GPSValues, comma);
                                    while( token != NULL ) {
                                        strcpy(parseValue[index], token);
                                        token = strtok(NULL, comma);
                                        index++;}

                                       //A = data valid or V = data not valid
                                    if (strcmp(parseValue[1],"A")==0) {

              //$GPRMC, 161229.487, A, 3723.2475, N, 12158.3416, W, 0.13, 309.62, 120598, , *10

                                    /* DDMM.MMM format to DD   ---->> degree + seconds
                                     * DD is degree  it is a integer value
                                     * MM.MMM is minutes
                                     * */
                                       // System_printf("saat %s \n",parseValue[0]);
                                        strcpy(latitudeResult, parseValue[2]);
                                        latitude= atof(latitudeResult);       // float value
                                        degrees = latitude/100;               // DD  degree
                                        seconds =(latitude-(degrees*100))/60; //  MM.MMM to seconds
                                        converted_result = degrees+seconds;             // decimal format of latitude
                                        sprintf(latitudeResult,"%f", converted_result);
                                        System_printf("latitude %s\n",latitudeResult );
                                        System_flush();


                                        strcpy(longitudeResult, parseValue[4]);
                                        longitude = atof(longitudeResult);  // float value
                                        degrees = longitude/100;            // DD degree
                                        seconds = (longitude-(degrees*100))/60;     // MM.MMM to seconds
                                        converted_result = degrees+seconds;           // decimal format of longitude
                                        sprintf(longitudeResult,"%f" ,converted_result);
                                        System_printf("longitude %s\n",longitudeResult);
                                        System_flush();

                                        /* latitude and longtitude value as a same string to send a Server*/
                                        strcpy(Location, "latitude: ");
                                        strcat(Location, latitudeResult);
                                        strcat(Location, " ");
                                        strcat(Location,parseValue[3]);
                                        strcat(Location, " - longitude: ");
                                        strcat(Location, longitudeResult);
                                        strcat(Location, " ");
                                        strcat(Location,parseValue[5]);

                                        /* To send Latitude and longitude value every seconds*/
                                        Event_pend(event0,Event_Id_00,Event_Id_NONE,BIOS_WAIT_FOREVER);  // waits 1 seconds
                                        Mailbox_post(mailbox0, &Location, BIOS_NO_WAIT);   // post location value to client task
                                        Semaphore_post(semaphore0); // post to GpsDate task
                                        System_flush();

                                    }


                                    else{
                                        char NotFixed[80] ="00.000000 , 00.000000";
                                        /* To send Latitude and longitude value every seconds*/
                                        Event_pend(event0,Event_Id_00,Event_Id_NONE,BIOS_WAIT_FOREVER); // waits 1 seconds
                                        Mailbox_post(mailbox0, &NotFixed, BIOS_NO_WAIT);  // post notFixed value to client task

                                        System_printf("  GPS NOT FIXED \n\n");
                                    }

                                        System_flush();

                            }}}}}}}


    }
}

Void GpsDate(UArg arg1, UArg arg2){
    int index1=0,i=0;

    while(1){
       Semaphore_pend(semaphore0,BIOS_WAIT_FOREVER);   // waits location values for 1 second
           char *GpsDate =parseValue[7];
           index1=0;
          for (i = 0; i <6 ; ++i) {
            Date[index1] = GpsDate[i];

            if (i==1 ||i==3) {
                index1++;
                Date[index1] = '/';
            }
            index1++;}
          Date[8]='\0';

       System_printf("DATE: %s\n",Date);
       System_flush();
    }
}

/* another NTP func recv over TCP*/
/*bool recvTimeFromNTp(char *serverIP, int serverPort)
{
    System_printf("\nRecvTimefromNtpServer is runnig");
    System_flush();

    int sockfd, connStat;
    bool retval=false;
    struct sockaddr_in serverAddr;
    char timedata[4];


    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        System_printf("Socket not created");
        close(sockfd);
        return false;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));  // clear serverAddr structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);     // convert port # to network order
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr.s_addr));

    connStat = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if(connStat < 0) {
        System_printf("recvTimeFromNTP::Error while connecting to server\n");
    }

        recv(sockfd,  timedata ,sizeof(timedata),0);   // recv  32 bit number 4 byte data from NTP
        if (sockfd>0) {
            close(sockfd);
        }
        System_flush();
        System_printf(" --timestamp : %lu\n",timedata);
         // Time =          2^24*Hour   +   2^16*Minute      +  2^8*Second     + 2^0*Milisecond
        timestamp= timedata[0]*16777216 +  timedata[1]*65536 + timedata[2]*256 + timedata[3];
        timestamp+=10800; // +3 gmt
        //System_printf("\nTime :%lu", timestamp);
       // System_flush();
        //ctime(&timedata);

        time_t rawtime = timestamp;

        // sec,min,hour,day,mon,year
        // Format time, "yyyy-mm-dd hh:mm:ss"
         ts = localtime(&rawtime);

       //  strftime(currentTime, sizeof(currentTime), "%Y-%m-%d %H:%M:%S", ts);


         year = ts->tm_year+1900;
         month = ts->tm_mon+1;
         day = ts->tm_mday;
         hour = ts->tm_hour;
         minutes = ts->tm_min;
         seconds = ts->tm_sec;

         ref=minutes;



    close(sockfd);
    return retval;
}*/


void sendData2Server(char *serverIP, int serverPort, char *data, int size)
{
    int sockfd;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd == -1) {
        System_printf("Socket not created");
        BIOS_exit(-1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));         /* clear serverAddr structure */
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);            /* convert port # to network order */
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));

    int connStat = connect(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr));

    if(connStat < 0) {
        System_printf("Error while connecting to server\n");

    if (sockfd > 0)
        close(sockfd);
        BIOS_exit(-1);
    }

    int numSend = send(sockfd, data, size, 0); /* send data to the server*/

    if(numSend < 0) {
        System_printf("Error while sending data to server\n");

    if (sockfd > 0) close(sockfd);
    BIOS_exit(-1);
    }
    if (sockfd > 0) close(sockfd);

}




Void clientSocketTask(UArg arg0, UArg arg1){
    char GPSloc[120];

    while(1){

    Mailbox_pend(mailbox0, &GPSloc, BIOS_WAIT_FOREVER);   // waits gps location

    sprintf(currentTime, "\nDate: %d-%lu-%d Time: %02d:%02d:%02d", day,month,year,hour,minutes ,seconds);
    strcat(currentTime, " - ");
    strcat(currentTime, GPSloc); // latitude and longitude values
    strcat(currentTime, " - ");
    strcat(currentTime, Date); // GPS date

    sendData2Server("192.168.1.26", SERVERPORT, currentTime, strlen(currentTime));


    }

}

Void SNTPsocketTask(UArg arg0, UArg arg1){
   while(1){

      int ret;
      int currPos;
      time_t ts;
      struct tm  *tk;
      struct sockaddr_in ntpAddr;
      struct addrinfo hints;
      struct addrinfo *addrs;
      struct addrinfo *currAddr;



      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET;   // IPv4
      hints.ai_socktype = SOCK_DGRAM;  // UDP

      ret = getaddrinfo(NTP_HOSTNAME, NTP_PORT, NULL, &addrs);
      if (ret != 0) {
          System_printf("startNTP: NTP host cannot be resolved!\n");
      }

      currPos = 0;

      for (currAddr = addrs; currAddr != NULL; currAddr = currAddr->ai_next) {
          if (currPos < NTP_SERVERS_SIZE) {
              ntpAddr = *(struct sockaddr_in *)(currAddr->ai_addr);
              memcpy(ntpServers + currPos, &ntpAddr, sizeof(struct sockaddr_in));
              currPos += sizeof(struct sockaddr_in);
          }
          else {
              break;
          }
      }

      freeaddrinfo(addrs);

      ret = SNTP_start(Seconds_get, Seconds_set, swi1_func, (struct sockaddr *)&ntpServers, NTP_SERVERS, 0);
      if (ret == 0) {
          System_printf("startNTP: SNTP cannot be started!\n");
      }



      SNTP_forceTimeSync();

      Semaphore_pend(semaphore3, BIOS_WAIT_FOREVER);   // waits for  10 seconds to restart sntp

      ts = time(NULL) +10800; // +3 gmt

      // sec,min,hour,day,mon,year
      // Format time, "yyyy-mm-dd hh:mm:ss"
      tk =localtime(&ts);

      year = tk->tm_year+1900;
      month = tk->tm_mon+1;
      day = tk->tm_mday;
      hour = tk->tm_hour;
      minutes = tk->tm_min;
      seconds = tk->tm_sec;


      System_printf("Date: %d-%lu-%d Time: %02d:%02d:%02d\n", day,month,year,hour,minutes ,seconds);
          System_flush();

      System_printf("Current time: %s\n", ctime(&ts));

      Semaphore_post(semaphore2);       // starts GPS task.
      System_flush();
   }


}


/*
 *  ======== netIPAddrHook ========
 *  This function is called when IP Addr is added/deleted
 */
void netIPAddrHook(unsigned int IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
    static Task_Handle taskHandle1,taskHandle2;
    Task_Params taskParams;
    Error_Block eb;

    // Create a HTTP task when the IP address is added
        if (fAdd && !taskHandle1 && !taskHandle2 ) {
              Error_init(&eb);

              /* clientSocketTask*/
          Task_Params_init(&taskParams);
          taskParams.stackSize = TASKSTACKSIZE;
          taskParams.priority = 1;
          taskHandle2 = Task_create((Task_FuncPtr)SNTPsocketTask, &taskParams, &eb);

              /* serverSocketTask*/
       Task_Params_init(&taskParams);
       taskParams.stackSize = TASKSTACKSIZE;
       taskParams.priority = 1;
       taskHandle1 = Task_create((Task_FuncPtr)clientSocketTask, &taskParams, &eb);



        if (taskHandle1 == NULL || taskHandle2== NULL ) {
            printError("netIPAddrHook: Failed to create HTTP and Socket Tasks\n", -1);
        }
     }

}

/*
 *  ======== main ========
 */
int main(void)
{
    GPS_Init();
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initEMAC();

    /* Turn on user LED */
    GPIO_write(Board_LED0, Board_LED_ON);

    System_printf("Starting the HTTP GET example\nSystem provider is set to "
            "SysMin. Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();
    //Semaphore_post(semaphore0);
    /* Start BIOS */
    BIOS_start();

    return (0);
}
