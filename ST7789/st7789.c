#include "st7789.h"
#include "color.h"

#define USE_DMA

//Modifies the SPI clk when another slow device is present on the same line, such as an SD card.
#define FCLK_FASTER() { MODIFY_REG(hspi1.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_2); }	/* Set SCLK = fast, approx 32 MBits/s */

#ifdef USE_DMA
#include <string.h>
uint16_t DMA_MIN_SIZE = 4;
/* If you're using DMA, then u need a "framebuffer" to store datas to be displayed.
 * If your MCU don't have enough RAM, please avoid using DMA(or set 5 to 1).
 * And if your MCU have enough RAM(even larger than full-frame size),
 * Then you can specify the framebuffer size to the full resolution below.
 */
#define HOR_LEN 	60	//	Alse mind the resolution of your screen!
uint16_t disp_buf[ST7789_WIDTH * HOR_LEN];
#endif

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
	FCLK_FASTER();
	ST7789_Select();
	ST7789_DC_Clr();
	HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, &cmd, sizeof(cmd));

	ST7789_UnSelect();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
	FCLK_FASTER();
	ST7789_Select();
	ST7789_DC_Set();

	// split data in small chunks because HAL can't send more than 64K at once

	while (buff_size > 0)
	{
		uint16_t chunk_size = buff_size > 0xFFFF ? 0xFFFF : buff_size;
#ifdef USE_DMA
		HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);

		while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY)
			;

#else
//			/*TODO*/ HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size,  HAL_MAX_DELAY);
			LL_SPI_TransmitData8(&ST7789_SPI_PORT, *buff);
		#endif
		buff += chunk_size;
		buff_size -= chunk_size;
	}

	ST7789_UnSelect();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
	FCLK_FASTER();
	ST7789_Select();
	ST7789_DC_Set();
	/*TODO*/HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), /*TODO*/
	HAL_MAX_DELAY);
//	LL_SPI_TransmitData8(&ST7789_SPI_PORT, data);
	ST7789_UnSelect();
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m)
	{
	case 0:
		ST7789_WriteSmallData(
		ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(
		ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(
		ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET);
	{
		uint8_t data[] =
		{ x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF };
		ST7789_WriteData(data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] =
		{ y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF };
		ST7789_WriteData(data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void ST7789_Init(void)
{
#ifdef USE_DMA
	memset(disp_buf, 0, sizeof(disp_buf));
#endif
	/*TODO*/HAL_Delay(25);
	ST7789_RST_Clr();
	/*TODO*/HAL_Delay(25);
	ST7789_RST_Set();
	/*TODO*/HAL_Delay(50);

	ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
	ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
	ST7789_WriteCommand(0xB2);				//	Porch control
	{
		uint8_t data[] =
		{ 0x0C, 0x0C, 0x00, 0x33, 0x33 };
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)

	/* Internal LCD Voltage generator settings */
	ST7789_WriteCommand(0XB7);				//	Gate Control
	ST7789_WriteSmallData(0x35);			//	Default value
	ST7789_WriteCommand(0xBB);				//	VCOM setting
	ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
	ST7789_WriteCommand(0xC0);				//	LCMCTRL
	ST7789_WriteSmallData(0x2C);			//	Default value
	ST7789_WriteCommand(0xC2);				//	VDV and VRH command Enable
	ST7789_WriteSmallData(0x01);			//	Default value
	ST7789_WriteCommand(0xC3);				//	VRH set
	ST7789_WriteSmallData(0x12);		//	+-4.45v (defalut +-4.1v for 0x0B)
	ST7789_WriteCommand(0xC4);				//	VDV set
	ST7789_WriteSmallData(0x20);			//	Default value
	ST7789_WriteCommand(0xC6);			//	Frame rate control in normal mode
	ST7789_WriteSmallData(0x0F);			//	Default value (60HZ)
	ST7789_WriteCommand(0xD0);				//	Power control
	ST7789_WriteSmallData(0xA4);			//	Default value
	ST7789_WriteSmallData(0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(0xE0);
	{
		uint8_t data[] =
		{ 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23 };
		ST7789_WriteData(data, sizeof(data));
	}

	ST7789_WriteCommand(0xE1);
	{
		uint8_t data[] =
		{ 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23 };
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_WriteCommand(ST7789_INVON);		//	Inversion ON
	ST7789_WriteCommand(ST7789_SLPOUT);	//	Out of sleep mode
	ST7789_WriteCommand(ST7789_NORON);		//	Normal Display on
	ST7789_WriteCommand(ST7789_DISPON);	//	Main screen turned on

	/*TODO*/HAL_Delay(50);
	HAL_TIM_PWM_Start(&ST7789_PWM_TIM, TIM_CHANNEL_1);
#ifdef CHxN
	HAL_TIMEx_PWMN_Start(&ST7789_PWM_TIM, ST7789_PWM_CH);
	HAL_TIMEx_OCN_Start(&ST7789_PWM_TIM, TIM_CHANNEL_1);	//Enable Timer Complementary Output Compare functions to use CHxN
#endif
	ST7789_Fill_Color(BLACK);
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
	uint16_t i;
	ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
	ST7789_Select();

#ifdef USE_DMA
	for (i = 0; i < ST7789_HEIGHT / HOR_LEN; i++)
	{
		wmemset(disp_buf, WINVERT(color), sizeof(disp_buf));
		ST7789_WriteData(disp_buf, sizeof(disp_buf));
	}
#else
		uint16_t j;
		for (i = 0; i < ST7789_WIDTH; i++)
				for (j = 0; j < ST7789_HEIGHT; j++) {
					uint8_t data[] = {color >> 8, color & 0xFF};
					ST7789_WriteData(data, sizeof(data));
				}
	#endif
	ST7789_UnSelect();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x < 0) || (x >= ST7789_WIDTH) || (y < 0) || (y >= ST7789_HEIGHT))
		return;

	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] =
	{ color >> 8, color & 0xFF };
	ST7789_Select();
	ST7789_WriteData(data, sizeof(data));
	ST7789_UnSelect();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	xEnd = xEnd >= ST7789_WIDTH ? ST7789_WIDTH - 1 : xEnd;
	yEnd = yEnd >= ST7789_HEIGHT ? ST7789_HEIGHT - 1 : yEnd;
	if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) || (yEnd < 0) || (yEnd >= ST7789_HEIGHT))
		return;
	uint16_t datas[(yEnd - ySta) * (xEnd - xSta + 1)];

	ST7789_Select();

	ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
	wmemset(datas, WINVERT(color), sizeof(datas));
	ST7789_WriteData((uint8_t*) datas, sizeof(datas));
	ST7789_UnSelect();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x <= 0) || (x > ST7789_WIDTH) || (y <= 0) || (y > ST7789_HEIGHT))
		return;
	ST7789_Select();
	ST7789_Fill(x - 1, y - 1, x + 1, y + 1, color);
	ST7789_UnSelect();
}

