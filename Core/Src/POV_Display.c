
/*******************************************************************************************************
 *  [FILE NAME]   :      <POV_Display.c>                                                               *
 *  [AUTHOR]      :      <David S. Alexander>                                                          *
 *  [DATE CREATED]:      <Jan 19, 2024>                                                                *
 *  [Description} :      <Source file for POV Display driver>                                          *
 *******************************************************************************************************/

#include "POV_Display.h"
#include <stdlib.h>

volatile uint32_t TimeDifference;
volatile uint16_t Capture;
volatile uint16_t ICU_TIM2_OVC   = 0;
volatile uint8_t  POV_Digits     = 0;
volatile uint8_t  PixelsCounter  = 0;
volatile uint8_t  PovDisplayData[RESOLUTION];
uint8_t           CursPos        = 0;
uint8_t           PixelPos       = 0;
uint8_t           POVDigits      = (RESOLUTION / (FONTSIZE + 1));
uint8_t           sysClockFreq;

/**
  * @brief Displays an interval on the POV Display.
  *
  * This function updates the POV Display with the specified value by setting the corresponding GPIO pins.
  *
  * @param valueToPresent: The 8-bit value to be displayed on the POV Display.
  */
static void POV_IntervalsDisplay(uint8_t valueToPresent)
{
    uint8_t PixelsCount = 0;

    /* Iterate through each pixel and set or reset the corresponding GPIO pin based on the valueToPresent */
    for (; PixelsCount < PIXELS; PixelsCount++)
    {
        HAL_GPIO_WritePin(
            POV_Pins.POV_Ports[PixelsCount],
            POV_Pins.POV_Pins[PixelsCount],
            (valueToPresent & (1 << PixelsCount)) ? GPIO_PIN_SET : GPIO_PIN_RESET
        );
    }
}

/**
  * @brief Sets the interrupt period for TIM3.
  *
  * This function calculates the auto-reload value for TIM3 based on the desired interrupt period in microseconds
  * and sets it accordingly. It also resets the TIM3 counter to zero.
  *
  * @param desiredPeriodMicroseconds: The desired interrupt period for TIM3 in microseconds.
  */
static void setTIM3InterruptPeriod(uint16_t desiredPeriodMicroseconds)
{
    /* Calculate auto-reload value based on desired interrupt period and system clock frequency */
    uint16_t autoReloadValue = (desiredPeriodMicroseconds * (uint16_t)sysClockFreq) - 1;

    /* Reset TIM3 counter to zero */
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    /* Set the auto-reload value for TIM3 */
    __HAL_TIM_SET_AUTORELOAD(&htim3, autoReloadValue);
}

/**
  * @brief Initializes the POV Display timers and counters.
  *
  * This function configures the necessary timers and counters for the POV Display.
  * It starts the base timer of TIM2 and enables the interrupt, starts input capture for TIM2 Channel 1,
  * and starts the base timer of TIM3. The function also sets the prescaler for TIM2 based on the system clock frequency.
  * Additionally, it initializes variables related to system clock frequency, the number of POV digits, cursor position,
  * pixel position, and pixels counter.
  */
void POV_Init(void)
{
    /* Start TIM2 base and enable interrupt */
    HAL_TIM_Base_Start_IT(&htim2);

    /* Start TIM2 input capture for Channel 1 and enable interrupt */
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

    /* Start TIM3 base */
    HAL_TIM_Base_Start_IT(&htim3);

    /* Calculate system clock frequency in MHz */
    sysClockFreq = (uint8_t)(HAL_RCC_GetSysClockFreq() / 1000000);

    /* Set the prescaler for TIM2 */
    __HAL_TIM_SET_PRESCALER(&htim2, (sysClockFreq - 1));

    /* Initialize POV Display variables */
    CursPos = 0;
    PixelPos = 0;
    PixelsCounter = 0;
}

