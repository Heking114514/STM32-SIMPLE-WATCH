#ifndef __W25QXX_H
#define __W25QXX_H

#include "main.h" // 包含HAL库
#include "spi.h"  // <--- 必须有这个，且 spi.h 必须按第一步修好

// W25Q128 指令集
#define W25X_WriteEnable		0x06 
#define W25X_ReadStatusReg1		0x05 
#define W25X_ReadData			0x03 
#define W25X_PageProgram		0x02 
#define W25X_SectorErase		0x20 
#define W25X_JedecDeviceID		0x9F 

// 函数声明
void W25Q_Init(void);
void W25Q_Read_ID(uint8_t *ID);
void W25Q_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void W25Q_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void W25Q_Erase_Sector(uint32_t Dst_Addr);
void W25Q_Erase_Chip(void);


#endif