# ST7789-STM32
Using STM32's Hardware SPI(with simple DMA support) to drive a ST7789 based LCD display.

# Table of Contents
* 1. [How to use ?](#Howtouse)
* 2. [SPI Interface](#SPIInterface)
* 3. [Supported Displays](#SupportedDisplays)
* 4. [Why this fork exist?](#Whythisforkexist)
* 5. [How to create an RLE compressed image?](#HowtocreateanRLEcompressedimage)
* 6. [HAL SPI Performance](#HALSPIPerformance)
		* 6.1. [Reference](#Reference)
		* 6.2. [Contributor](#Contributor)

##  1. <a name='Howtouse'></a>How to use ?

1. Copy the "st7789" dir to your project src path, add it to include path   
2. Include `"st7789.h"` in where you want to use this driver.   
3. Configure parameters in `"st7789.h"` according to your own display panel  
4. In system startup, perform `ST7789_Init();`.  
5. Run a `ST7789_Test()` to exam this driver.  
6. Don't forget to turn the backlight on  

This code has been tested on 240x240 & 170x320 LCD screens.

> DMA is only useful when huge block write is performed, e.g: Fill full screen or draw a bitmap.  
> Most MCUs don't have a large enough RAM, so a framebuffer is "cut" into pieces, e.g: a 240x5 pixel buffer for a 240x240 screen.  

##  2. <a name='SPIInterface'></a>SPI Interface

If you are using **Dupont Line(or jumper wire)**, please notice that your CLK frequency should not exceed 40MHz (may vary, depends on the length of your wire), **otherwise data transfer will collapse!**  
For higher speed applications, it's recommended to **use PCB** rather than jumper wires.  

In STM32CubeMX/CubeIDE, config the SPI params as follow:

![spi](fig/spi.jpg)

I've had a simple test, connect the screen and mcu via 20cm dupont line, and it works normally on **21.25MB/s**. And if I connect a logic analyzer to the clk and data lines(15cm probe), **21.25MB/s doesn't work anymore**, I have to lower its datarate to 10.625MB/s. Using PCB to connect the display, it works up to **40MB/s** and still looks nice.

##  3. <a name='SupportedDisplays'></a>Supported Displays

- 135*240   
- 240*240   
- 170*320 (new)  

If you like, you could customize it's resolution to drive different displays you prefer. 
> For example, a 240x320 display is perfectly suited for st7789.  
> Just set all X_SHIFT and Y_SHIFT to 0, and set resolution to 240|320.  

For more details, please refer to ST7789's datasheet.  

##  4. <a name='Whythisforkexist'></a>Why this fork exist?
For my final bachelor project, I needed a library to drive an [DFR0664](https://wiki.dfrobot.com/2.0_Inches_320_240_IPS_TFT_LCD_Display_with_MicroSD_Card_Breakout_SKU_DFR0664) display of 240 by 320 pixels with the famous ST7789 driver.
However, I found that the library was really slow even when using DMA...
So I decided to optimize it. For example, the characters were drawn pixel by pixel with a complexity of O(n^2) and now the library fills a buffer and everything is sent at the same time to the screen, reducing the complexity to O(n).

I've also added a function for drawing images compressed in RLE format. This makes it possible to have images that take up less space in flash memory.

Finally, I implemented a `wmemset` function. This function is used to fill an area of the screen with a color, speeding up the fill time.


##  5. <a name='HowtocreateanRLEcompressedimage'></a>How to create an RLE compressed image?
1. Used this tool to generate an hardcoded image. [LCD-Image-converter](https://lcd-image-converter.riuson.com/en/about/) you can use the default parameters.
2. Use the `compressIMG.py` script to convert your .h image.
3. Add your RLE image in your project and draw it with `ST7789_DrawImageComp` function.


##  6. <a name='HALSPIPerformance'></a>HAL SPI Performance

- DMA Enabled

With DMA enabled, cpu won't participate in the data transfer process. So filling a large size of data block is much faster.e.g. fill, drawImage. (You can see no interval between each data write)

![DMA](/fig/fill_dma.png)


- DMA Disabled

Without DMA enabled, the filling process could be a suffer. As you can see, before each data byte write, an interval is inserted, so the total datarate would degrade. 

![noDMA](/fig/fill_normal.png)

Especially in some functions where need a little math, the cpu needs to calculate data before a write operation, so the effective datarate would be much lower.(e.g. drawLine)

![line](fig/draw_line.png)


# Special thanks to

####  6.1. <a name='Reference'></a>Reference
- [ananevilya's Arduino-ST7789-Lib](https://github.com/ananevilya/Arduino-ST7789-Library)  
- [afiskon's stm32-st7735 lib](https://github.com/afiskon/stm32-st7735)

####  6.2. <a name='Contributor'></a>Contributor
- [JasonLrh](https://github.com/JasonLrh)  
- [ZiangCheng](https://github.com/ZiangCheng)  
