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
//#include "scripts/flownew_compressed.h"
//#include "images/simpson.h"
//#include "images/thesimpsonsdarker.h"
//#include "scripts/test_compressed.h"
//#include "scripts/simpson100x100darker_compressed.h"
//include "scripts/testcircle_compressed.h"
//#include "scripts/pikachu_compressed.h"
//#include "scripts/charmander_compressed.h"
//#include "images/1.h"
//#include "images/2.h"
//Images Here
#include "scripts/pokemon/079Slowpoke_Dream_compressed.h"
#include "scripts/pokemon/Poke_mew_compressed.h"
#include "scripts/pokemon/124Jynx_compressed.h"
#include "scripts/pokemon/2100ShinyVoltorb_compressed.h"
#include "scripts/pokemon/pichu_by_mighty355d7cwjv7_compressed.h"
#include "scripts/pokemon/311Plusle_AG_anime_2_compressed.h"
#include "scripts/pokemon/001Bulbasaur_Dream_compressed.h"
#include "scripts/pokemon/158Totodile_OS_anime_compressed.h"
#include "scripts/pokemon/squirtle_compressed.h"
#include "scripts/pokemon/613px006Charizard_Dream_compressed.h"
#include "scripts/pokemon/255_torchic_by_pklucariod5z1jk7_compressed.h"
#include "scripts/pokemon/hitmonlee_by_mighty355d7f7yqw_compressed.h"
#include "scripts/pokemon/250px150Mewtwo_compressed.h"
#include "scripts/pokemon/enhanced1938714483146278_compressed.h"
#include "scripts/pokemon/pikachu_compressed.h"
#include "scripts/pokemon/063_compressed.h"
#include "scripts/pokemon/93Haunter_compressed.h"
#include "scripts/pokemon/5e7_compressed.h"
#include "scripts/pokemon/258Mudkip_AG_anime_2_compressed.h"
#include "scripts/pokemon/minum_by_kinlinkd3leo5p_compressed.h"
#include "scripts/pokemon/077Ponyta_Dream_compressed.h"
#include "scripts/pokemon/Lugia_compressed.h"
#include "scripts/pokemon/010Caterpie_Dream_compressed.h"
#include "scripts/pokemon/vulpix_compressed.h"
#include "scripts/pokemon/Charmander265x300_compressed.h"
#include "scripts/pokemon/013Weedle_Dream_compressed.h"
#include "scripts/pokemon/155Cyndaquil_OS_anime_2_compressed.h"
#include "scripts/pokemon/083Farfetchd_Dream_compressed.h"
#include "scripts/pokemon/gloom_by_mighty355d7j1sn7_compressed.h"
#include "scripts/pokemon/132_compressed.h"
#include "scripts/pokemon/055Golduck_Dream_compressed.h"
#include "scripts/pokemon/Patamon_t_compressed.h"
#include "scripts/pokemon/046Paras_Dream_compressed.h"
#include "scripts/pokemon/143Snorlax_OS_anime_compressed.h"
#include "scripts/pokemon/43Oddish_compressed.h"
#include "scripts/pokemon/024_compressed.h"
#include "scripts/pokemon/psyduck_compressed.h"
#include "scripts/pokemon/090_shellder_by_tails19950d4awdd2_compressed.h"
#include "scripts/pokemon/Gengar_compressed.h"
//#include "scripts/pokemon/Gastly_compressed.h"

char* pointer_mux;
//char* pointer_mux[2];

// Number of active pixels on the Dotstar LED strip.
// You may need to decrease this number if your power supply is not strong
// enough.
#define NUM_PIXELS 144
#define PIXEL_BUFFER_SIZE ((NUM_PIXELS*4) + 8)
#define IMAGE_NUM 40

// Number of frames in an image
#define NUM_FRAME 512

// Number of pixels in a strip
#define STRIP_LENGTH 36

#define HEADER_PIXEL_COMPRESSED(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
}

#define HEADER_PIXEL(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
data += 4; \
}

// A Color is a 'packed' representation of its RGB components.
typedef uint32_t Color;

