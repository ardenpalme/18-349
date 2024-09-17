/** @file    mpu.h
 *
 *  @brief  Memory Protection functions (MPU)
 *
 *  @date   4/23/21
 *
 *  @author Arden Diakhate-Palme
 */
#ifndef _MPU_H_
#define _MPU_H_

#include <unistd.h>

/**
 * @brief  Returns ceiling (log_2 n).
 */
uint32_t mm_log2ceil_size(uint32_t n);

/**
 * @brief Enables a region for memory protection
 */
int mm_region_enable( uint32_t region_number, void *base_address, 
        uint8_t size_log2, int execute, int user_write_access);

/**
 * @brief Enables a region for memory protection
 */
void mm_region_disable( uint32_t region_number );

/**
 * @brief 
 */
void mm_disable();
/**
 * @brief 
 */
void mm_enable();

#endif /* _MPU_H_ */
