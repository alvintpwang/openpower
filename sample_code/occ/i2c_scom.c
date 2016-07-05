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





// To generate attn to OCC
#define ATTN_DATA                0x0006B035

// For BMC to read/write SRAM
#define OCB_ADDRESS              0x0006B070
#define OCB_DATA                 0x0006B075
#define OCB_STATUS_CONTROL_AND   0x0006B072
#define OCB_STATUS_CONTROL_OR    0x0006B073

// OCC SRAM address
#define OCC_COMMAND_ADDR 0xFFFF6000
#define OCC_RESPONSE_ADDR 0xFFFF7000

#define MODE_GETSCOM 1
#define MODE_PUTSCOM 2
#define MODE_OCC 3


#define DUMP_RAW 1

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
  int file=setup_i2c("/dev/i2c-3",0x50);


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
    printf("Getting OCC poll response data...\n");

    // Init OCB
    putscom(file, OCB_STATUS_CONTROL_OR,  0x08000000, 0x00000000);
    putscom(file, OCB_STATUS_CONTROL_AND, 0xFBFFFFFF, 0xFFFFFFFF);

    // Send poll command to OCC
    putscom(file, OCB_ADDRESS, OCC_COMMAND_ADDR, 0x00000000);
    putscom(file, OCB_ADDRESS, OCC_COMMAND_ADDR, 0x00000000);
    putscom(file, OCB_DATA, 0x00000001, 0x10001100);

    // Trigger ATTN
    putscom(file, ATTN_DATA, 0x01010000, 0x00000000);

    // TODO: check command status Refere to "1.6.2 OCC Command/Response Sequence" in OCC_OpenPwr_FW_Interfaces1.2.pdf
    // Use sleep as workaround
    sleep(2);

    // Get response data
    putscom(file, OCB_ADDRESS, OCC_RESPONSE_ADDR, 0x00000000);
    char occ_data[4096];
    
    getscomb(file,OCB_DATA,occ_data,0);

    uint16_t num_bytes=get_length(occ_data);
    int b;
    printf("Length found: %d\n",num_bytes);
    if (num_bytes > 4096) {
      fprintf(stderr,"ERROR:   Length must be < 4KB\n");
      exit(1);
    }

#ifdef DUMP_RAW
    printf("\nRAW data\n==================\n");
    for (int i=0;i<8;i++) {
      if(i==4) printf("  ");
      printf("%02x", occ_data[i]);
    }
    printf("\n");
#endif

    for (b=8; b<num_bytes; b=b+8) {
      getscomb(file, OCB_DATA, occ_data,b);
#ifdef DUMP_RAW
      for (int i=0;i<8;i++) {
        if(i==4) printf("  ");
        printf("%02x", occ_data[b+i]);
      }
      printf("\n");
#endif
    }

    printf("\n");
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
