#define I2C_STATUS_REG 0x000d0001
#define I2C_ERROR_REG  0x000d0002
#define I2C_READ_ERROR 1
#define I2C_WRITE_ERROR 2
#define I2C_DATABUFFER_SIZE_ERROR 3

int setup_i2c(const char*,int);
int getscom(int,uint32_t,uint32_t*,uint32_t*);
int getscomb(int, uint32_t,char*,int);
int putscom(int,uint32_t,uint32_t,uint32_t);
int check_i2c_errors(int);