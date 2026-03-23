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


/* ============================================================================
 *  Main
 *  When you run this code without any changes
 *  - it should stream video on to the monitor
 *  - first button press should freeze the video
 *  - second button press should exit the program
 * ========================================================================== */

int main(void)
{
    printf("Opening /dev/mem...\n");

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

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
        
        
	    if (*KEY_ptr != 0)						// check if any KEY was pressed
	    {
		   *(VID_reg3) = 0x0;			    // Disable the video to capture one frame
		   while (*KEY_ptr != 0);				// wait for pushbutton KEY release
		   break;
	    }

	}

    while (1)
	{
		if (*KEY_ptr != 0)						// check if any KEY was pressed
		{
			break;
		}
	}
	    
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
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            short temp2 = Video_Mem_ptr[(y << 9) + x];
            *(Video_Mem_ptr + (y << 9) + x) = temp2;
        }
    }
    
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

    

    close(fd);
    return 0;
}