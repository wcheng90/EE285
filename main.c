#include <stdio.h>
#include <stdint.h>
//test
// Tock API
#include <led.h>
#include <timer.h>
#include <spi.h>

#include <math.h>

//#include "images/test.h"
//#include "images/flownew.h"
//#include "images/simpson.h"
//#include "images/thesimpsonsdarker.h"
#include "scripts/test_compressed.h"

// Number of active pixels on the Dotstar LED strip.
// You may need to decrease this number if your power supply is not strong
// enough.
#define NUM_PIXELS 144
#define PIXEL_BUFFER_SIZE ((NUM_PIXELS*4) + 8)

// Number of frames in an image
#define NUM_FRAME 512

// Number of pixels in a strip
#define STRIP_LENGTH 36

// A Color is a 'packed' representation of its RGB components.
typedef uint32_t Color;

// Animation Array 
static Color animation_data[STRIP_LENGTH * NUM_FRAME];
static Color test_data[STRIP_LENGTH];

// Stores value of the current frame of strip 0;
static uint32_t current_frame = 0;

// Declare timer registers
tock_timer_t period_timer;
tock_timer_t frame_timer;

static uint32_t last = 0;
static uint32_t period = 0xFFFFFFFF;
static char unlock = 0x1;
static uint32_t now = 0;
static uint32_t frame_time = 0;
static uint32_t current = 0;
static int timer_interval = 10000;
static uint32_t interrupt_ctr = 0;

// Global buffer which holds all of the pixels.
static char pixels[PIXEL_BUFFER_SIZE];

// Needed for the spi_read_write call; unused.
static char rbuf[PIXEL_BUFFER_SIZE];

// Dotstar strips expect the pixel colors to be set blue first, green second,
// and red third.
static const uint32_t BLUE_OFFSET = 1;
static const uint32_t GREEN_OFFSET = 2;
static const uint32_t RED_OFFSET = 3;

// Constants used when converting between a Color and its RGB components.
#define COLOR_SHIFT(x) ((8 * (3 - x)))
#define RED_SHIFT COLOR_SHIFT(RED_OFFSET)
#define BLUE_SHIFT COLOR_SHIFT(BLUE_OFFSET)
#define GREEN_SHIFT COLOR_SHIFT(GREEN_OFFSET)

// Constants used when getting and setting pixel values.
#define PIXEL_INDEX(i, c) (((i * 4) + 4 + c))
#define RED_INDEX(i) (PIXEL_INDEX(i, RED_OFFSET))
#define BLUE_INDEX(i) (PIXEL_INDEX(i, BLUE_OFFSET))
#define GREEN_INDEX(i) (PIXEL_INDEX(i, GREEN_OFFSET))



/**
 * Extract the color components from a Color.
 * @{
 */
static uint8_t red(Color color) {
    return (uint8_t)((color >> RED_SHIFT) & 0xFF);
}

static uint8_t green(Color color) {
    return (uint8_t)((color >> GREEN_SHIFT) & 0xFF);
}

static uint8_t blue(Color color) {
    return (uint8_t)((color >> BLUE_SHIFT) & 0xFF);
}
/**
 * @}
 */

/**
 * Create a Color from its individual components.
 */
static Color color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << RED_SHIFT) | (g << GREEN_SHIFT) | (b << BLUE_SHIFT);
}

/**
 * Calculate a color along a color wheel, which cycles from red to blue to green
 * and back.
 *
 * @param wheelpos A value from 0-255 representing the position along the color
 *                 wheel.
 * @returns The Color corresponding to the given position in the color wheel.
 */
static Color wheel(uint8_t wheelpos) {
    wheelpos = 255 - wheelpos;
    if (wheelpos < 85) return color(255 - wheelpos * 3, 0, wheelpos * 3);

    if (wheelpos < 170) {
        wheelpos -= 85;
        return color(0, wheelpos*3, 255 - wheelpos * 3);
    }

    wheelpos -= 170;
    return color(wheelpos*3, 255 - wheelpos * 3, 0);
}