/**
  * @brief Writes a character to the POV Display.
  *
  * This function takes an input character and presents it on the POV Display.
  * It uses a predefined font to map the character to pixel data and updates the display data accordingly.
  * The cursor position and pixel position are also updated to prepare for the next character.
  *
  * @param Chr: The 8-bit variable representing the character to be displayed.
  */
void POV_WriteChar(uint8_t Chr)
{
    uint8_t PixelsCount = 0;

    /* Calculate the starting pixel position for the current cursor position */
    PixelPos = CursPos * (FONTSIZE + 1);

    /* Copy pixel data from the font to the display data */
    for (; PixelsCount < FONTSIZE; PixelsCount++)
    {
        PovDisplayData[PixelPos] = POV_Font[Chr - 32][PixelsCount];
        PixelPos = (PixelPos + 1) % RESOLUTION;
    }

    /* Add a blank pixel after each character (save one index in the font array) */
    PovDisplayData[PixelPos] = 0x00;

    /* Update cursor position for the next character */
    CursPos = (CursPos + 1) % POVDigits;
}

/**
  * @brief Writes a character at a specific position on the POV Display.
  *
  * This function writes the specified character at the given position on the POV Display.
  * It checks if the position is within the valid range before setting the cursor and writing the character.
  *
  * @param Chr: The 8-bit variable representing the character to be displayed.
  * @param Pos: The position at which to display the character.
  */
void POV_WriteCharInPos(uint8_t Chr, uint8_t Pos)
{
    /* Check if the position is within the valid range */
    if ((RESOLUTION / (FONTSIZE + 1)) > Pos)
    {
        /* Set the cursor to the specified position and write the character */
        POV_SetCursor(Pos);
        POV_WriteChar(Chr);
    }
}

/**
  * @brief Sets the cursor position on the POV Display.
  *
  * This function sets the cursor position on the POV Display based on the provided position.
  * It checks if the position is within the valid range before updating the cursor and pixel positions.
  *
  * @param Pos: The desired cursor position.
  */
void POV_SetCursor(uint8_t Pos)
{
    /* Check if the position is within the valid range */
    if ((RESOLUTION / (FONTSIZE + 1)) > Pos)
    {
        /* Update the cursor and pixel positions based on the specified position */
        CursPos = Pos;
        PixelPos = CursPos * (FONTSIZE + 1);
    }
}

/**
  * @brief Clears the POV Display.
  *
  * This function clears the entire POV Display by setting all pixel data to 0x00.
  * It also resets the pixel and cursor positions to the starting positions.
  */
void POV_Clear(void)
{
    uint8_t PixelsCount = 0;

    /* Set all pixel data to 0x00 to clear the display */
    for (; PixelsCount < RESOLUTION; PixelsCount++)
    {
        PovDisplayData[PixelsCount] = 0x00;
    }

    /* Reset pixel and cursor positions to the starting positions */
    PixelPos = 0;
    CursPos = 0;
}

/**
  * @brief Writes a pixel at a specific position on the POV Display.
  *
  * This function writes the specified pixel state (ON or OFF) at the given row and column positions on the POV Display.
  * It checks if the provided state is valid, and if the row and column positions are within the display's limits.
  *
  * @param Row: The row position of the pixel (the bit index of the colum).
  * @param Column: The column position of the pixel (the array index).
  * @param State: The state of the pixel (ON or OFF).
  */
void POV_WritePixel(uint8_t Row, uint8_t Column, uint8_t State)
{
    /* Check if the provided state is valid and if row and column positions are within the display's limits */
    if ((State == ON || State == OFF) && PIXELS > Row && RESOLUTION > Column)
    {
        /* Update the pixel state based on the specified state and positions */
        if (State == ON)
        {
        	/* Set the specified bit */
            PovDisplayData[Column] |= (1 << Row);
        }
        else
        {
        	/* Clear the specified bit */
            PovDisplayData[Column] &= ~(1 << Row);
        }
    }
}

