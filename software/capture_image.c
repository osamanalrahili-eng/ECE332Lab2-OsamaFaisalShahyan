/*
 * ============================================================================
 *  DE1-SoC Lab: Video Capture + Image Processing (Baseline Provided)
 * ============================================================================
 *
 *  Learning Objectives:
 *   1) Map FPGA physical addresses into user virtual space using mmap()
 *   2) Understand page alignment and offset math
 *   3) Capture a 320x240 RGB565 frame from FPGA on-chip memory
 *   4) Perform software-based image processing
 *   5) Display text on VGA character buffer
 *   
 *
 *  NOTE:
 *   - This file provides the hardware access infrastructure.
 *   - Students must complete the TODO sections.
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* ============================================================================
 *  FPGA Physical Base Addresses
 *  (Verify using sopcinfo / Platform Designer)
 * ========================================================================== */

#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

#define PAGE_SIZE 4096UL
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define VIDEO_STRIDE 512 
#define IMG_WIDTH 320
#define IMG_HEIGHT 240
#define STRIDE 128

/* ============================================================================
 *  Memory Mapping Helper
 * ========================================================================== */

static void *map_span(int fd, off_t phys_addr, size_t span_bytes)
{
    off_t base = phys_addr & PAGE_MASK;
    off_t off  = phys_addr - base;

    size_t map_len =
        ((off + span_bytes + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

    void *v = mmap(NULL, map_len,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, base);

    if (v == MAP_FAILED)
        return NULL;
 
    return (uint8_t *)v + off;
}

static volatile uint32_t *map32(int fd, off_t phys_addr)
{
    return (volatile uint32_t *)map_span(fd, phys_addr, sizeof(uint32_t));
}

static volatile uint16_t *map16_span(int fd, off_t phys_addr, size_t span)
{
    return (volatile uint16_t *)map_span(fd, phys_addr, span);
}

static volatile uint8_t *map8_span(int fd, off_t phys_addr, size_t span)
{
    return (volatile uint8_t *)map_span(fd, phys_addr, span);
}


// Writes a string to the VGA character buffer
void VGA_text(volatile uint8_t *buf, int x, int y, const char *text)

{int i = 0;

    while (text[i] != '\0')

    {buf[y * STRIDE + x + i] = text[i];

        i++;}}

// Clears the VGA character buffer
void VGA_clear(volatile uint8_t *buf)

{int x,y;

    for ( y = 0; y < 60; y++) 

    {for ( x = 0; x < STRIDE; x++) 

        {buf[y * STRIDE + x] = ' ';}}}

/* ============================================================================
 *  Main
 *  When you run this code without any changes
 *  - it should stream video on to the monitor
 *  - first button press should freeze the video
 *  - second button press should exit the program
 * ========================================================================== */

 // Setting up a global mode variable


int main(void)

{printf("Opening /dev/mem...\n");

    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0) 

    {perror("open(/dev/mem)");

        return 1;}

    /* Map hardware regions */

    volatile uint32_t *KEY_ptr =

        map32(fd, KEY_BASE);

    volatile uint32_t *VID_reg3 =

        map32(fd, VIDEO_IN_BASE + 0xC);

    volatile uint16_t *Video_Mem_ptr =

        map16_span(fd, FPGA_ONCHIP_BASE,

                   512 * 240 * 2);  // stride 512, 240 rows

    volatile uint8_t *Char_buf_ptr =

        map8_span(fd, FPGA_CHAR_BASE,

                  128 * 60);

    if (!KEY_ptr || !VID_reg3 ||

        !Video_Mem_ptr || !Char_buf_ptr) {

        perror("mmap failed");

        close(fd);

        return 1;
    }

    printf("Memory mapping successful.\n");

    /* Enable video stream */
    *VID_reg3 = 0x4;

    int capture_count = 0;

    int mode = 0; // 0: video streaming, 1: capture + timestamp, 2: video streaming, 3: capture + processing, 4: video streaming

    uint16_t frame[240][320];

    uint16_t temp[240][320]; 

    char text[256];

    /* ============================================================
     * TODO 1:
     *   Understand how key button presson is captured and how we are enabling and disabling the video
     *   Modify this loop struture to enable/disbale the video and do image processing tasks

     *   IMPORTANT:
     *     - You can do this task, however you want, satisfying the requirments-
     *     - 1. Running the program will start the video on monitor
     *     - 2. first button press button press will stop video capture the image, show it on the monitor and show the time stamp on the image captured
     *     - 3. second button press, enable the video again
     *     - 4. third button press, stops video, capture image, do image processing taask and display the output image and the count of the captured image on monitor
     *     - 5. fourth button press, enables video stream on to monitor again and so on..........
     * ============================================================ */

     while (1)
    {
        
        if (*KEY_ptr != 0)                  // check if any KEY was pressed
        {

            while (*KEY_ptr != 0);  
            
            mode++; // wait for pushbutton KEY release


            if (mode == 1) 
               {*VID_reg3 = 0x0; // stop video stream

                int x,y;
                for (y = 0; y < IMG_HEIGHT; y++)

                {for (x=0; x < IMG_WIDTH; x++) {frame [y][x] = Video_Mem_ptr[(y << 9) + x];}}

                VGA_clear(Char_buf_ptr); // capture frame and display timestamp

                time_t now = time(NULL);

                char timestamp[64];

                strftime(timestamp, sizeof(timestamp), "Timestamp: %Y-%m-%d %H:%M:%S", localtime(&now));

                VGA_text(Char_buf_ptr, 2, 2, timestamp);} // Display timestamp on VGA

            else if (mode == 2) 

            {VGA_clear(Char_buf_ptr); // enable video stream

                *VID_reg3 = 0x4;} // enable video stream

            else if (mode == 3) 

            {   *VID_reg3 = 0x0; // stop video stream

                VGA_clear(Char_buf_ptr); // capture frame

                capture_count++;

                sprintf(text, "Captured frame count: %d", capture_count);
                
                VGA_text(Char_buf_ptr, 0, 0, text); // Display captured frame count on VGA
            

    
        
    /* ============================================================
     * TODO 2:
     *   Capture a 320x240 frame from FPGA video memory into
     *   frame[x][y]
     *

     *   IMPORTANT:
     *     - Memory stride is 512 pixels per row
     *     - So row y starts at (y * 512)
     *     -- Memory stride is 512 pixels per row (not 320)
     *     - Indexing: Video_Mem_ptr[y*512 + x] == Video_Mem_ptr[(y<<9) + x]
     *     - below code gives a sample code about how to read memory pixel by pixel
     *     - it is suggested to save each pixel value in to a image frame, to to each image processing task
     * ============================================================ */

                int x,y;

                for (y=0; y< 240; y++) 

                {for (x=0; x <320; x++) 
                    {frame [y][x] = Video_Mem_ptr[(y << 9) + x];}}}


            else if (mode == 4)

            {VGA_clear(Char_buf_ptr); // capture frame and display timestamp

                *VID_reg3 = 0x4;} // enable video stream
            
            else if (mode == 5) 

            {*VID_reg3 = 0x0; // stop video stream

            VGA_clear(Char_buf_ptr); // capture frame

            VGA_text(Char_buf_ptr, 0, 0, "Horizontal Flip"); // Display captured frame count on VGA
                
                int x,y;

                for (y=0; y< 240; y++) 

                    {for (x=0; x <320; x++) {frame[y][x] = Video_Mem_ptr[(y << 9) + x];}}

                for (y=0; y< 240; y++) 

                    {for (x=0; x <320; x++)

                        {temp[y][x] = frame[y][319-x];}} // Horizontal flip
                   

                for (y=0; y< 240; y++) 

                {for (x=0; x <320; x++) 

                {Video_Mem_ptr[y * VIDEO_STRIDE + x] = temp[y][x];}}} // Write back to memory
                

            else if (mode == 6) 

            {*VID_reg3 = 0x0; // stop video stream

            VGA_clear(Char_buf_ptr); // capture frame

            VGA_text(Char_buf_ptr, 0, 0, "Vertical Flip"); // Display captured frame count on VGA
                
                int x,y;

                for (y=0; y< 240; y++) 

                    {for (x=0; x <320; x++) 

                        {frame[y][x] = Video_Mem_ptr[(y << 9) + x];}}

                for (y=0; y< 240; y++) 

                {for (x=0; x <320; x++) 

                    {temp[y][x] = frame[239-y][x];}} // Vertical flip


                for (y=0; y< 240; y++) 

                {for (x=0; x <320; x++) 

                    {Video_Mem_ptr[y * VIDEO_STRIDE + x] = temp[y][x];}}} // Write back to memory

            else if (mode == 7) 

            {*VID_reg3 = 0x0; // stop video stream

            VGA_clear(Char_buf_ptr); // capture frame

            VGA_text(Char_buf_ptr, 0, 0, "Black and White");

                int x,y;

                for (y=0; y< 240; y++) 

                {for (x=0; x <320; x++) 
                    
                    {frame[y][x] = Video_Mem_ptr[(y << 9) + x];}}   


    /* ============================================================
     * TODO 3:
     *  Write a function that prints a string to VGA character 
     *  go through DE1-SoC_Computer_with_ARM_Cortex-A9.pdf provided on the course website
     *  we are using the hardware from Terasic, and it gives information on how to use their IP
     *  it has information about how to write characters on to the monitor  
     *
     *  HINT:
     *    - Visible size: 80x60
     *    - Memory stride: 128 bytes per row
     *    - Each character = 1 byte (ASCII)
     * ============================================================ */

    // Done in the VGA_text() function above

    /* ============================================================
     * TODO 4:
     *   Implement at least THREE image processing operations:
     *
     *     - Vertical flip
     *     - Horizontal flip
     *     - Threshold-based black/white
     *
     *   Apply the processed result back into Video_Mem_ptr
     *
     * ============================================================ */

                
                // Threshold-based black/white
                for (y=0; y< 240; y++) {
                    for (x=0; x <320; x++) {

                        uint16_t pixel = frame[y][x];

                        uint16_t r = (pixel >> 11) & 0x1F; // Extract red

                        uint16_t g = (pixel >> 5) & 0x3F;  // Extract green

                        uint16_t b = pixel & 0x1F;         // Extract blue

                        uint16_t gray = (r + g + b) / 3; // Simple average for grayscale
                        
                        if (gray > 10) 
                         
                            {frame[y][x] = 0xFFFF;} // White

                        else 

                            {frame[y][x] = 0x0000;} // Black


                        
                        Video_Mem_ptr[y * VIDEO_STRIDE + x] = frame[y][x];}}} // frame back to memory

            else if (mode == 8)
            
                {*VID_reg3 = 0x0;

                VGA_clear(Char_buf_ptr); // capture frame

                VGA_text(Char_buf_ptr, 0, 0, "Invert");

                int x,y;

                for (y=0; y< 240; y++) 
                
                    {for (x=0; x <320; x++) 

                        {frame[y][x] = Video_Mem_ptr[(y << 9) + x];}}

                for (y=0; y< 240; y++) 

                    {for (x=0; x <320; x++) 

                        {frame[y][x] = ~frame[y][x]; // Invert colors

                        Video_Mem_ptr[y * VIDEO_STRIDE + x] = frame[y][x];}}} // Write back to memory


            else if (mode == 9) 
            
                {VGA_clear(Char_buf_ptr); // enable video stream

                *VID_reg3 = 0x4; // enable video stream

                mode = 0;}}} // reset mode to start over

    close(fd);

    return 0;
}