/**
 * Callback for the SPI write operation in update_strip().
 */
static void write_cb(__attribute__ ((unused)) int arg0,
                     __attribute__ ((unused)) int arg2,
                     __attribute__ ((unused)) int arg3,
                     __attribute__ ((unused)) void* userdata) {
}

/**
 * Set the color of the pixel at the given index.
 *
 * This does not actually update the LED strip, but instead just modifies the
 * pixel's color in the pixel buffer. To display the new color on the strip, you
 * must call update_strip().
 */
static void set_pixel(uint32_t pixel, Color color) {
    pixels[RED_INDEX(pixel)] = red(color);
    pixels[GREEN_INDEX(pixel)] = green(color);
    pixels[BLUE_INDEX(pixel)] = blue(color);
}

/**
 * Get the color of the pixel at the given index.
 *
 * This does not return the pixel color as it is currently displayed on the
 * strip, but rather the color in the pixel buffer that has been set by set_pixel().
 */
static Color __attribute__((unused)) get_pixel(uint32_t pixel) {
    return color(pixels[RED_INDEX(pixel)],
                 pixels[GREEN_INDEX(pixel)],
                 pixels[BLUE_INDEX(pixel)]);
}

/**
 * Initialize the SPI and the pixel buffer in order to use the LED strip.
 *
 * This function must be called once before ever calling update_strip().
 * All pixels in the buffer will be initially set to zero (off).
 */
static void initialize_strip(void) {
    spi_set_chip_select(0);
    spi_set_rate(12e6);
    spi_set_polarity(false);
    spi_set_phase(false);

    int i;
    for (i = 0; i < 4; i++) {
        pixels[i] = 0x0;
    }

    for (i = 4; i < PIXEL_BUFFER_SIZE; i++) {
        pixels[i] = 0xFF;
    }

    for (i = 0; i < NUM_PIXELS; i++) {
        set_pixel(i, 0);
    }
}

/**
 * Write all pixel values to the Dotstar LED strip.
 *
 * This function is what actually displays the new pixel colors onto the strip.
 * Before using this function, you must first have called initialize_strip().
 */
static void update_strip(void) {
//    spi_read_write(pixels, rbuf, PIXEL_BUFFER_SIZE, write_cb, NULL);
	spi_read_write_sync(&pixels, &rbuf, PIXEL_BUFFER_SIZE);
}

Color image_rect[100][100];


