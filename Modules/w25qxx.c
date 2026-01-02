#include "w25qxx.h"
#include "spi.h" // 引入CubeMX生成的spi.h
#include "stdint.h"

// 片选控制宏
#define W25Q_CS_LOW()  HAL_GPIO_WritePin(W25_CS_GPIO_Port, W25_CS_Pin, GPIO_PIN_RESET)
#define W25Q_CS_HIGH() HAL_GPIO_WritePin(W25_CS_GPIO_Port, W25_CS_Pin, GPIO_PIN_SET)
// SPI读写一个字节
uint8_t W25Q_SPI_SwapByte(uint8_t byte)
{
    uint8_t r_byte;
    HAL_SPI_TransmitReceive(&hspi1, &byte, &r_byte, 1, 100);
    return r_byte;
}

// 初始化
void W25Q_Init(void)
{
    W25Q_CS_HIGH();
}

// 读取芯片ID (W25Q128 应该是 EF 40 18)
void W25Q_Read_ID(uint8_t *ID)
{
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(W25X_JedecDeviceID);
    ID[0] = W25Q_SPI_SwapByte(0xFF); // Manufacturer ID
    ID[1] = W25Q_SPI_SwapByte(0xFF); // Memory Type
    ID[2] = W25Q_SPI_SwapByte(0xFF); // Capacity
    W25Q_CS_HIGH();
}

// 写使能
void W25Q_Write_Enable(void)
{
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(W25X_WriteEnable);
    W25Q_CS_HIGH();
}

// 等待忙碌 (读/写/擦除后需要等待)
void W25Q_Wait_Busy(void)
{
    uint8_t status;
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(W25X_ReadStatusReg1);
    do {
        status = W25Q_SPI_SwapByte(0xFF);
    } while ((status & 0x01) == 0x01); // 等待BUSY位清空
    W25Q_CS_HIGH();
}

// 擦除一个扇区 (4KB)
void W25Q_Erase_Sector(uint32_t Dst_Addr)
{
    W25Q_Write_Enable();
    W25Q_Wait_Busy();
    
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(W25X_SectorErase);
    W25Q_SPI_SwapByte((uint8_t)((Dst_Addr) >> 16));
    W25Q_SPI_SwapByte((uint8_t)((Dst_Addr) >> 8));
    W25Q_SPI_SwapByte((uint8_t)Dst_Addr);
    W25Q_CS_HIGH();
    
    W25Q_Wait_Busy(); // 等待擦除完成
}

void W25Q_Erase_Chip(void)
{
    W25Q_Write_Enable();
    W25Q_Wait_Busy();
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(0x60); // Chip Erase 指令
    W25Q_CS_HIGH();
    W25Q_Wait_Busy(); // 全片擦除非常慢，可能要几十秒
}

// 2. 基础页写入 (保持你原来的，但参数类型改为 uint32_t 以防万一)
void W25Q_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    W25Q_Write_Enable();
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(0x02); // Page Program
    W25Q_SPI_SwapByte((uint8_t)((WriteAddr) >> 16));
    W25Q_SPI_SwapByte((uint8_t)((WriteAddr) >> 8));
    W25Q_SPI_SwapByte((uint8_t)WriteAddr);
    while (NumByteToWrite--)
    {
        W25Q_SPI_SwapByte(*pBuffer);
        pBuffer++;
    }
    W25Q_CS_HIGH();
    W25Q_Wait_Busy();
}

// 3. 【核心】无限制写入函数 (自动处理换页，不带擦除)
// 用这个函数替代你原来的 W25Q_Write
void W25Q_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    uint16_t pageremain;
    pageremain = 256 - (WriteAddr % 256); // 当前页剩余字节数

    if (NumByteToWrite <= pageremain) pageremain = NumByteToWrite;

    while (1)
    {
        W25Q_Write_Page(pBuffer, WriteAddr, pageremain);
        
        if (NumByteToWrite == pageremain) break; // 写完了
        
        // 调整指针和计数器
        pBuffer += pageremain;
        WriteAddr += pageremain;
        NumByteToWrite -= pageremain;
        
        // 下一次写满一页(256)或者剩下的
        if (NumByteToWrite > 256) pageremain = 256;
        else pageremain = NumByteToWrite;
    }
}
// 读取数据
void W25Q_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
    W25Q_CS_LOW();
    W25Q_SPI_SwapByte(W25X_ReadData);
    W25Q_SPI_SwapByte((uint8_t)((ReadAddr) >> 16));
    W25Q_SPI_SwapByte((uint8_t)((ReadAddr) >> 8));
    W25Q_SPI_SwapByte((uint8_t)ReadAddr);
    
    while (NumByteToRead--)
    {
        *pBuffer = W25Q_SPI_SwapByte(0xFF);
        pBuffer++;
    }
    W25Q_CS_HIGH();
}