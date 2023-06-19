#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;
bool check_value;
int bytes_done=0;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */

static bool nread(int sd, int len, uint8_t *buf) {

  // Read bytes from fd
  while (bytes_done < len){
    int bytes_now = read(sd, buf + bytes_done, len - bytes_done);

    if (bytes_now <0){
      return false;
    }

    bytes_done += bytes_now;
  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on 
 * failure */

static bool nwrite(int sd, int len, uint8_t *buf) {
  while (bytes_done < len){
    int bytes_now = write(sd, buf + bytes_done, len - bytes_done);

    if (bytes_now <0){
      return false;
    }


    bytes_done += bytes_now;
  }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int sd, uint32_t *op, uint8_t *ret, uint8_t *block) {
    uint8_t recieveBuf[HEADER_LEN];
    uint8_t returnCode;
    uint8_t recieveBuf_1[HEADER_LEN + JBOD_BLOCK_SIZE];
    uint32_t command = (*op >> 26); 
    uint32_t opCode = 0;
    int len_packet = HEADER_LEN;
  // check for command
    if (command == JBOD_READ_BLOCK) {
        len_packet += JBOD_BLOCK_SIZE; //264
        //using buffers according to their shifted regions
      memcpy(recieveBuf_1 + 6, &returnCode, sizeof(returnCode));
      memcpy(recieveBuf_1 + 8, block, JBOD_BLOCK_SIZE);


      return nread(sd, HEADER_LEN + JBOD_BLOCK_SIZE, recieveBuf_1);

    }
    else{
      len_packet=HEADER_LEN;//8
      return nread(sd, HEADER_LEN, recieveBuf_1);

    }

    check_value = nread(sd, len_packet, recieveBuf);

    if (!check_value) {
        return false;
    }
  //regardless of cmd do opcode
    memcpy(&opCode+2, recieveBuf, 4);
    opCode = ntohl(opCode);

    memcpy(&returnCode+6, recieveBuf + 4, sizeof(returnCode));

    if (*ret) {
        check_value = nread(sd, JBOD_BLOCK_SIZE, block);
    }

    return check_value;
}



/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {

  uint8_t sendBuf[HEADER_LEN + JBOD_BLOCK_SIZE];
  int len_packet = HEADER_LEN;
  uint32_t opCode = htonl(op);
  uint8_t returnCode;
  uint32_t command = (op >> 26) ; 

  // check for command being JBOD_WRITE_BLOCK
  if (command == JBOD_WRITE_BLOCK){

    if(len_packet > HEADER_LEN + JBOD_BLOCK_SIZE){

      memcpy(sendBuf+2, &opCode, sizeof(opCode));
      memcpy(sendBuf + 6, &returnCode, sizeof(returnCode));
      memcpy(sendBuf + 8, block, JBOD_BLOCK_SIZE);

      
      return nwrite(sd, HEADER_LEN + JBOD_BLOCK_SIZE, sendBuf);
    }
    else {

      memcpy(sendBuf+2, &opCode, sizeof(opCode));
      memcpy(sendBuf + 6, &returnCode, sizeof(returnCode));

      return nwrite(sd, HEADER_LEN, sendBuf);
    }
 return check_value;

  }
return check_value;
}




/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */

bool jbod_connect(const char *ip, uint16_t port) {
 
   // calling a socket
  cli_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1){
    return false;
  }
   struct sockaddr_in caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);

 if (inet_aton(ip, &caddr.sin_addr) == 0){
    return false;
  }
  //check connection
  if (connect(cli_sd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1){
    return false;
  }
  return true;
}




/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  // close the socket and set it-1
  close(cli_sd);
  cli_sd = -1;
}



/* sends the JBOD operation to the server (use the send_packet function) and receives and processes the 
* response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {

  // check for sending
  check_value = send_packet(cli_sd, op, block);
  
  if (check_value == true){
    return 0;
  }
  else{
    return -1;
  }
  // check for recievijng
  check_value= recv_packet(cli_sd, &op, NULL, block);

  if (check_value == true){
    return 0;
  }
  else{
    return -1;
  }

  return 0;
}