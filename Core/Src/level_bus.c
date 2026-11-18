#include "level_bus.h"
#include "level_config.h"
#include "main.h"

static I2C_HandleTypeDef bus;
void LevelBus_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    GPIO_InitTypeDef pins = {0};
    pins.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    pins.Mode = GPIO_MODE_AF_OD;
    pins.Pull = GPIO_NOPULL; /* External pull-ups on SDK-1.1M. */
    pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    pins.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &pins);
    bus.Instance = I2C1;
    bus.Init.ClockSpeed = 400000;
    bus.Init.DutyCycle = I2C_DUTYCYCLE_2;
    bus.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    bus.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    bus.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    bus.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&bus) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigAnalogFilter(&bus, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
        Error_Handler();
}
bool LevelBus_Read(uint8_t address, uint8_t reg, uint8_t *data, size_t size)
{
    return HAL_I2C_Mem_Read(&bus, (uint16_t)address << 1, reg,
        I2C_MEMADD_SIZE_8BIT, data, (uint16_t)size, LEVEL_I2C_TIMEOUT_MS) == HAL_OK;
}
bool LevelBus_Write(uint8_t address, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&bus, (uint16_t)address << 1, reg,
        I2C_MEMADD_SIZE_8BIT, &value, 1, LEVEL_I2C_TIMEOUT_MS) == HAL_OK;
}
bool LevelBus_Send(uint8_t address, const uint8_t *data, size_t size)
{
    return HAL_I2C_Master_Transmit(&bus, (uint16_t)address << 1,
        (uint8_t *)data, (uint16_t)size, LEVEL_I2C_TIMEOUT_MS) == HAL_OK;
}