#ifdef BL_PWM
void ST7789_SetBacklight(uint8_t duty)
{
#ifdef CHxN
	__HAL_TIM_SET_COMPARE(&ST7789_PWM_TIM, ST7789_PWM_CH, 255 - duty);
#else
	__HAL_TIM_SET_COMPARE(&ST7789_PWM_TIM, ST7789_PWM_CH, duty);
#endif

}
#elif BL_BIN
void ST7789_SetBacklight(uint8_t stat)
{
	HAL_GPIO_WritePin(TFT_BL_GPIO_Port, TFT_BL_Pin, stat);
}
#endif

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	uint16_t swap;
	uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
	if (steep)
	{
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
		//_swap_int16_t(x0, y0);
		//_swap_int16_t(x1, y1);
	}

	if (x0 > x1)
	{
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
		//_swap_int16_t(x0, x1);
		//_swap_int16_t(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = ABS(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			ST7789_DrawPixel(y0, x0, color);
		}
		else
		{
			ST7789_DrawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{

	ST7789_Select();
	ST7789_DrawLine(x1, y1, x2, y1, color);
	ST7789_DrawLine(x1, y1, x1, y2, color);
	ST7789_DrawLine(x1, y2, x2, y2, color);
	ST7789_DrawLine(x2, y1, x2, y2, color);
	ST7789_UnSelect();
}

/** 
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_Select();
	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawPixel(x0 + x, y0 + y, color);
		ST7789_DrawPixel(x0 - x, y0 + y, color);
		ST7789_DrawPixel(x0 + x, y0 - y, color);
		ST7789_DrawPixel(x0 - x, y0 - y, color);

		ST7789_DrawPixel(x0 + y, y0 + x, color);
		ST7789_DrawPixel(x0 - y, y0 + x, color);
		ST7789_DrawPixel(x0 + y, y0 - x, color);
		ST7789_DrawPixel(x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
		return;
	if ((x + w - 1) >= ST7789_WIDTH)
		return;
	if ((y + h - 1) >= ST7789_HEIGHT)
		return;

	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	ST7789_WriteData((uint8_t*) data, sizeof(uint16_t) * w * h);
	ST7789_UnSelect();
}

/**
 * @brief Draw a compressed image with Run-Length Encoding (RLE). If isTransparent is set, all black are replaced by bcolor.
 *
 * @param x
 * @param y
 * @param w
 * @param h
 * @param data
 * @param num
 * @param bcolor
 * @param isTransparent
 */
void ST7789_DrawImageComp(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data, uint16_t num, uint16_t bcolor, uint16_t isTransparent)
{
	uint16_t image[w * h];	//decompressed image
	uint32_t imageI = 0;	//image index
	uint16_t n;
	uint16_t color;

	for (uint16_t i = 0; i < num; ++i)
	{
		color = data[i];	//get color
		++i;
		n = data[i];	//get number of repetition

		if (color == 0x00 && isTransparent)	//Transparency support
		{
			wmemset((uint16_t*) (image + imageI), WINVERT(bcolor), n * sizeof(uint16_t));	//set color to memory range, better than loop ;D
		}
		else
		{
			wmemset((uint16_t*) (image + imageI), color, n * sizeof(uint16_t));	//set color to memory range, better than loop ;D
		}
		imageI += n;
	}
	ST7789_DrawImage(x, y, w, h, image);
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{
	ST7789_Select();
	ST7789_WriteCommand(invert ? 0x21 /* INVON */: 0x20 /* INVOFF */);
	ST7789_UnSelect();
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
	uint32_t i, b, j;
	ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);
	//	uint8_t data[2];
	//	uint8_t charDatas[font.width][2];
	uint8_t charDatas[font.height][font.width][2];

	for (i = 0; i < font.height; i++)
	{
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++)
		{
			if ((b << j) & 0x8000)
			{
				charDatas[i][j][0] = color >> 8;
				charDatas[i][j][1] = color & 0xFF;

			}
			else
			{
				charDatas[i][j][0] = bgcolor >> 8;
				charDatas[i][j][1] = bgcolor & 0xFF;
			}
		}
	}

	ST7789_WriteData((uint8_t*) charDatas, sizeof(charDatas));

}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
	ST7789_Select();
	while (*str)
	{
		if (x + font.width >= ST7789_WIDTH)
		{
			x = 0;
			y += font.height;
			if (y + font.height >= ST7789_HEIGHT)
			{
				break;
			}

			if (*str == ' ')
			{
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		ST7789_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	ST7789_Select();
	uint8_t i;

	/* Check input parameters */
	if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT)
	{
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= ST7789_WIDTH)
	{
		w = ST7789_WIDTH - x;
	}
	if ((y + h) >= ST7789_HEIGHT)
	{
		h = ST7789_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++)
	{
		/* Draw lines */
		ST7789_DrawLine(x, y + i, x + w, y + i, color);
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	/* Draw lines */
	ST7789_DrawLine(x1, y1, x2, y2, color);
	ST7789_DrawLine(x2, y2, x3, y3, color);
	ST7789_DrawLine(x3, y3, x1, y1, color);
	ST7789_UnSelect();
}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1)
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	else
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1)
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	else
	{
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay)
	{
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else
	{
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		ST7789_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den)
		{
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
	ST7789_UnSelect();
}

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	ST7789_Select();
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);
	ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}

