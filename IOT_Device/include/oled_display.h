#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "app_utils.h"

typedef unsigned char      uint8_t;
//#define OLED_MODE_0_96 //0.96'
#define OLED_MODE_1_3   //1.3'

#ifdef OLED_MODE_0_96
	#define OLED_PAGE_SIZES		128
#endif
#ifdef OLED_MODE_1_3
	#define OLED_PAGE_SIZES		132
#endif



/**
 * @brief 1v偏压控制
 * 
 * @param lvl 1=3.3v，0=0v
 */
void ctrl_set(unsigned short);

/**
 * @brief 在OLED屏幕中间位置显示消息，仅支持英文数字
 * 
 * @param line 行数，0-2
 * @param context 需显示的消息，char*
 */
void _disp(byte line, void *context, byte mode);
/**
 * @brief 所选行数白屏 
 * 
 * @param start 开始行数（0-7）
 * @param end 结束行数（1-8）
 */
void OLED_Full(byte start, byte end);
void OLED_Refresh_GRAM( void );
#endif