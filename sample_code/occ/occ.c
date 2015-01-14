#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "occ.h"

uint16_t get_length(char* d) {
  uint16_t data_length;
  data_length = d[3] << 8;
  data_length = data_length | d[4];
  return data_length;
}

int read_occ_response(char* d, occ_response* o) {
  o->sequence_num = d[0];
  o->cmd_type = d[1];
  o->rtn_status = d[2];
  o->data_length = d[3] << 8;
  o->data_length = o->data_length | d[4];
  o->data.status = d[5];
  o->data.ext_status = d[6];
  o->data.occs_present = d[7];
  o->data.config = d[8];
  o->data.occ_state = d[9];
  o->data.reserved0 = d[10];
  o->data.reserved1 = d[11];
  o->data.error_log_id = d[12];
  o->data.error_log_addr_start = d[13] << 24;
  o->data.error_log_addr_start = o->data.error_log_addr_start | d[14] << 16;
  o->data.error_log_addr_start = o->data.error_log_addr_start | d[15] << 8;
  o->data.error_log_addr_start = o->data.error_log_addr_start | d[16];
  o->data.error_log_length = d[17] << 8;
  o->data.error_log_length = o->data.error_log_length | d[18];
  o->data.reserved2 = d[19];
  o->data.reserved3 = d[20];
  strncpy(&o->data.occ_code_level[0],(const char*)&d[21],16);
  strncpy(&o->data.sensor_eye_catcher[0],(const char*)&d[37],6);
  o->data.sensor_eye_catcher[6]='\0';
  o->data.num_of_sensor_blocks=d[43];
  o->data.sensor_data_version = d[44];
  
  if (strcmp(o->data.sensor_eye_catcher,"SENSOR")!=0) {
    fprintf(stderr,"ERROR: SENSOR not found at byte 37 (%s)\n",o->data.sensor_eye_catcher);
    return 1;
  }
  int b;
  int s;
  int dnum=45;
  o->data.blocks = malloc(sizeof(sensor_data_block)*o->data.num_of_sensor_blocks);
  //printf("Reading %d sensor blocks\n",o->data.num_of_sensor_blocks);
  for(b = 0;b < o->data.num_of_sensor_blocks;b++) {
    strncpy(&o->data.blocks[b].sensor_type[0],(const char*)&d[dnum],4);
    o->data.blocks[b].reserved0 = d[dnum+4];
    o->data.blocks[b].sensor_format = d[dnum+5];
    o->data.blocks[b].sensor_length = d[dnum+6];
    o->data.blocks[b].num_of_sensors = d[dnum+7];
    
//    printf("Sensor Header found: %s;  format=%d size=%d num=%d index=%d\n",o->data.blocks[b].sensor_type,
//	   o->data.blocks[b].sensor_format,o->data.blocks[b].sensor_length,o->data.blocks[b].num_of_sensors,dnum);
    dnum=dnum+8;
    
    if (strcmp(o->data.blocks[b].sensor_type,"TEMP")==0 ||
        strcmp(o->data.blocks[b].sensor_type,"FREQ")==0) {
      o->data.blocks[b].sensor = malloc(sizeof(occ_sensor)*o->data.blocks[b].num_of_sensors);

      for (s=0;s< o->data.blocks[b].num_of_sensors;s++) {
	o->data.blocks[b].sensor[s].sensor_id = d[dnum] << 8;
        o->data.blocks[b].sensor[s].sensor_id = o->data.blocks[b].sensor[s].sensor_id | d[dnum+1];
	o->data.blocks[b].sensor[s].value = d[dnum+2] << 8;
	o->data.blocks[b].sensor[s].value = o->data.blocks[b].sensor[s].value | d[dnum+3];
	dnum=dnum+o->data.blocks[b].sensor_length;
      }
    }
    else if (strcmp(o->data.blocks[b].sensor_type,"POWR")==0) {
      o->data.blocks[b].powr = malloc(sizeof(powr_sensor)*o->data.blocks[b].num_of_sensors);
 
      for (s=0;s< o->data.blocks[b].num_of_sensors;s++) {
	o->data.blocks[b].powr[s].sensor_id = d[dnum] << 8;
        o->data.blocks[b].powr[s].sensor_id = o->data.blocks[b].powr[s].sensor_id | d[dnum+1];
	o->data.blocks[b].powr[s].update_tag = d[dnum+2] << 24;
	o->data.blocks[b].powr[s].update_tag = o->data.blocks[b].powr[s].update_tag | d[dnum+3] << 16;
	o->data.blocks[b].powr[s].update_tag = o->data.blocks[b].powr[s].update_tag | d[dnum+4] << 8;
	o->data.blocks[b].powr[s].update_tag = o->data.blocks[b].powr[s].update_tag | d[dnum+5];
	o->data.blocks[b].powr[s].accumulator = d[dnum+6] << 24;
	o->data.blocks[b].powr[s].accumulator = o->data.blocks[b].powr[s].accumulator | d[dnum+7] << 16;
	o->data.blocks[b].powr[s].accumulator = o->data.blocks[b].powr[s].accumulator | d[dnum+8] << 8;
	o->data.blocks[b].powr[s].accumulator = o->data.blocks[b].powr[s].accumulator | d[dnum+9];
	o->data.blocks[b].powr[s].value = d[dnum+10] << 8;
        o->data.blocks[b].powr[s].value = o->data.blocks[b].powr[s].value | d[dnum+11];
		
	dnum=dnum+o->data.blocks[b].sensor_length;
      }
    }
    else 
    {
      fprintf(stderr,"ERROR: sensor type %s not found\n",o->data.blocks[b].sensor_type);
      return 1;
    }
  }
  return 0;  
}
int close_occ_response(occ_response* o) {
  int b;
  for(b = 0;b < o->data.num_of_sensor_blocks;b++) {
      free(o->data.blocks[b].sensor);
      free(o->data.blocks[b].powr);
  }
  free(o->data.blocks);
  return 0;    
}
