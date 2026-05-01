# valeo_ecu_server

/**********************************************************************
 * Programme socket serveur TCP pour simuler une ECU automobile
  * Fonctionnalités :
 *  - read port 9000
 *  - recieve data from TCP client
 *  - read (generate) Can frames for some physical values (RPM, speed, temp, pressure)
 *  - save datas in      /var/tmp/ecu_can_data.log
 *  - Activate Mode daemon (-d) before starting the server
 *  - Management of  signals
 *  - Log to syslog events like start, stop, connections, errors
 *  - Handle multiple clients (one at a time) with accept() in a loop
 *  - Clean up resources properly (close sockets, free memory, etc.)
 *  - Use of volatile sig_atomic_t for signal handling
 *  - Use of setsockopt to set timeouts on sockets
 *  - Use of syslog for logging important events and errors
 *  - Use of daemon() to run in background if -d option is provided
 * Done by: Arnaud, Arnaud
 * Date: 2026-04-29 
 
 *********************************************************************/
