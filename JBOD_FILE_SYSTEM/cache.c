#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"
#include "jbod.h"
static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;



int cache_create(int num_entries) {
  if (cache!=NULL) {
    return -1; 
  }
  if (num_entries < 2 || num_entries > 4096) {
    return -1; // parameters
  }
  cache_size = num_entries;
  clock=0;
  cache = calloc(cache_size, sizeof(cache_entry_t));
    if (cache) {
    return 1; 
  } else {
    return -1; 
  }
}

int cache_destroy(void) {
  if (cache==NULL) {
    return -1; 
  }

  if (cache_size < 2 || cache_size > 4096) {
    return -1; // invalid parameter
  }
  clock=0;
  free(cache);
  cache = NULL;
  cache_size = 0;
  return 1;  
}



int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  //parameters
  if(cache == NULL || buf == NULL || disk_num < 0 || disk_num >= JBOD_NUM_DISKS || block_num < 0 || block_num >= JBOD_NUM_BLOCKS_PER_DISK){
    return -1;
  }
  
  for(int i = 0; i < cache_size; i++){
    if(cache[i].valid && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
      cache[i].access_time=clock++;
    num_hits++;
  num_queries++;  //increases hit rate compared to being outside
      return 1;
    }
  }
  
  return -1;
}


void cache_update(int disk_num, int block_num, const uint8_t *buf) {

  for(int i = 0; i< cache_size;i++){
    if(cache[i].valid && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
        clock++;
        cache[i].access_time++;
        memcpy(cache[i].block,buf,JBOD_BLOCK_SIZE);
        
      }
    }
}
 

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  //parameters
  if(buf == NULL || cache == NULL || disk_num < 0 || disk_num >= JBOD_NUM_DISKS || block_num < 0 || block_num >= JBOD_NUM_BLOCKS_PER_DISK){
    return -1;
  }

  for(int i = 0; i < cache_size; i++){
    if(cache[i].valid && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      return -1;
    }
    if(!cache[i].valid){
      cache[i].disk_num = disk_num;
      cache[i].block_num = block_num;
      cache[i].access_time = clock++;
      cache[i].valid = true;
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      return 1;
    }
}
  
//least Recenly Used

cache_entry_t *initial_block = &cache[0];
for(int i = 1; i < cache_size; i++){      
    if(cache[i].access_time <= initial_block->access_time){
        initial_block = &cache[i];
    }
}

initial_block->disk_num = disk_num;
initial_block->block_num = block_num;
initial_block->access_time = clock++;
memcpy(initial_block->block, buf, JBOD_BLOCK_SIZE);

  return 1;
}


bool cache_enabled(void) {
  if (cache != NULL && cache_size > 0) {
    return true;
  } else {
    return false;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}