/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear)
{
	ST7789_Select();
	ST7789_WriteCommand(tear ? 0x35 /* TEON */: 0x34 /* TEOFF */);
	ST7789_UnSelect();
}

/**
 * @brief memset(...) function but for 16-bit pattern. Complexity is O(n) -> O(size)
 *
 * @param destination : Address to write pattern.
 * @param data		: Pattern 16-bit
 * @param size		: Size in number of byte to copy at destination.
 */
void wmemset(uint16_t *dest, uint16_t data, size_t len)
{
	uint16_t blockSize = sizeof(uint16_t);
	uint8_t *start = (uint8_t*) dest;
	uint8_t *current = (uint8_t*) dest;
	uint8_t *end = start + len;

	memcpy(dest, &data, blockSize);
	while (current + blockSize < end)
	{
		memcpy(dest + blockSize / 2, dest, blockSize);
		current += blockSize;
		blockSize *= 2;
	}
	// fill the rest
	if (end - current - 2 >= 0)
	{
		memcpy(dest + blockSize / 2, dest, end - current - 2);	//-2 because 16bit
	}
}

/** 
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
	ST7789_Fill_Color(WHITE);
	/*TODO*/HAL_Delay(1000);
	ST7789_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
	/*TODO*/HAL_Delay(1000);
	ST7789_Fill_Color(CYAN);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(RED);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(BLUE);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(GREEN);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(YELLOW);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(BROWN);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(DARKBLUE);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(MAGENTA);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(LIGHTGREEN);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(LGRAY);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(LBBLUE);
	/*TODO*/HAL_Delay(500);
	ST7789_Fill_Color(WHITE);
	/*TODO*/HAL_Delay(500);

	ST7789_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
	ST7789_WriteString(10, 50, "Hello Steve!", Font_7x10, RED, WHITE);
	ST7789_WriteString(10, 75, "Hello Steve!", Font_11x18, YELLOW, WHITE);
	ST7789_WriteString(10, 100, "Hello Steve!", Font_16x26, MAGENTA, WHITE);
	/*TODO*/HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
	/*TODO*/HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
	/*TODO*/HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawCircle(60, 60, 25, WHITE);
	/*TODO*/HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledCircle(60, 60, 25, WHITE);
	/*TODO*/HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, BLACK);
	ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
	/*TODO*/HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
	/*TODO*/HAL_Delay(1000);

	//	If FLASH cannot storage anymore datas, please delete codes below.
	ST7789_Fill_Color(WHITE);
	ST7789_DrawImage(0, 0, 128, 128, (uint16_t*) saber);
	/*TODO*/HAL_Delay(3000);
}
