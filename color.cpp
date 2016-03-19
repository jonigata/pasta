namespace {

static unsigned long color_table[] = {
	0x8b0000,
	0x0000cd,
	0xdcdcdc,
	0x20b2aa,
	0xfffaf0,
	0xffd700,
	0x00ced1,
	0x66cdaa,
	0xff8c00,
	0xfff0f5,
	0x00ff7f,
	0xfffafa,
	0x6a5acd,
	0xf0ffff,
	0xffefd5,
	0xff1493,
	0xf5f5f5,
	0xf5f5dc,
	0x7b68ee,
	0x00fa9a,
	0xb22222,
	0xe0ffff,
	0xfffacd,
	0xff69b4,
	0x4682b4,
	0x8b4513,
	0xa0522d,
	0xf0e68c,
	0x7fff00,
	0xf4a460,
	0x00bfff,
	0xff4500,
	0x87cefa,
	0x6b8e23,
	0xfafad2,
	0x9400d3,
	0xd8bfd8,
	0xc71585,
	0x8fbc8f,
	0x0000ff,
	0x483d8b,
	0x191970,
	0x9acd32,
	0xfff8dc,
	0x800000,
	0xee82ee,
	0xd3d3d3,
	0xffa07a,
	0x006400,
	0xffe4b5,
	0xffff00,
	0xda70d6,
	0xc0c0c0,
	0xffe4c4,
	0xfff5ee,
	0x808080,
	0xff00ff,
	0xffb6c1,
	0xcd853f,
	0xadff2f,
	0xcd5c5c,
	0xff00ff,
	0xa52a2a,
	0xa9a9a9,
	0xffdead,
	0xb0e0e6,
	0xfdf5e6,
	0x2f4f4f,
	0xfaebd7,
	0xf0fff0,
	0xff6347,
	0x228b22,
	0xdaa520,
	0xfa8072,
	0xffffff,
	0xf5deb3,
	0xffc0cb,
	0xeee8aa,
	0x808000,
	0xafeeee,
	0xd2691e,
	0xfffff0,
	0x778899,
	0x2e8b57,
	0xb0c4de,
	0xffa500,
	0xf0f8ff,
	0xfaf0e6,
	0x48d1cc,
	0x7fffd4,
	0x556b2f,
	0xb8860b,
	0xdb7093,
	0xe6e6fa,
	0xdeb887,
	0x00ff00,
	0x98fb98,
	0xf5fffa,
	0x008080,
	0x3cb371,
	0x9370db,
	0xbdb76b,
	0x4169e1,
	0xffe4e1,
	0xd2b48c,
	0xba55d3,
	0x00008b,
	0xf8f8ff,
	0xffebcd,
	0xff0000,
	0x800080,
	0x8a2be2,
	0x5f9ea0,
	0x32cd32,
	0x6495ed,
	0x7cfc00,
	0xdda0dd,
	0x90ee90,
	0x8b008b,
	0xe9967a,
	0xadd8e6,
	0x00ffff,
	0x1e90ff,
	0xffffe0,
	0x9932cc,
	0x000000,
	0xdc143c,
	0x4b0082,
	0x696969,
	0xbc8f8f,
	0x708090,
	0x87ceeb,
	0x008b8b,
	0x40e0d0,
	0x008000,
	0xf08080,
	0xffdab9,
	0x000080,
	0x00ffff,
	0xff7f50,
};

typedef unsigned long rgb_type;

struct hsv_type {
	float h;
	float s;
	float v;
};

struct vec_type {
	float x;
	float y;
	float z;
};

hsv_type rgb_to_hsv( rgb_type rgb )
{
	float r = ( ( rgb & 0xff0000 ) >> 16 ) / 255.0f;
	float g = ( ( rgb & 0xff00 ) >> 8 ) / 255.0f;
	float b = ( ( rgb & 0xff ) ) / 255.0f;

	float maxc = r >= g && r >= b ? r : ( g >= b && g >= r ) ? g : b;
	float minc = r <= g && r <= b ? r : ( g <= b && g <= r ) ? g : b;

	float range = maxc - minc;

	hsv_type hsv;

	if( range == 0.0f ) {
		hsv.h = 0;
	} else if( r >= g && r >= b ) {
		hsv.h = 60.0f * ( g - b ) / range + 0;
	} else if( g >= r && g >= b ) {
		hsv.h = 60.0f * ( b - r ) / range + 120.0f;
	} else {
		hsv.h = 60.0f * ( r - g ) / range + 240.0f;
	}

	if( hsv.h < 0 ) { hsv.h += 360.0f; }

	hsv.s = range;
	hsv.v = maxc;
	return hsv;
}

float deg_to_rad( float deg )
{
	return deg / 180.0f * 3.14159265f;
}

vec_type hsv_to_vec( const hsv_type& hsv )
{
	float theta = deg_to_rad( hsv.h );

	vec_type v;
	v.x = cosf( theta ) * hsv.s * hsv.v;
	v.y = sinf( theta ) * hsv.s * hsv.v;
	v. z = hsv.v;
	return v;
}

float distance_sq( unsigned long x, unsigned long y )
{
	hsv_type xhsv = rgb_to_hsv( x );
	hsv_type yhsv = rgb_to_hsv( y );

	vec_type xv = hsv_to_vec( xhsv );
	vec_type yv = hsv_to_vec( yhsv );	
	
	vec_type d;
	d.x = yv.x - xv.x;
	d.y = yv.y - xv.y;
	d.z = yv.z - xv.z;
	return d.x * d.x + d.y * d.y + d.z * d.z;
}

} // unnamed namespace

float get_color_distance(unsigned long x, unsigned long y) {
    return sqrtf(distance_sq(x, y));
}

unsigned long get_color(int n) {
    unsigned long nn =(unsigned long)n;
    nn %= sizeof(color_table) / sizeof(unsigned long);
    return color_table[nn];
}