void extract(char compressed_image[]) {
	int i = 0; //i forms the horizontal rows of extracted image
	int j = 0; //j forms the vertical columns of extracted image
	int k = 0; //k iterates through compressed image data
	uint32_t pixel_repeat_cnt = 0;
	char current_pixel[4];
	Color extracted_image[100][100];
//	while (k < (sizeof(compressed_image)/sizeof(compressed_image[0])) || j == 100){
//	while (i < 100){ //k <= 32){
	while (k + 7 < 40 && i < 100){
		uint8_t pixel_data[3] = {128,128,128};
// Handles pixels with repetitions
//		if (compressed_image[0] == 0x58){
		if (compressed_image[k+4] == 0x20){

			pixel_repeat_cnt = (compressed_image[5] << 16) | (compressed_image[6] << 8) | (compressed_image[7]); //| (compressed_image[6] << 4) | (compressed_image[7]);
			for (int g = 1; g <= pixel_repeat_cnt; g++){
				current_pixel[0] = compressed_image[k];
				current_pixel[1] = compressed_image[k+1];
				current_pixel[2] = compressed_image[k+2];
				current_pixel[3] = compressed_image[k+3];
				HEADER_PIXEL(current_pixel , pixel_data);
				image_rect[i][j] = color(pixel_data[0], pixel_data[1], pixel_data[2]);	

				//image_rect[i][j] = color(0, 0, 255);			
				if (j >= 100){
					i += 1;
					j = 0;
				}
				else{
					j += 1;
				} 
			}
/*
			for (i = 0; i < 100; i++){
				for (j = 0; j < 100; j++){
					image_rect[i][j] = color(128,128,128);
				}
			}
*/
			k += 8; // Skip over color and value field
		} 
// Handles single pixels
		else {
			current_pixel[0] = compressed_image[k];
			current_pixel[1] = compressed_image[k+1];
			current_pixel[2] = compressed_image[k+2];
			current_pixel[3] = compressed_image[k+3];
			HEADER_PIXEL(current_pixel , pixel_data);
//			extracted_image[i][j] = color(pixel_data[0], pixel_data[1], pixel_data[2]);	

//			extracted_image[i][j] = color(128, 0, 128);
			image_rect[i][j] = color(128, 0, 128);
				
			if (j >= 100){
				i += 1;
				j = 0;
			}
			else{
				j += 1;
			} 

			k += 4; // Skip to next color

/*
set_pixel(0, compressed_image[5] & 0x00000001); 
set_pixel(1, (compressed_image[5] & 0x00000002) >> 1); 
set_pixel(2, (compressed_image[5] & 0x00000004) >> 2); 
set_pixel(3, (compressed_image[5] & 0x00000008) >> 3); 
set_pixel(4, (compressed_image[5] & 0x00000010) >> 4); 
set_pixel(5, (compressed_image[5] & 0x00000020) >> 5); 
set_pixel(6, (compressed_image[5] & 0x00000040) >> 6); 
set_pixel(7, (compressed_image[5] & 0x00000080) >> 7); 
set_pixel(8, (compressed_image[5] & 0x00000100) >> 8); 
set_pixel(9, (compressed_image[5] & 0x00000200) >> 9); 
set_pixel(10, (compressed_image[5] & 0x00000400) >> 10); 
set_pixel(11, (compressed_image[5] & 0x00000800) >> 11); 
set_pixel(12, (compressed_image[5] & 0x00001000) >> 12); 
set_pixel(13, (compressed_image[5] & 0x00002000) >> 13); 
set_pixel(14, (compressed_image[5] & 0x00004000) >> 14); 
set_pixel(15, (compressed_image[5] & 0x00008000) >> 15); 
set_pixel(16, compressed_image[6] & 0x00000001); 
set_pixel(17, (compressed_image[6] & 0x00000002) >> 1); 
set_pixel(18, (compressed_image[6] & 0x00000004) >> 2); 
set_pixel(19, (compressed_image[6] & 0x00000008) >> 3); 
set_pixel(20, (compressed_image[6] & 0x00000010) >> 4); 
set_pixel(21, (compressed_image[6] & 0x00000020) >> 5); 
set_pixel(22, (compressed_image[6] & 0x00000040) >> 6); 
set_pixel(23, (compressed_image[6] & 0x00000080) >> 7); 
set_pixel(24, (j & 0x00000001) >> 8); 
set_pixel(25, (j & 0x00000002) >> 9); 
set_pixel(26, (j & 0x00000004) >> 10); 
set_pixel(27, (j & 0x00000008) >> 11); 
set_pixel(28, (j & 0x00000010) >> 12); 
set_pixel(29, (j & 0x00000020) >> 13); 
set_pixel(30, (j & 0x00000040) >> 14); 
set_pixel(31, (j & 0x00000080) >> 15); 
update_strip();
*/
/*
set_pixel(0, i & 0x00000001); 
set_pixel(1, (i & 0x00000002) >> 1); 
set_pixel(2, (i & 0x00000004) >> 2); 
set_pixel(3, (i & 0x00000008) >> 3); 
set_pixel(4, (i & 0x00000010) >> 4); 
set_pixel(5, (i & 0x00000020) >> 5); 
set_pixel(6, (i & 0x00000040) >> 6); 
set_pixel(7, (i & 0x00000080) >> 7); 
set_pixel(8, (i & 0x00000001) >> 8); 
set_pixel(9, (i & 0x00000002) >> 9); 
set_pixel(10, (i & 0x00000004) >> 10); 
set_pixel(11, (i & 0x00000008) >> 11); 
set_pixel(12, (i & 0x00000010) >> 12); 
set_pixel(13, (i & 0x00000020) >> 13); 
set_pixel(14, (i & 0x00000040) >> 14); 
set_pixel(15, (i & 0x00000080) >> 15); 
set_pixel(16, j & 0x00000001); 
set_pixel(17, (j & 0x00000002) >> 1); 
set_pixel(18, (j & 0x00000004) >> 2); 
set_pixel(19, (j & 0x00000008) >> 3); 
set_pixel(20, (j & 0x00000010) >> 4); 
set_pixel(21, (j & 0x00000020) >> 5); 
set_pixel(22, (j & 0x00000040) >> 6); 
set_pixel(23, (j & 0x00000080) >> 7); 
set_pixel(24, (j & 0x00000001) >> 8); 
set_pixel(25, (j & 0x00000002) >> 9); 
set_pixel(26, (j & 0x00000004) >> 10); 
set_pixel(27, (j & 0x00000008) >> 11); 
set_pixel(28, (j & 0x00000010) >> 12); 
set_pixel(29, (j & 0x00000020) >> 13); 
set_pixel(30, (j & 0x00000040) >> 14); 
set_pixel(31, (j & 0x00000080) >> 15); 
update_strip();
*/
/*
set_pixel(0, k & 0x00000001); 
set_pixel(1, (k & 0x00000002) >> 1); 
set_pixel(2, (k & 0x00000004) >> 2); 
set_pixel(3, (k & 0x00000008) >> 3); 
set_pixel(4, (k & 0x00000010) >> 4); 
set_pixel(5, (k & 0x00000020) >> 5); 
set_pixel(6, (k & 0x00000040) >> 6); 
set_pixel(7, (k & 0x00000080) >> 7); 
update_strip();
*/

//	k += 1;
/*
		for (i = 0; i < 100; i++){
			for (j = 0; j < 100; j++){
				image_rect[i][j] = color(128,128,128);
			}
		}
*/
		}
	}
}
			
				  
	
