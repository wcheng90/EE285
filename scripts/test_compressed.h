/*  GIMP header image file format (RGB): /home/user/Desktop/test.h  */

static unsigned int width = 100;
static unsigned int height = 100;

/*  Call this macro repeatedly.  After each use, the pixel data can be extracted  */

#define HEADER_PIXEL(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
}
static char *header_data_compressed =
"!!$` \xc\xe4 !.UC \x0\x64 !0]! \xc\x80 )NU! \x0\x64 `Q!! \xc\xe4 ";