// Animation Array 
static Color animation_data[STRIP_LENGTH * NUM_FRAME];
static Color animation_data2[STRIP_LENGTH * NUM_FRAME];

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
//static int timer_interval = 10000;
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
//	while (k + 7 < 40 && i < 100){
	while (i < 99){
		uint8_t pixel_data[3] = {128,128,128};
// Handles pixels with repetitions
		if (compressed_image[k+4] == 0x20){

			pixel_repeat_cnt = (compressed_image[k+5] << 8) | (compressed_image[k+6]); //| (compressed_image[6] << 4) | (compressed_image[7]);
			for (int g = 1; g <= pixel_repeat_cnt; g++){
				current_pixel[0] = compressed_image[k];
				current_pixel[1] = compressed_image[k+1];
				current_pixel[2] = compressed_image[k+2];
				current_pixel[3] = compressed_image[k+3];
				HEADER_PIXEL_COMPRESSED(current_pixel , pixel_data);
				image_rect[i][j] = color(pixel_data[0]>>4, pixel_data[1]>>4, pixel_data[2]>>4);	

				//image_rect[i][j] = color(0, 0, 255);			
				if (j >= 99){
					i += 1;
					j = 0;
				}
				else{
					j += 1;
				} 
			}

//			for (i = 0; i < 100; i++){
//				for (j = 0; j < 100; j++){
//					image_rect[i][j] = color(128,128,128);
//				}
//			}

			k += 8; // Skip over color and value field
		} 

// Handles single pixels
		else {

			current_pixel[0] = compressed_image[k];
			current_pixel[1] = compressed_image[k+1];
			current_pixel[2] = compressed_image[k+2];
			current_pixel[3] = compressed_image[k+3];
			HEADER_PIXEL_COMPRESSED(current_pixel , pixel_data);
			image_rect[i][j] = color(pixel_data[0]>>4, pixel_data[1]>>4, pixel_data[2]>>4);	

//			extracted_image[i][j] = color(128, 0, 128);
//			image_rect[i][j] = color(128, 0, 128);
				
			if (j >= 99){
				i += 1;
				j = 0;
			}
			else{
				j += 1;
			} 
			
			k += 4; // Skip to next color
			pixel_repeat_cnt = 0;

//	k += 1;

//		for (i = 0; i < 100; i++){
//			for (j = 0; j < 100; j++){
//				image_rect[i][j] = color(128,128,128);
//			}
//		}

		}
//printf("The data is %c %c %c %c. Pixel count is %x. \r\n", current_pixel[0], current_pixel[1], current_pixel[2], current_pixel[3], pixel_repeat_cnt);
//delay_ms(250);
//printf("The i is %d. The j is %d. \r\n", i, j);
//delay_ms(250);

	}
}


				  
	
/*
void populate_rect(int image_num)  {
// char* ptr = (char*)frames[image_num];

//	if (image_num == 0){
		pointer_mux = header_data1;
	}
	else{
		pointer_mux = header_data2;
	}

	uint8_t pixel_data[3];
	for (int i = 0; i < 100; i++) {  //i forms the horizontal rows of the image
		for (int j = 0; j < 100; j++) {   //j forms the vertical columns
			HEADER_PIXEL(pointer_mux, pixel_data);
 //  printf("The data is %d\r\n", header_data1);
 //  delay_ms(250);
//			image_rect[i][j] = color(pixel_data[0], pixel_data[1], pixel_data[2]);
//			image_rect[i][j] = color(pixel_data[0]>>2, pixel_data[1]>>2, pixel_data[2]>>2);
			image_rect[i][j] = color(pixel_data[0]>>4, pixel_data[1]>>4, pixel_data[2]>>4);
		}
	}
	}
*/
	

#define PI 3.14159265
double radian_interval = (2*PI)/512;
void rect_to_polar(int polar_buffer_num){
	int i = 0;
	for (int frame_num = 0; frame_num < 512; frame_num++){
		for (int r = 7; r < 43; r++){
//			if (polar_buffer_num == 0){
				animation_data[i] = image_rect[(int) (r*(cos(((double)frame_num/512)*2*PI)) + 50)][(int) (r*(sin(((double)frame_num/512)*2*PI)) + 50)]; 
//			}
//			else{
//				animation_data2[i] = image_rect[(int) (r*(cos(((double)frame_num/512)*2*PI)) + 50)][(int) (r*(sin(((double)frame_num/512)*2*PI)) + 50)]; 
//			}
	
			i++;
		}
	}
	}


