
typedef struct {
  uint16_t sensor_id;
  uint16_t value;
} occ_sensor;

typedef struct {
  uint16_t sensor_id;
  uint32_t update_tag;
  uint32_t accumulator;
  uint16_t value;
} powr_sensor;


typedef struct {
  char sensor_type[5];
  uint8_t reserved0;
  uint8_t sensor_format;
  uint8_t sensor_length;
  uint8_t num_of_sensors;
  occ_sensor *sensor;
  powr_sensor *powr;
  
} sensor_data_block;

typedef struct {
  uint8_t status;
  uint8_t ext_status;
  uint8_t occs_present;
  uint8_t config;
  uint8_t occ_state;
  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t error_log_id;
  uint32_t error_log_addr_start;
  uint16_t error_log_length;
  uint8_t reserved2;
  uint8_t reserved3;
  char occ_code_level[17];
  char sensor_eye_catcher[7];
  uint8_t num_of_sensor_blocks;
  uint8_t sensor_data_version;
  sensor_data_block* blocks;
} occ_poll_data;

typedef struct {
  uint8_t sequence_num;
  uint8_t cmd_type;
  uint8_t rtn_status;
  uint16_t data_length;
  occ_poll_data data;
  uint16_t chk_sum;
} occ_response;

int read_occ_response(char*, occ_response*);
int close_occ_response(occ_response*);
uint16_t get_length(char*); 