/*
void populate_rect(void)  {
	uint8_t pixel_data[3];
	for (int i = 0; i < 100; i++) {  //i forms the horizontal rows of the image
		for (int j = 0; j < 100; j++) {   //j forms the vertical columns
			HEADER_PIXEL(header_data, pixel_data);
//			image_rect[i][j] = color(pixel_data[0], pixel_data[1], pixel_data[2]);
//			image_rect[i][j] = color(pixel_data[0]>>2, pixel_data[1]>>2, pixel_data[2]>>2);
			image_rect[i][j] = color(pixel_data[0]>>4, pixel_data[1]>>4, pixel_data[2]>>4);
		}
	}
	}
*/	

#define PI 3.14159265
double radian_interval = (2*PI)/512;
void rect_to_polar(void){
	int i;
	for (int frame_num = 0; frame_num < 512; frame_num++){
//		for (int r = 14; r < 50; r++){
		for (int r = 7; r < 43; r++){
//			animation_data[i] = image_rect[(int) (r*cos(0.703*frame_num) + 50)][(int) (r*sin(0.703*frame_num) + 50)];
//			animation_data[i] = image_rect[(int) (r*cos((double)radian_interval*frame_num) + 50)][(int) (r*sin((double)radian_interval*frame_num) + 50)]; 
			animation_data[i] = image_rect[(int) (r*(cos(((double)frame_num/512)*2*PI)) + 50)][(int) (r*(sin(((double)frame_num/512)*2*PI)) + 50)]; 
			i++;
		}
	}
	}


