/**
  * @file    touch.c
  * @brief   GT911 触摸控制器驱动 (800x480 RGB LCD)
  */

#include "touch.h"
#include "port/config.h"

volatile TouchStructure touchInfo;
static volatile uint8_t Modify_Flag = 0;   /* 硬件版本适配标志 */

/* I2C 读写函数声明 */
static uint8_t GT9XX_WriteHandle(uint16_t addr);
static uint8_t GT9XX_WriteData(uint16_t addr, uint8_t value);
static uint8_t GT9XX_WriteReg(uint16_t addr, uint8_t cnt, uint8_t *value);
static uint8_t GT9XX_ReadReg(uint16_t addr, uint8_t cnt, uint8_t *value);
static void    PanelRecognition(void);

//--------------------------------------------------------------------+
// 复位 GT911 (将 I2C 地址设为 0xBA/0xBB)
//--------------------------------------------------------------------+

void GT9XX_Reset(void)
{
    Touch_INT_Out();

    HAL_GPIO_WritePin(Touch_INT_PORT, Touch_INT_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_SET);
    Touch_IIC_Delay(10000);

    HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_RESET);
    Touch_IIC_Delay(150000);
    HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_SET);
    Touch_IIC_Delay(350000);
    Touch_INT_In();
    Touch_IIC_Delay(20000);
}

//--------------------------------------------------------------------+
// I2C 读写底层
//--------------------------------------------------------------------+

static uint8_t GT9XX_WriteHandle(uint16_t addr)
{
    Touch_IIC_Start();
    if (Touch_IIC_WriteByte(GT9XX_IIC_WADDR) != ACK_OK) {
        Touch_IIC_Stop();
        return ERROR;
    }
    Touch_IIC_WriteByte((uint8_t)(addr >> 8));
    Touch_IIC_WriteByte((uint8_t)(addr));
    return SUCCESS;
}

static uint8_t GT9XX_WriteData(uint16_t addr, uint8_t value)
{
    Touch_IIC_Start();
    if (GT9XX_WriteHandle(addr) != SUCCESS) {
        Touch_IIC_Stop();
        return ERROR;
    }
    Touch_IIC_WriteByte(value);
    Touch_IIC_Stop();
    return SUCCESS;
}

static uint8_t GT9XX_WriteReg(uint16_t addr, uint8_t cnt, uint8_t *value)
{
    Touch_IIC_Start();
    if (GT9XX_WriteHandle(addr) != SUCCESS) {
        Touch_IIC_Stop();
        return ERROR;
    }
    for (uint8_t i = 0; i < cnt; i++)
        Touch_IIC_WriteByte(value[i]);
    Touch_IIC_Stop();
    return SUCCESS;
}

static uint8_t GT9XX_ReadReg(uint16_t addr, uint8_t cnt, uint8_t *value)
{
    Touch_IIC_Start();
    if (GT9XX_WriteHandle(addr) != SUCCESS) {
        Touch_IIC_Stop();
        return ERROR;
    }

    Touch_IIC_Start();
    if (Touch_IIC_WriteByte(GT9XX_IIC_RADDR) != ACK_OK) {
        Touch_IIC_Stop();
        return ERROR;
    }

    for (uint8_t i = 0; i < cnt; i++) {
        if (i == (cnt - 1))
            value[i] = Touch_IIC_ReadByte(0);   /* 最后一字节发 NOACK */
        else
            value[i] = Touch_IIC_ReadByte(1);   /* 其他字节发 ACK */
    }
    Touch_IIC_Stop();
    return SUCCESS;
}

//--------------------------------------------------------------------+
// 硬件版本识别 (兼容旧版 1024x600 触摸屏)
//--------------------------------------------------------------------+

static void PanelRecognition(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    Touch_INT_CLK_ENABLE;
    Touch_RST_CLK_ENABLE;

    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin   = Touch_INT_PIN;
    HAL_GPIO_Init(Touch_INT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = Touch_RST_PIN;
    HAL_GPIO_Init(Touch_RST_PORT, &GPIO_InitStruct);

    Touch_IIC_Delay(4000);

    /* 旧版硬件 RST + INT 未连接至核心板, 检测引脚电平判断版本 */
    if ((HAL_GPIO_ReadPin(Touch_RST_PORT, Touch_RST_PIN) != 1) &&
        (HAL_GPIO_ReadPin(Touch_INT_PORT, Touch_INT_PIN) != 1)) {
        Modify_Flag = 1;
    }
}

//--------------------------------------------------------------------+
// 触摸初始化
//--------------------------------------------------------------------+

uint8_t Touch_Init(void)
{
    uint8_t GT9XX_Info[11];
    uint8_t cfgVersion = 0;

    PanelRecognition();
    Touch_IIC_GPIO_Config();
    GT9XX_Reset();

    GT9XX_ReadReg(GT9XX_ID_ADDR, 11, GT9XX_Info);
    GT9XX_ReadReg(GT9XX_CFG_ADDR, 1, &cfgVersion);

    if (GT9XX_Info[0] != '9') {
        DEBUG_PRINT("Touch Error: no GT911 detected\r\n");
        return ERROR;
    }

    DEBUG_PRINT("Touch ID: GT%.4s\r\n", GT9XX_Info);
    DEBUG_PRINT("Firmware: 0x%04x\r\n",
                (GT9XX_Info[5] << 8) + GT9XX_Info[4]);
    DEBUG_PRINT("Resolution: %d x %d\r\n",
                (GT9XX_Info[7] << 8) + GT9XX_Info[6],
                (GT9XX_Info[9] << 8) + GT9XX_Info[8]);
    DEBUG_PRINT("Config ver: 0x%02x\r\n", cfgVersion);

    /* 判断是否需要坐标缩放 (1024x600 → 800x480) */
    uint16_t resX = (GT9XX_Info[7] << 8) + GT9XX_Info[6];
    if (resX == 1024) {
        Modify_Flag = 1;
    } else if (resX == 800) {
        Modify_Flag = 0;
    }

    return SUCCESS;
}

//--------------------------------------------------------------------+
// 触摸扫描 (循环调用)
//--------------------------------------------------------------------+

void Touch_Scan(void)
{
    uint8_t touchData[2 + 8 * TOUCH_MAX];

    GT9XX_ReadReg(GT9XX_READ_ADDR, 2 + 8 * TOUCH_MAX, touchData);
    GT9XX_WriteData(GT9XX_READ_ADDR, 0);        /* 清除寄存器标志 */

    touchInfo.num = touchData[0] & 0x0F;

    if (touchInfo.num >= 1 && touchInfo.num <= 5) {
        for (uint8_t i = 0; i < touchInfo.num; i++) {
            touchInfo.y[i] = (touchData[5 + 8 * i] << 8) | touchData[4 + 8 * i];
            touchInfo.x[i] = (touchData[3 + 8 * i] << 8) | touchData[2 + 8 * i];

            /* 旧版硬件坐标缩放 1024x600 → 800x480 */
            if (Modify_Flag == 1) {
                touchInfo.y[i] = (uint16_t)(touchInfo.y[i] * 0.80f);
                touchInfo.x[i] = (uint16_t)(touchInfo.x[i] * 0.78f);
            }
        }
        touchInfo.flag = 1;
    } else {
        touchInfo.flag = 0;
    }
}

//--------------------------------------------------------------------+
// 配置读写 (预留)
//--------------------------------------------------------------------+

void GT9XX_SendCfg(void) { /* 预留 */ }
void GT9XX_ReadCfg(void) { /* 预留 */ }
