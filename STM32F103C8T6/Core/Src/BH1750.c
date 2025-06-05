#include <stdlib.h>
#include <string.h>

#include "stm32f1xx_hal.h"

#include "BH1750.h"

void _Error_Handler(char * file, int line)
{
  while(1)
  {
  }
}

HAL_StatusTypeDef BH1750_init_i2c(I2C_HandleTypeDef* i2c_handle)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0}; //cấu hình các chân GPIO, khởi tạo tất cả các trường về 0

    // Cấu hình chân PB6 (SCL) và PB7 (SDA)
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; //Chế độ Alternate Function Open-Drain
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); //Khởi tạo các chân PB6, PB7 theo cấu hình đã thiết lập

    i2c_handle->Instance = I2C1;
    i2c_handle->Init.ClockSpeed = 100000;
    i2c_handle->Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c_handle->Init.OwnAddress1 = 0; //địa chỉ của master
    i2c_handle->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_handle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_handle->Init.OwnAddress2 = 0;
    i2c_handle->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_handle->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    //Gọi hàm HAL_I2C_Init() để khởi tạo module I2C với các thiết lập đã cấu hình
    if (HAL_I2C_Init(i2c_handle) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef BH1750_send_command(BH1750_device_t* dev, uint8_t cmd)
{
	//TODO hal checks
	if(HAL_I2C_Master_Transmit(
			dev->i2c_handle,	//Con trỏ tới cấu trúc I2C đã khởi tạo
			dev->address_w,		//I2C addr of dev
			&cmd,				//Con trỏ đến biến lệnh cần gửi
			1,					//Số byte cần truyền
			10					//Wait time (ms)
		) != HAL_OK) return HAL_ERROR;

	return HAL_OK;
}
//Hàm này được gọi để polling (kiểm tra, đọc dữ liệu) của cảm biến BH1750
void BH1750_poll_self(BH1750_device_t* self)
{
	BH1750_get_lumen(self); //hàm này thực hiện quá trình đọc dữ liệu và chuyển đổi sang đơn vị lumen (hoặc lux)
}
//Hàm khởi tạo cấu trúc thiết bị BH1750
BH1750_device_t* BH1750_init_dev_struct(I2C_HandleTypeDef* i2c_handle,
		char* name, bool addr_grounded)
{
	BH1750_device_t* init =
			(BH1750_device_t*)calloc(1, sizeof(BH1750_device_t));
//Gọi calloc() để cấp phát bộ nhớ cho một cấu trúc BH1750_device_t và khởi tạo tất cả các byte về 0
	if(init == NULL) return NULL;

	if(addr_grounded){
	        init->address_r = BH1750_GROUND_ADDR_READ;
	        init->address_w = BH1750_GROUND_ADDR_WRITE;
	}else{
	        init->address_r = BH1750_NO_GROUND_ADDR_READ;
	        init->address_w = BH1750_NO_GROUND_ADDR_WRITE;
	}
	//Cấp phát bộ nhớ cho chuỗi name trong cấu trúc, với kích thước bằng độ dài chuỗi được truyền
	init->name = (char*)malloc(sizeof(char) * strlen(name));

	if(init->name == NULL) return NULL;

	init->i2c_handle = i2c_handle;

	strcpy(init->name, name); //Sao chép chuỗi name từ tham số vào bộ nhớ đã cấp phát trong cấu trúc

	init->poll = &BH1750_poll_self; //Gán con trỏ hàm poll trỏ đến hàm BH1750_poll_self. Điều này cho phép gọi hàm polling từ cấu trúc mà không cần gọi trực tiếp hàm

	return init;
}

HAL_StatusTypeDef BH1750_init_dev(BH1750_device_t* dev)
{
	BH1750_send_command(dev, CMD_POWER_ON);
	BH1750_send_command(dev, CMD_RESET);
	BH1750_send_command(dev, CMD_H_RES_MODE);

	return HAL_OK;
}
//Hàm đọc dữ liệu từ BH1750
HAL_StatusTypeDef BH1750_read_dev(BH1750_device_t* dev)
{
	if(HAL_I2C_Master_Receive(dev->i2c_handle,
			dev->address_r,
			dev->buffer,
			2,
			10
	) != HAL_OK) return HAL_ERROR;

	return HAL_OK;
}

HAL_StatusTypeDef BH1750_convert(BH1750_device_t* dev)
{
	dev->value = dev->buffer[0];
	dev->value = (dev->value << 8) | dev->buffer[1];

	//TODO check float stuff
	dev->value/=1.2;

	return HAL_OK;
}

HAL_StatusTypeDef BH1750_get_lumen(BH1750_device_t* dev)
{
	BH1750_read_dev(dev);
	BH1750_convert(dev);
	return HAL_OK;
}