void animation_init(void) {
	extract(header_data_compressed);
//	populate_rect();

	for (int i = 0; i < STRIP_LENGTH*NUM_FRAME; i++){
		animation_data[i] = 0x0;
		}

	rect_to_polar();


/*
// Falling white light animation v2
	int j = 0;
	for (int k = 0; k < 36; k++){
		for (int i = 0; i < 14; i++) {	
			animation_data[(k*14*STRIP_LENGTH + i*STRIP_LENGTH) + j] = 0xFFFFFFFF;
		}
		if (j < STRIP_LENGTH){
			j++;
		}
		else{
			j = 0;
		}
	}
*/

/*
// Falling white light animation
	int j = 0;
	for (int i = 0; i < NUM_FRAME; i++) {	
		animation_data[i*STRIP_LENGTH + j] = 0xFFFFFFFF;
		if (j < STRIP_LENGTH){
			j++;
		}
		else{
			j = 0;
		}
	}
*/

/*
// Colorwheel animation

	int j = 0; // j keeps track of the led pixel in each frame
	int k = 0; // k keeps track of the color wheel
	for (int i = 0; i < NUM_FRAME; i++){ // i keeps track of the frame
		while (j < STRIP_LENGTH){
			animation_data[(i*STRIP_LENGTH) + j] = wheel(k);
			if (k >= 255){
				k = 0;
				}
			else{
				k = k + 1;
				}
			j = j + 1;			
			}
		j = 0;
		if (k >= STRIP_LENGTH - 1){
			k = k - (STRIP_LENGTH - 2);
			}
		else{
			k = 255 + (k - (STRIP_LENGTH - 2));
		}
	}
*/

// Blinktest Animation

//	animation_data[71] = 0xFFFFFFFF;
//	animation_data[35 * 36 + 35] = 0xFFFFFFFF;


/*
	for (int i = 0; i < STRIP_LENGTH*NUM_FRAME; i++){
		animation_data[i] = 0x0;
		}
	
	animation_data[0] = 0xFFFFFFFF;
	animation_data[37] = 0xFFFFFFFF;
	animation_data[74] = 0xFFFFFFFF;
	animation_data[111] = 0xFFFFFFFF;
	animation_data[148] = 0xFFFFFFFF;
	animation_data[185] = 0xFFFFFFFF;
	animation_data[222] = 0xFFFFFFFF;
	animation_data[259] = 0xFFFFFFFF;
	animation_data[296] = 0xFFFFFFFF;
	animation_data[333] = 0xFFFFFFFF;
	animation_data[370] = 0xFFFFFFFF;
	animation_data[407] = 0xFFFFFFFF;
	animation_data[444] = 0xFFFFFFFF;
	animation_data[481] = 0xFFFFFFFF;
	animation_data[518] = 0xFFFFFFFF;
*/
	for (int i = 0; i < STRIP_LENGTH; i++){
		test_data[i] = 0x0;
		}
	test_data[2] = 0xFFFFFFFF;

}

static void set_period_cb(__attribute__ ((unused)) int arg0,
                     __attribute__ ((unused)) int arg2,
                     __attribute__ ((unused)) int arg3,
                     __attribute__ ((unused)) void* userdata){
	now = alarm_read();

//set_pixel(0, now);
//set_pixel(2, last);
//update_strip();

if (now - last > 32000000){
	if (now > last){
		period = now - last;
		}
	else if(now < last){
		period = ((2^32-1) - last) + now;
		}
	else{
		period = 0;
		}
//set_pixel(33, 0xFFFFFFFF);
//update_strip();
//	frame_time = period/512;
	frame_time = 10;
	last = now;  // If this is pass by reference, then this won't work out;

//set_pixel(4, period);
//set_pixel(0, alarm_read());
//update_strip();

	unlock = 1;

	interrupt_ctr = interrupt_ctr + 1;
}
	}

static void blink_cb(__attribute__ ((unused)) int arg0,
                     __attribute__ ((unused)) int arg2,
                     __attribute__ ((unused)) int arg3,
                     __attribute__ ((unused)) void* userdata){
set_pixel(0, 0xFFFFFFFF);
update_strip();
	}