/**
  * @brief Inverts the entire POV Display.
  *
  * This function inverts the state of each pixel on the POV Display by bitwise negating the current state.
  */
void POV_InvertDisplay(void)
{
    uint8_t PixelsCount = 0;

    /* Invert the state of each pixel on the POV Display */
    for (; PixelsCount < RESOLUTION; PixelsCount++)
    {
        PovDisplayData[PixelsCount] = ~PovDisplayData[PixelsCount];
    }
}

/**
  * @brief Writes a string to the POV Display.
  *
  * This function writes a string of characters to the POV Display by calling POV_WriteChar for each character in the string.
  *
  * @param Str: The null-terminated string to be displayed on the POV Display.
  */
void POV_WriteString(const uint8_t *Str)
{
    /* Iterate through each character in the string and write it to the POV Display */
    while (*Str != '\0')
    {
        POV_WriteChar(*Str);
        Str++;
    }
}

/**
  * @brief Writes a string at a specific position on the POV Display.
  *
  * This function sets the cursor position to the specified position and writes a null-terminated string to the POV Display.
  *
  * @param Str: The null-terminated string to be displayed on the POV Display.
  * @param Pos: The position at which to start displaying the string.
  */
void POV_WriteStringInPos(const uint8_t *Str, uint8_t Pos)
{
    /* Set the cursor position to the specified position */
    POV_SetCursor(Pos);
    /* Write the string to the POV Display */
    POV_WriteString(Str);
}

/**
  * @brief Draws a bitmap on the POV Display.
  *
  * This function copies the pixel data from the provided bitmap to the POV Display.
  * It checks if the bitmap pointer is not NULL and if the size of the bitmap matches or lower than the resolution of the display.
  *
  * @param MyBitmap: Pointer to the bitmap data to be displayed.
  * @param BitmapSize: The size of the bitmap data.
  */
void POV_DrawBitmap(const uint8_t *MyBitmap, uint8_t BitmapSize)
{
    /* Check if the bitmap pointer is not NULL and if the size matches or lower than the resolution of the display */
    if (MyBitmap != NULL && BitmapSize <= RESOLUTION)
    {
        uint8_t PixelsCount = 0;

        /* Copy the pixel data from the bitmap to the POV Display */
        for (; PixelsCount < BitmapSize; PixelsCount++)
        {
            PovDisplayData[PixelsCount] = MyBitmap[PixelsCount];
        }
    }
}

/**
  * @brief Draws a frame on the POV Display.
  *
  * This function draws a rectangular frame on the POV Display using the specified column and row coordinates.
  * It ensures that the provided coordinates are within the valid display bounds before updating the pixel data.
  *
  * @param Column1: The starting column of the frame.
  * @param Row1: The starting row of the frame.
  * @param Row2: The ending row of the frame.
  * @param Column2: The ending column of the frame.
  */
void POV_DrawFrame(uint8_t Column1, uint8_t Row1, uint8_t Row2, uint8_t Column2)
{
    /* Check if the coordinates are within the valid display bounds */
    if (PIXELS > Row2 && Row2 > Row1 && RESOLUTION > Column1 && RESOLUTION > Column2)
    {
        uint8_t PixelsCountColumn = Column1, PixelsCountRow = Row1;

        /* Iterate through the columns of the frame */
        for (; PixelsCountColumn <= Column2; PixelsCountColumn++)
        {
            /* Set pixels in the specified rows and columns to create the frame */
            PovDisplayData[PixelsCountColumn] |= (1 << Row1) | (1 << Row2);

            /* If it's the first or last column, set pixels in all rows between Row1 and Row2 */
            if (PixelsCountColumn == Column1 || PixelsCountColumn == Column2)
            {
                for (PixelsCountRow = Row1; PixelsCountRow <= Row2; PixelsCountRow++)
                {
                    PovDisplayData[PixelsCountColumn] |= (1 << PixelsCountRow);
                }
            }
        }
    }
}

