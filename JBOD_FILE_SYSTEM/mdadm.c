#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

uint32_t op(int cmd,int disk_num, int reserved, int block_num){ 

  uint32_t temp_cmd = (cmd) << 26;  
  uint32_t temp_disk =  (disk_num) << 22;  
  uint32_t temp_reserve= reserved<<8;
  uint32_t temp_block =  (block_num);  

  uint32_t format = temp_cmd| temp_disk| temp_reserve|temp_block;  
  return format;
  }

// global is used for mounting and unmounting
int global=-1;
// check basically checks if jbod operation causes error
int check=0;
int mdadm_mount(void) {

if (global==-1){

  check=jbod_operation(op(JBOD_MOUNT,0,0,0),NULL);
  
  if(check!=-1){
  global=1;
}
  return 1;
}

else{
return -1;
}}	


int mdadm_unmount(void) {

if (global==1){

  check=jbod_operation(op(JBOD_UNMOUNT,0,0,0),NULL);

  if (check!=-1){
    global=-1;
  }
  return 1;
}

else{
return -1; 
}}

//Used later for offset
int min(int x, int y){
if(x<y){  
  return x;}
else{
  return y;}
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf)  {
  
//Checking for invalid parameters and out of bound values
 if ((global == -1||(( (addr+len) > 16*JBOD_DISK_SIZE)||len>1024 ))||buf == NULL){ 
    
    
     if(len > 0) {  return -1;}
   
 }

//Initializing byte count (number of bytes read) to 0
int byte_done_reading=0; 

 while(byte_done_reading < len){ 

  uint32_t diskId = addr / JBOD_DISK_SIZE; 
  uint32_t blockId = (addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE; 
  uint32_t offset = (addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;
  uint8_t tempBuf[256]; 
  uint32_t bytes_to_read; 
  
  //For reading across disk

  check = jbod_operation(op(JBOD_SEEK_TO_DISK,diskId,0, 0),NULL);
      if(check == -1){
        return -1;
      }

//For reading across block check if the cmd is seek to block

check = jbod_operation(op( JBOD_SEEK_TO_BLOCK,0,0, blockId),NULL);
    if(check == -1){
      return -1;
    }

check = jbod_operation (op (JBOD_READ_BLOCK,0,0,0), tempBuf);  
    if(check== -1){ 
      return -1;
    }

  bytes_to_read= 0; 
   
if(offset==0){
  bytes_to_read=min(bytes_to_read+len,JBOD_BLOCK_SIZE);
}
if(offset!=0){
  bytes_to_read=JBOD_BLOCK_SIZE-offset;
}

    memcpy((buf+byte_done_reading), tempBuf+offset, bytes_to_read);
    ////Incrementing to know bytes to read
    byte_done_reading +=  bytes_to_read; 
    ////Incrementing the address 
    addr += byte_done_reading; 
  }
  return len; 
}




int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {

//Checking for invalid parameters and out of bound values
 if ((global == -1||(( (addr+len) > 16*JBOD_DISK_SIZE)||len>1024 ))||buf == NULL){ 
    
    
     if(len > 0) {  return -1;}
   
 }

//Initializing byte count (number of bytes read) to 0
int byte_done_writing=0; 

 while(byte_done_writing < len){

  uint32_t diskId = addr / JBOD_DISK_SIZE; 
  uint32_t blockId = (addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE; 
  uint32_t offset = (addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;
  uint8_t tempBuf[256]; 
  uint32_t bytes_to_read=0; 
  //For reading across disk
  check = jbod_operation(op(JBOD_SEEK_TO_DISK,diskId,0, 0),NULL);
      if(check == -1){
        return -1;
      }
//For reading across block check if the cmd is seek to block
  check = jbod_operation(op( JBOD_SEEK_TO_BLOCK,0, 0,blockId),NULL);
      if(check == -1){
        return -1;
    }
  check = jbod_operation (op (JBOD_READ_BLOCK,0,0,0), tempBuf);  
      if(check== -1){ 
        return -1;
    }
    if(offset==0){
      bytes_to_read=min(bytes_to_read+len,JBOD_BLOCK_SIZE);
    }
    if(offset!=0){
      bytes_to_read=JBOD_BLOCK_SIZE-offset;
    }

    if((offset+len)<=JBOD_BLOCK_SIZE){

    jbod_operation(op(JBOD_SEEK_TO_DISK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_SEEK_TO_BLOCK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_READ_BLOCK,diskId,0,blockId),tempBuf);

    memcpy(tempBuf+offset,buf+byte_done_writing, bytes_to_read);

    jbod_operation(op(JBOD_SEEK_TO_DISK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_SEEK_TO_BLOCK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_WRITE_BLOCK,diskId,0,blockId),tempBuf);

    } 

else {

    int bytes_left=offset+len-JBOD_BLOCK_SIZE;

    // Read into the temporary buffer to store the initial state of the appropriate block of a disk
    jbod_operation(op(JBOD_SEEK_TO_DISK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_SEEK_TO_BLOCK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_READ_BLOCK,diskId,0,blockId),tempBuf);

    // Copy the bytes from write_buf to tempBuf
    memcpy(tempBuf+offset,buf+byte_done_writing, bytes_to_read);

    // Write the bytes to the disk
    jbod_operation(op(JBOD_SEEK_TO_DISK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_SEEK_TO_BLOCK,diskId,0,blockId),NULL);
    jbod_operation(op(JBOD_WRITE_BLOCK,diskId,0,blockId),tempBuf);

    // Write the remaining bytes to the next block(s) of the disk
    uint32_t newaddr=offset+len-byte_done_writing;
    
while (bytes_left > 0) {
    // Calculate the disk and block ID based on the new address
    uint32_t newdisk = newaddr / JBOD_BLOCK_SIZE;
    uint32_t newblock = newaddr % JBOD_BLOCK_SIZE;

    // Read the block from the disk
    jbod_operation(op(JBOD_SEEK_TO_DISK, newdisk, 0, 0), NULL);
    jbod_operation(op(JBOD_SEEK_TO_BLOCK, newdisk, 0, newblock), NULL);
    jbod_operation(op(JBOD_READ_BLOCK, newdisk, 0, newblock), tempBuf);

    // Calculate the number of bytes to copy
    uint32_t bytes_to_copy = bytes_left < JBOD_BLOCK_SIZE ? bytes_left : JBOD_BLOCK_SIZE;

    // Copy the bytes from the buffer to the block
    memcpy(tempBuf, buf + newaddr - addr, bytes_to_copy);

    // Write the block to the disk
    jbod_operation(op(JBOD_SEEK_TO_DISK, newdisk, 0, 0), NULL);
    jbod_operation(op(JBOD_SEEK_TO_BLOCK, newdisk, 0, newblock), NULL);
    jbod_operation(op(JBOD_WRITE_BLOCK, newdisk, 0, newblock), tempBuf);

    // Update the counters
    bytes_left -= bytes_to_copy;
    newaddr -= bytes_to_copy;
}
}


// Incrementing bytes done and address to move forward/completing the remaining bytes
byte_done_writing += bytes_to_read; 
addr += bytes_to_read;
 }
 return len;
 }




 /*  
 For implementing cache when reading and writing

 if(cache_lookup(diskId,blockId,tempBuf)==-1){
  jbod_operation(op( JBOD_SEEK_TO_DISK,diskId,0, 0),NULL);
  jbod_operation(op( JBOD_SEEK_TO_BLOCK,0,0, blockId),NULL);

  jbod_operation(op(JBOD_READ_BLOCK,diskId,0,blockId),tempBuf);
  cache_insert(diskId,blockId, tempBuf);
} 

 
if(cache_lookup(diskId,blockId,tempBuf) == 1 ){
	    cache_update(diskId,blockId,tempBuf);
      }
      else{
	    cache_insert(diskId,blockId,tempBuf);
      }
*/