static void incr_cb(__attribute__ ((unused)) int arg0,
                     __attribute__ ((unused)) int arg2,
                     __attribute__ ((unused)) int arg3,
                     __attribute__ ((unused)) void* userdata){
if (current_frame < STRIP_LENGTH){
	current_frame = current_frame + 1;
	}
else{
	current_frame = 0;
	}
	}

static void blinktest_cb(__attribute__ ((unused)) int arg0,
                     __attribute__ ((unused)) int arg2,
                     __attribute__ ((unused)) int arg3,
                     __attribute__ ((unused)) void* userdata){
	current_frame = 1;
	update_wheel();
	}
//void set_strip(char strip_number, Color data) {	
void set_strip(char strip_number, Color data[STRIP_LENGTH]) {
	for (int i = 0; i < STRIP_LENGTH; i++){
		set_pixel((strip_number * 36) + i, data[i]);
	}
}

void update_wheel(void) {
// Subtracting current frame allows us to compensate for the fact that the wheel
// rotates clockwise, but the cosine/sine trig functions map counter clockwise.
// Without this, the image will appear in reverse as you bike forward
// The remapping of the strips and the strip pointers allow the image to be 
// displayed in the correct orientation
int framestrip0_ptr = (511 - current_frame) * STRIP_LENGTH;
int framestrip1_ptr = (((128 + (511 - current_frame)) % 512) * STRIP_LENGTH);
int framestrip2_ptr = (((256 + (511 - current_frame)) % 512) * STRIP_LENGTH);
int framestrip3_ptr = (((384 + (511 - current_frame)) % 512) * STRIP_LENGTH);
	set_strip(0, &animation_data[framestrip2_ptr]);
	set_strip(1, &animation_data[framestrip3_ptr]);
	set_strip(2, &animation_data[framestrip0_ptr]);
	set_strip(3, &animation_data[framestrip1_ptr]);
/*
	set_strip(0, &animation_data[framestrip3_ptr]);
	set_strip(1, &animation_data[framestrip2_ptr]);
	set_strip(2, &animation_data[framestrip1_ptr]);
	set_strip(3, &animation_data[framestrip0_ptr]);
*/
//	set_strip(0, test_data);
//	spi_read_write(pixels, rbuf, PIXEL_BUFFER_SIZE, write_cb, NULL);
	spi_read_write_sync(&pixels, &rbuf, PIXEL_BUFFER_SIZE);
	}
/**
 * Simple animation which cycles two simultaneous color wheels on the Dotstar LED strip.
 */

uint8_t pixel_data[3];
Color temp_color;