/**
  * @brief Draws a line on the POV Display.
  *
  * This function draws a line on the POV Display using the specified starting and ending column and row coordinates.
  * It ensures that the provided coordinates are within the valid display bounds before updating the pixel data.
  * It uses Bresenham's line algorithm.
  *
  * @param Column1: The starting column of the line.
  * @param Row1: The starting row of the line.
  * @param Column2: The ending column of the line.
  * @param Row2: The ending row of the line.
  */
void POV_DrawLine(uint8_t Column1, uint8_t Row1, uint8_t Column2, uint8_t Row2)
{
    /* Ensure rows and columns are within bounds */
    if (Row1 >= PIXELS || Column1 >= RESOLUTION || Row2 >= PIXELS || Column2 >= RESOLUTION)
    {
        /* Handle invalid input */
        return;
    }

    /* Calculate differences and initialize variables for Bresenham's line algorithm */

    /* dx: The absolute difference in column positions between the two end points */
    int8_t dx = abs(Column2 - Column1);

    /* sx: The sign of the change in the x-direction (column) for incrementing or decrementing */
    int8_t sx = Column1 < Column2 ? 1 : -1;

    /* dy: The absolute difference in row positions between the two end points */
    int8_t dy = abs(Row2 - Row1);

    /* sy: The sign of the change in the y-direction (row) for incrementing or decrementing */
    int8_t sy = Row1 < Row2 ? 1 : -1;

    /* err: The error term used in Bresenham's line algorithm, initialized based on the larger of dx and dy */
    int8_t err = (dx > dy ? dx : -dy) / 2;

    /* e2: Temporary variable to store the current error term during iteration */
    int8_t e2;


    /* Iterate through the pixels along the line and set their state */
    while (1)
    {
        /* Set the state of the current pixel to the specified value (ON or OFF) */
        POV_WritePixel(Row1, Column1, ON);

        /* Check if the end of the line is reached */
        if (Row1 == Row2 && Column1 == Column2)
        {
            break;
        }

        /* Update the error term (e2) with the current error value */
        e2 = err;

        /* Adjust the error term and move to the next column if applicable */
        if (e2 > -dx)
        {
            err -= dy;
            /* Move to the next column in the specified direction */
            Column1 += sx;
        }

        /* Adjust the error term and move to the next row if applicable */
        if (e2 < dy)
        {
            err += dx;
            /* Move to the next row in the specified direction */
            Row1 += sy;
        }
    }
}

/**
  * @brief Draws a triangle on the POV Display.
  *
  * This function connects the vertices of the triangle by drawing three lines using the POV_DrawLine function.
  *
  * @param Column1: The column position of the first vertex.
  * @param Row1: The row position of the first vertex.
  * @param Column2: The column position of the second vertex.
  * @param Row2: The row position of the second vertex.
  * @param Column3: The column position of the third vertex.
  * @param Row3: The row position of the third vertex.
  */
void POV_DrawTriangle(uint8_t Column1, uint8_t Row1, uint8_t Column2, uint8_t Row2, uint8_t Column3, uint8_t Row3)
{
    /* Draw three lines connecting the vertices of the triangle */
    POV_DrawLine(Column1, Row1, Column2, Row2);
    POV_DrawLine(Column2, Row2, Column3, Row3);
    POV_DrawLine(Column3, Row3, Column1, Row1);
}

/**
 * @brief Writes a value to the specified column in the POV display data.
 *
 * @param Column The column index where the value is to be written.
 * @param Value The value to be written to the specified column.
 *
 * @note Ensures that the column index is within bounds before performing the write operation.
 * @note If the column index is out of bounds, the function returns without modifying the data.
 */
void POV_WriteColumn(uint8_t Column, uint8_t Value)
{
    /* Ensure column is within bounds */
    if (Column >= RESOLUTION)
    {
        /* Handle invalid input */
        return;
    }

    PovDisplayData[Column] = Value;
}

