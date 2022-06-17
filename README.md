# GPS_Project!
In this project Neo6mv2 GPS receiver implemented to the tiva-c 1294xl launchpad. GPS tasks receive latitude, longtitude and date values every second and send over Ethernet port to a server on the local network (Hercules). If the location is not fixed sends “0.0000 0.0000” to the server.

## Flow and Synchronization Table Between Threads
![image](https://user-images.githubusercontent.com/98567140/151528274-39fafb9c-17e6-4e79-96fb-73f131fa942f.png)