int main(void) {
	// Need to init the GPIOs
//		gpio_enable_interrupt(6,0,2); // Pull None; Falling Edge
		gpio_enable_interrupt(6,2,2); // Pull None; Falling Edge
	// Needed before calling update_strip(). All pixels in the buffer are
	// initialized to zero.
		initialize_strip();
		animation_init();
	//	timer_every(timer_interval, set_period_cb, NULL, &period_timer);
	//	timer_every(timer_interval, blinktest_cb, NULL, &period_timer);

//gpio_interrupt_callback(blinktest_cb, NULL);
//gpio_interrupt_callback(blink_cb, NULL);
gpio_interrupt_callback(set_period_cb, NULL);
//gpio_interrupt_callback(write_cb, NULL);
    while (1) {

		now = alarm_read();

	if (unlock == 1){
//		if (now > last){
		if (now > last && (((double)now - (double)last)/ (double) period) <= 1){
			current_frame = ((NUM_FRAME - 1) * ((double)now - (double)last))/ (double) period;
			}
//		else if(now < last){
		else if(now < last && ((double) ((2^32-1) - last) + (double) now) <= 1){
			current_frame = ((NUM_FRAME - 1) * ((double) ((2^32-1) - last) + (double) now))/ (double) period;			
		}
		else{
			current_frame = 0;
		}
	}
	else{
	}


//current_frame = 0;

/*
set_pixel(0, now);
set_pixel(2, last);
set_pixel(4, now-last);
set_pixel(6, period);
set_pixel(8, 0x00FF00FF);
set_pixel(10, current_frame);
*/

/*
set_pixel(0, current_frame & 0x00000001); 
set_pixel(1, (current_frame & 0x00000002) >> 1); 
set_pixel(2, (current_frame & 0x00000004) >> 2); 
set_pixel(3, (current_frame & 0x00000008) >> 3); 
set_pixel(4, (current_frame & 0x00000010) >> 4); 
set_pixel(5, (current_frame & 0x00000020) >> 5); 
set_pixel(6, (current_frame & 0x00000040) >> 6); 
set_pixel(7, (current_frame & 0x00000080) >> 7); 
set_pixel(8, (current_frame & 0x00000100) >> 8); 
set_pixel(9, (current_frame & 0x00000200) >> 9);
set_pixel(10, (current_frame & 0x00000400) >> 10);
set_pixel(11, (current_frame & 0x00000800) >> 11);
set_pixel(12, (current_frame & 0x00001000) >> 12);
set_pixel(13, (current_frame & 0x00002000) >> 13);
set_pixel(14, (current_frame & 0x00004000) >> 14);
set_pixel(15, (current_frame & 0x00008000) >> 15);
set_pixel(16, (current_frame & 0x00010000) >> 16);
set_pixel(17, (current_frame & 0x00020000) >> 17);
set_pixel(18, (current_frame & 0x00040000) >> 18);
set_pixel(19, (current_frame & 0x00080000) >> 19);
set_pixel(20, (current_frame & 0x00100000) >> 20);
set_pixel(21, (current_frame & 0x00200000) >> 21);
set_pixel(22, (current_frame & 0x00400000) >> 22);
set_pixel(23, (current_frame & 0x00800000) >> 23);
set_pixel(24, (current_frame & 0x01000000) >> 24);
set_pixel(25, (current_frame & 0x02000000) >> 25);
set_pixel(26, (current_frame & 0x04000000) >> 26);
set_pixel(27, (current_frame & 0x08000000) >> 27);
set_pixel(28, (current_frame & 0x10000000) >> 28);
set_pixel(29, (current_frame & 0x20000000) >> 29);
set_pixel(30, (current_frame & 0x40000000) >> 30);
set_pixel(31, (current_frame & 0x80000000) >> 31);
update_strip();
*/

/*
set_pixel(0, period & 0x00000001); 
set_pixel(1, (period & 0x00000002) >> 1); 
set_pixel(2, (period & 0x00000004) >> 2); 
set_pixel(3, (period & 0x00000008) >> 3); 
set_pixel(4, (period & 0x00000010) >> 4); 
set_pixel(5, (period & 0x00000020) >> 5); 
set_pixel(6, (period & 0x00000040) >> 6); 
set_pixel(7, (period & 0x00000080) >> 7); 
set_pixel(8, (period & 0x00000100) >> 8); 
set_pixel(9, (period & 0x00000200) >> 9);
set_pixel(10, (period & 0x00000400) >> 10);
set_pixel(11, (period & 0x00000800) >> 11);
set_pixel(12, (period & 0x00001000) >> 12);
set_pixel(13, (period & 0x00002000) >> 13);
set_pixel(14, (period & 0x00004000) >> 14);
set_pixel(15, (period & 0x00008000) >> 15);
set_pixel(16, (period & 0x00010000) >> 16);
set_pixel(17, (period & 0x00020000) >> 17);
set_pixel(18, (period & 0x00040000) >> 18);
set_pixel(19, (period & 0x00080000) >> 19);
set_pixel(20, (period & 0x00100000) >> 20);
set_pixel(21, (period & 0x00200000) >> 21);
set_pixel(22, (period & 0x00400000) >> 22);
set_pixel(23, (period & 0x00800000) >> 23);
set_pixel(24, (period & 0x01000000) >> 24);
set_pixel(25, (period & 0x02000000) >> 25);
set_pixel(26, (period & 0x04000000) >> 26);
set_pixel(27, (period & 0x08000000) >> 27);
set_pixel(28, (period & 0x10000000) >> 28);
set_pixel(29, (period & 0x20000000) >> 29);
set_pixel(30, (period & 0x40000000) >> 30);
set_pixel(31, (period & 0x80000000) >> 31);
*/

/*
set_pixel(0, interrupt_ctr & 0x00000001); 
set_pixel(1, (interrupt_ctr & 0x00000002) >> 1); 
set_pixel(2, (interrupt_ctr & 0x00000004) >> 2); 
set_pixel(3, (interrupt_ctr & 0x00000008) >> 3); 
set_pixel(4, (interrupt_ctr & 0x00000010) >> 4); 
set_pixel(5, (interrupt_ctr & 0x00000020) >> 5); 
set_pixel(6, (interrupt_ctr & 0x00000040) >> 6); 
set_pixel(7, (interrupt_ctr & 0x00000080) >> 7); 
set_pixel(8, (interrupt_ctr & 0x00000100) >> 8); 
set_pixel(9, (interrupt_ctr & 0x00000200) >> 9);
set_pixel(10, (interrupt_ctr & 0x00000400) >> 10);
set_pixel(11, (interrupt_ctr & 0x00000800) >> 11);
set_pixel(12, (interrupt_ctr & 0x00001000) >> 12);
set_pixel(13, (interrupt_ctr & 0x00002000) >> 13);
set_pixel(14, (interrupt_ctr & 0x00004000) >> 14);
set_pixel(15, (interrupt_ctr & 0x00008000) >> 15);
set_pixel(16, (interrupt_ctr & 0x00010000) >> 16);
set_pixel(17, (interrupt_ctr & 0x00020000) >> 17);
set_pixel(18, (interrupt_ctr & 0x00040000) >> 18);
set_pixel(19, (interrupt_ctr & 0x00080000) >> 19);
set_pixel(20, (interrupt_ctr & 0x00100000) >> 20);
set_pixel(21, (interrupt_ctr & 0x00200000) >> 21);
set_pixel(22, (interrupt_ctr & 0x00400000) >> 22);
set_pixel(23, (interrupt_ctr & 0x00800000) >> 23);
set_pixel(24, (interrupt_ctr & 0x01000000) >> 24);
set_pixel(25, (interrupt_ctr & 0x02000000) >> 25);
set_pixel(26, (interrupt_ctr & 0x04000000) >> 26);
set_pixel(27, (interrupt_ctr & 0x08000000) >> 27);
set_pixel(28, (interrupt_ctr & 0x10000000) >> 28);
set_pixel(29, (interrupt_ctr & 0x20000000) >> 29);
set_pixel(30, (interrupt_ctr & 0x40000000) >> 30);
set_pixel(31, (interrupt_ctr & 0x80000000) >> 31);
//set_pixel(32, now);
//set_pixel(33, last);
//set_pixel(34, (now-last));
//set_pixel(35, period);
set_pixel(32, now >> 16);
set_pixel(33, last >> 16);
set_pixel(34, (now-last) >> 16);
set_pixel(35, period > 16);
//set_pixel(40, now);
//set_pixel(41, last);
//set_pixel(42, (now-last));
//set_pixel(43, period);
update_strip();
*/

/*
set_pixel(0, wheel(0)); 
set_pixel(1, wheel(1)); 
set_pixel(2, wheel(2)); 
set_pixel(3, wheel(3)); 
set_pixel(4, wheel(4)); 
set_pixel(5, wheel(5)); 
set_pixel(6, wheel(6)); 
set_pixel(7, wheel(7)); 
set_pixel(8, wheel(8)); 
set_pixel(9, wheel(9));
update_strip();
*/
//set_pixel(0, 0x00000000);
/*
HEADER_PIXEL(header_data, pixel_data);
temp_color = color(pixel_data[0], pixel_data[1], pixel_data[2]);
set_pixel(0, temp_color);
update_strip();
*/

        // Display the new pixel values.
	update_wheel();
	//yield(); -- Do not use this

    }
}