void animation_init(int image_num) {
	if (image_num == 0){
		pointer_mux = header_data_compressed_squirtle;
	}
	else if (image_num == 1){
		pointer_mux = header_data_compressed_001Bulbasaur_Dream;
	}
	else if (image_num == 2){
		pointer_mux = header_data_compressed_158Totodile_OS_anime;
	}
	else if (image_num == 3){
		pointer_mux = header_data_compressed_2100ShinyVoltorb;
	}
	else if (image_num == 4){
		pointer_mux = header_data_compressed_pichu_by_mighty355d7cwjv7;
	}
	else if (image_num == 5){
		pointer_mux = header_data_compressed_311Plusle_AG_anime_2;
	}
	else if (image_num == 6){
		pointer_mux = header_data_compressed_083Farfetchd_Dream;
	}
	else if (image_num == 7){
		pointer_mux = header_data_compressed_124Jynx;
	}
	else if (image_num == 8){
		pointer_mux = header_data_compressed_010Caterpie_Dream;		
	}
	else if (image_num == 9){
		pointer_mux = header_data_compressed_613px006Charizard_Dream;
	}
	else if (image_num == 10){
		pointer_mux = header_data_compressed_Charmander265x300;
	}
	else if (image_num == 11){
		pointer_mux = header_data_compressed_hitmonlee_by_mighty355d7f7yqw;
	}
	else if (image_num == 12){
		pointer_mux = header_data_compressed_013Weedle_Dream;
	}
	else if (image_num == 13){
		pointer_mux = header_data_compressed_024;
	}
	else if (image_num == 14){
		pointer_mux = header_data_compressed_pikachu;
	}
	else if (image_num == 15){
		pointer_mux = header_data_compressed_063;
	}
	else if (image_num == 16){
		pointer_mux = header_data_compressed_93Haunter;
	}
	else if (image_num == 17){
		pointer_mux = header_data_compressed_5e7;
	}
	else if (image_num == 18){
		pointer_mux = header_data_compressed_258Mudkip_AG_anime_2;
	}
	else if (image_num == 19){
		pointer_mux = header_data_compressed_minum_by_kinlinkd3leo5p;
	}
	else if (image_num == 20){
		pointer_mux = header_data_compressed_077Ponyta_Dream;
	}
	else if (image_num == 21){
		pointer_mux = header_data_compressed_enhanced1938714483146278;
	}
	else if (image_num == 22){
		pointer_mux = header_data_compressed_079Slowpoke_Dream;
	}
	else if (image_num == 23){
		pointer_mux = header_data_compressed_vulpix;
	}
	else if (image_num == 24){
		pointer_mux = header_data_compressed_255_torchic_by_pklucariod5z1jk7;
	}
	else if (image_num == 25){
		pointer_mux = header_data_compressed_250px150Mewtwo;
	}
	else if (image_num == 26){
		pointer_mux = header_data_compressed_155Cyndaquil_OS_anime_2;
	}
	else if (image_num == 27){
		pointer_mux = header_data_compressed_Poke_mew;
	}
	else if (image_num == 28){
		pointer_mux = header_data_compressed_gloom_by_mighty355d7j1sn7;
	}
	else if (image_num == 29){
		pointer_mux = header_data_compressed_132;
	}
	else if (image_num == 30){
		pointer_mux = header_data_compressed_055Golduck_Dream;
	}
	else if (image_num == 31){
		pointer_mux = header_data_compressed_Patamon_t;
	}
	else if (image_num == 32){
		pointer_mux = header_data_compressed_046Paras_Dream;
	}
	else if (image_num == 33){
		pointer_mux = header_data_compressed_143Snorlax_OS_anime;
	}
	else if (image_num == 34){
		pointer_mux = header_data_compressed_43Oddish;
	}
	else if (image_num == 35){
		pointer_mux = header_data_compressed_Lugia;
	}
	else if (image_num == 36){
		pointer_mux = header_data_compressed_psyduck;
	}
	else if (image_num == 37){
		pointer_mux = header_data_compressed_090_shellder_by_tails19950d4awdd2;
	}
	else if (image_num == 38){
		pointer_mux = header_data_compressed_Gengar;
	}
/*	else if (image_num == 39){
		pointer_mux = header_data_compressed_Gastly;
	}
*/	extract(pointer_mux);
//	populate_rect(image_num);

/* Not really needed for full 100x100 images
	for (int i = 0; i < STRIP_LENGTH*NUM_FRAME; i++){
		animation_data[i] = 0x0;
		}
*/
	rect_to_polar(0);


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


}

