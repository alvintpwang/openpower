#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <endian.h>
#include "occ.h"
#include "ecmd_i2c.h"

#define SCOM_OCC_SRAM_WOX  0x0006B013
#define SCOM_OCC_SRAM_WAND 0x0006B012
#define SCOM_OCC_SRAM_ADDR 0x0006B010
#define SCOM_OCC_SRAM_DATA 0x0006B015
#define OCC_COMMAND_ADDR 0xFFFF6000
#define OCC_RESPONSE_ADDR 0xFFFF7000

#define MODE_GETSCOM 1
#define MODE_PUTSCOM 2
#define MODE_OCC 3

int main(int argc, char *argv[])
{
  int mode=MODE_OCC;
  if (argc==4) { 
    mode=MODE_PUTSCOM;
  }
  if (argc==2) {
    mode=MODE_GETSCOM;
  }
  
  //setup i2c interface that P8 connects to
  int file=setup_i2c("/dev/i2c3",0x50);  


  uint32_t scom_address;
  uint32_t scom_data0;
  uint32_t scom_data1;
  int rc;
  
  //read addresses off command line
  if (mode==MODE_PUTSCOM || mode==MODE_GETSCOM) {
    sscanf(argv[1],"%x",&scom_address);
  }

  // just do a putscom
  if (mode==MODE_PUTSCOM) {
    //putscom
    sscanf(argv[2],"%x",&scom_data0);
    sscanf(argv[3],"%x",&scom_data1);
 
    rc=putscom(file,scom_address,scom_data0,scom_data1);
    if (rc) {
      fprintf(stderr,"putscom error: %d\n",rc);
      exit(1);
    }
    printf("PUTSCOM %08x: %08x%08x\n",scom_address,scom_data0,scom_data1);
  }
  //just do a getscom
  if (mode==MODE_GETSCOM) {
    //getscom
    rc=getscom(file,scom_address,&scom_data0,&scom_data1);
    if (rc) {
      fprintf(stderr,"getscom error: %d\n",rc);
      exit(1);
    }
    printf("GETSCOM %08x: %08x%08x\n",scom_address,scom_data0,scom_data1);
    rc=check_i2c_errors(file);
    if (rc) { 
      fprintf(stderr,"ERROR present in P8 I2C Slave\n");
    }
  }
  //grab all the occ data and parse
  if (mode==MODE_OCC) {
    printf("Getting OCC data...\n");

    //Procedure to access SRAM where OCC data is located
    putscom(file,SCOM_OCC_SRAM_WOX,0x08000000,0x00000000);
    putscom(file,SCOM_OCC_SRAM_WAND,0xFBFFFFFF,0xFFFFFFFF);
    putscom(file,SCOM_OCC_SRAM_ADDR,OCC_RESPONSE_ADDR,0x00000000);
    putscom(file,SCOM_OCC_SRAM_ADDR,OCC_RESPONSE_ADDR,0x00000000);
    char occ_data[4096];
    
    getscomb(file,SCOM_OCC_SRAM_DATA,occ_data,0);
           
    uint16_t num_bytes=get_length(occ_data);
    int b;
    printf("Length found: %d\n",num_bytes);
    if (num_bytes > 4096) {
      fprintf(stderr,"ERROR:   Length must be < 4KB\n");
      exit(1);
    }
    for (b=8; b<num_bytes; b=b+8) {
      getscomb(file,SCOM_OCC_SRAM_DATA,occ_data,b);
    }
    
    occ_response occ_resp;
    read_occ_response(occ_data,&occ_resp);
    printf("OCC Code Level: %s\n",occ_resp.data.occ_code_level);
    int s;
    for (b=0;b<occ_resp.data.num_of_sensor_blocks;b++) {
      printf("Sensor block: %d\n",b);
      printf("\t%s Sensors\n",occ_resp.data.blocks[b].sensor_type);
      for (s=0;s<occ_resp.data.blocks[b].num_of_sensors;s++) {
	if (strcmp(occ_resp.data.blocks[b].sensor_type,"TEMP")==0 ||
	    strcmp(occ_resp.data.blocks[b].sensor_type,"FREQ")==0) {
	  printf("\t\t%d\n",occ_resp.data.blocks[b].sensor[s].value);
	}
      }
    }
    
    close_occ_response(&occ_resp);
  }
  exit(0);
}