/**
 * @brief Reads the value stored in the specified column of the POV display data.
 *
 * @param Column The column index from which to read the value.
 *
 * @return The value stored in the specified column.
 *
 * @note Ensures that the column index is within bounds before performing the read operation.
 * @note If the column index is out of bounds, the function returns 0.
 */
uint8_t POV_ReadColumn(uint8_t Column)
{
    /* Ensure column is within bounds */
    if (Column >= RESOLUTION)
    {
        /* Handle invalid input */
        return 0;
    }

    return PovDisplayData[Column];
}

/**
 * @brief Reads the state of the pixel at the specified row and column in the POV display data.
 *
 * @param Row The row index of the pixel.
 * @param Column The column index of the pixel.
 *
 * @return The state of the pixel (ON or OFF).
 *
 * @note Ensures that both the column and row indices are within bounds before performing the read operation.
 * @note If either the column or row index is out of bounds, the function returns 0.
 */
uint8_t POV_ReadPixel(uint8_t Row, uint8_t Column)
{
    /* Ensure column and row are within bounds */
    if (Column >= RESOLUTION || Row >= PIXELS)
    {
        /* Handle invalid input */
        return 0;
    }

    return ((PovDisplayData[Column] >> Row) & ON);
}



/**
  * @brief Callback function for TIM3 period elapsed interrupt.
  *
  * This function is called when the period of TIM3 elapses. It increments the PixelsCounter,
  * displays the pixel value corresponding to the current counter, and toggles the GPIO pin GPIOC_PIN_13.
  * If the counter exceeds the resolution, the display is completed.
  *
  * @param htim: Pointer to the TIM_HandleTypeDef structure that contains the configuration information for TIM3.
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* Check if the interrupt is triggered by TIM3 */
    if (htim->Instance == TIM3)
    {
        /* Increment the counter tracking the displayed pixels */
        PixelsCounter++;

        /* Check if there are more pixels to display */
        if (PixelsCounter < RESOLUTION)
        {
            /* Display the pixel value corresponding to the current counter */
            POV_IntervalsDisplay(PovDisplayData[PixelsCounter]);

            /* Toggle the GPIO pin (for debugging/visualization purposes) */
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
        else
        {
            /* Nothing to do */
        }
    }
    /* Check if the interrupt is triggered by TIM2 */
    else if (htim->Instance == TIM2)
    {
        /* Increment the overflow counter for TIM2 */
        ICU_TIM2_OVC++;
    }
}


/**
  * @brief Callback function for TIM2 input capture interrupt.
  *
  * This function is called when an input capture event occurs on TIM2.
  * It calculates the time difference between consecutive capture events, sets the period for TIM3 interrupts,
  * and resets relevant counters and registers for further measurements.
  *
  * @param htim: Pointer to the TIM_HandleTypeDef structure that contains the configuration information for TIM2.
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	/* Check if the interrupt is triggered by TIM2 */
    if (htim->Instance == TIM2)
    {
    	/* Reset the pixel counter */
        PixelsCounter = 0;

        /* Display the pixel value corresponding to the current counter */
        POV_IntervalsDisplay(PovDisplayData[PixelsCounter]);

        /* Read the captured value and calculate the time difference */
        Capture = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);
        TimeDifference = ((uint32_t)Capture + ((uint32_t)ICU_TIM2_OVC * 65536));

        /* Calculate the period for TIM3 interrupts based on the time difference */
        uint16_t Period = (uint16_t)(TimeDifference / RESOLUTION);
        /* Set the new intervals Period */
        setTIM3InterruptPeriod(Period);

        /* Reset the overflow counter for TIM2 */
        ICU_TIM2_OVC = 0;
        /* Reset the counter register for TIM2 */
        __HAL_TIM_SET_COUNTER(&htim2, 0);
    }
}
