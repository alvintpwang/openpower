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
#include "ecmd_i2c.h"

int setup_i2c(const char* dev,int address) {
  int file = open(dev, O_RDWR);
  if (file < 0) {
    fprintf(stderr,"I2C bus invalid\n");
    return 0;
  }

  if (ioctl(file, I2C_SLAVE, address) < 0) {
   fprintf(stderr,"ioctl failed\n");
    return 0;
  }
  return file;
}


int check_i2c_errors(int file) {
  uint32_t v0;
  uint32_t v1;
  getscom(file,I2C_STATUS_REG,&v0,&v1);
  if (v0!=0x80000000) {
    fprintf(stderr,"ERROR present in P8 I2C Slave.  Clearing...\n");
    putscom(file,I2C_ERROR_REG,0x00000000,0x00000000);
    putscom(file,I2C_STATUS_REG,0x00000000,0x00000000);
    return 1;
  }
  return 0;
}

int getscom(int file, uint32_t address,uint32_t *value0,uint32_t *value1)
{
  //P8 i2c slave requires address to be shifted by 1
  address = address << 1;
  const char* address_buf = (const char*)&address;
  
  uint32_t res=write(file, address_buf, 4);
  if (res != 4) {
    return I2C_WRITE_ERROR;
  }
  char buf[8];
  res=read(file, buf, 8);
  if (res != 8) {
    return I2C_READ_ERROR;
  }
  memcpy(value1,&buf[0],4);
  memcpy(value0,&buf[4],4);
  return 0;
}

int getscomb(int file, uint32_t address,char* data,int start)
{
  //P8 i2c slave requires address to be shifted by 1
  address = address << 1;
  const char* address_buf = (const char*)&address;
  
  uint32_t res=write(file, address_buf, 4);
  if (res != 4) {
    return I2C_WRITE_ERROR;
  }
  char buf[8];
  res=read(file, buf, 8);
  if (res != 8) {
    return I2C_READ_ERROR;
  }
  int b;
  for (b=0;b<8;b++) {
    data[start+b]=buf[7-b];
  }
  return 0;
}


int putscom(int file, uint32_t address, uint32_t data0, uint32_t data1) {
    //P8 i2c slave requires address to be shifted by 1
    address = address << 1;
    const char* address_buf = (const char*)&address;
    const char* d0 = (const char*)&data0;
    const char* d1 = (const char*)&data1;
  
    char buf[12];
    memcpy(&buf[0],address_buf,4);
    memcpy(&buf[4],d1,4);
    memcpy(&buf[8],d0,4);
       
    uint32_t res=write(file, buf,12);
    if (res != 12) {
      return I2C_WRITE_ERROR;
    } 
    return 0;
}