int image_ptr = 0;

static void set_period_cb(__attribute__ ((unused)) int arg0,
                     __attribute__ ((unused)) int arg2,
                     __attribute__ ((unused)) int arg3,
                     __attribute__ ((unused)) void* userdata){
	now = alarm_read();

//set_pixel(0, now);
//set_pixel(2, last);
//update_strip();


//Used to ensure that noise on the inductor input is thrown out
if (now - last > 32000000){
	if (now > last){
		period = now - last;
		}
	else if(now < last){
		period = (((2^32)-1) - last) + now;
		}
	else{
		period = 0;
		}
//set_pixel(33, 0xFFFFFFFF);
//update_strip();
//	frame_time = period/512;
	frame_time = 10;
	last = now;  // If this is pass by reference, then this won't work out;
	
	if (interrupt_ctr%6 == 0){
		if (image_ptr < IMAGE_NUM - 1){
			image_ptr += 1;
		}
		else{
			image_ptr = 0;
		}
		animation_init(image_ptr);
	}
	
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

void update_wheel(int polar_buffer_num) {
// Subtracting current frame allows us to compensate for the fact that the wheel
// rotates clockwise, but the cosine/sine trig functions map counter clockwise.
// Without this, the image will appear in reverse as you bike forward
// The remapping of the strips and the strip pointers allow the image to be 
// displayed in the correct orientation
int framestrip0_ptr = (511 - current_frame) * STRIP_LENGTH;
int framestrip1_ptr = (((128 + (511 - current_frame)) % 512) * STRIP_LENGTH);
int framestrip2_ptr = (((256 + (511 - current_frame)) % 512) * STRIP_LENGTH);
int framestrip3_ptr = (((384 + (511 - current_frame)) % 512) * STRIP_LENGTH);
//	if (polar_buffer_num == 0){
		set_strip(0, &animation_data[framestrip2_ptr]);
		set_strip(1, &animation_data[framestrip3_ptr]);
		set_strip(2, &animation_data[framestrip0_ptr]);
		set_strip(3, &animation_data[framestrip1_ptr]);
/*	}
	else{
		set_strip(0, &animation_data2[framestrip2_ptr]);
		set_strip(1, &animation_data2[framestrip3_ptr]);
		set_strip(2, &animation_data2[framestrip0_ptr]);
		set_strip(3, &animation_data2[framestrip1_ptr]);
	}
*/
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
//delay_ms(1000);
//pointer_mux[0] = header_data1;
//pointer_mux[1] = header_data2;
	// Need to init the GPIOs
		gpio_set(22);
		gpio_enable_input(22, 0);
		gpio_enable_interrupt(22, 2);
//		gpio_enable_interrupt(6,2,2); // Pull None; Falling Edge
//		gpio_enable_interrupt(21,0,2); // Pull None; Falling Edge
	// Needed before calling update_strip(). All pixels in the buffer are
	// initialized to zero.
		initialize_strip();
		animation_init(1);
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
		else if(now < last && ((double) (((2^32)-1) - last) + (double) now) <= 1){
			current_frame = ((NUM_FRAME - 1) * ((double) (((2^32)-1) - last) + (double) now))/ (double) period;			
		}
		else{
			current_frame = 0;
		}
	}
	else{
	}


/*
HEADER_PIXEL(header_data, pixel_data);
temp_color = color(pixel_data[0], pixel_data[1], pixel_data[2]);
set_pixel(0, temp_color);
update_strip();
*/
//    printf("Hi there! It's Warren");
//    delay_ms(250);
        // Display the new pixel values.
	update_wheel(0);
	//yield(); -- Do not use this

    }
}


