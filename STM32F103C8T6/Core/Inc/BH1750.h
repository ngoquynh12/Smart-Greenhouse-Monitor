#ifndef BH1750_H_ //Kiểm tra nếu macro BH1750_H_ chưa được định nghĩa
#define BH1750_H_ //Định nghĩa macro này, giúp các file khác biết rằng file đã được include

#include "stm32f1xx_hal.h"
#include <stdbool.h>

// Device Address
#define BH1750_GROUND_ADDR_WRITE  0x46  // Khi ADDR nối GND
#define BH1750_GROUND_ADDR_READ   0x47
#define BH1750_NO_GROUND_ADDR_WRITE  0xB8  // Khi ADDR nối VCC
#define BH1750_NO_GROUND_ADDR_READ   0xB9

// Instructions
#define CMD_POWER_DOWN          0x00
#define CMD_POWER_ON            0x01
#define CMD_RESET               0x03
#define CMD_H_RES_MODE          0x10
#define CMD_H_RES_MODE2         0x11
#define CMD_L_RES_MODE          0x13
#define CMD_ONE_H_RES_MODE      0x20
#define CMD_ONE_H_RES_MODE2     0x21
#define CMD_ONE_L_RES_MODE      0x23
#define CMD_CNG_TIME_HIGH       0x30
#define CMD_CNG_TIME_LOW        0x60

typedef struct BH1750_device BH1750_device_t;
struct BH1750_device {
    char* name;
    I2C_HandleTypeDef* i2c_handle; //Con trỏ tới cấu trúc xử lý I2C của STM32
    uint8_t address_r;
    uint8_t address_w;
    uint16_t value;
    uint8_t buffer[2]; //Mảng chứa 2 byte dữ liệu nhận được từ BH1750 qua I²C
    uint8_t mode; //Biến lưu trữ chế độ hoạt động hiện tại của cảm biến
    void (*poll)(BH1750_device_t*); //Con trỏ hàm, cho phép gọi hàm polling (kiểm tra, đọc dữ liệu) của thiết bị. Nó nhận đối số là con trỏ tới BH1750_device_t
};

HAL_StatusTypeDef BH1750_read_dev(BH1750_device_t* dev);
HAL_StatusTypeDef BH1750_init_i2c(I2C_HandleTypeDef* i2c_handle);
BH1750_device_t* BH1750_init_dev_struct(I2C_HandleTypeDef* i2c_handle, char* name, bool addr_grounded);
HAL_StatusTypeDef BH1750_init_dev(BH1750_device_t* dev);
HAL_StatusTypeDef BH1750_get_lumen(BH1750_device_t* dev);

#endif /* BH1750_H_ */
