/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "util.h"
#include "global.h"
#include "video.h"

#include <time.h>

//
// string utils
//

bool strendswith(const char *s, const char *e) {
	int ls = strlen(s);
	int le = strlen(e);

	if(le > ls)
		return false;

	int i; for(i = 0; i < le; ++i)
		if(s[ls-i-1] != e[le-i-1])
			return false;

	return true;
}

bool strstartswith(const char *s, const char *p) {
	int ls = strlen(s);
	int lp = strlen(p);

	if(ls < lp)
		return false;

	return !strncmp(s, p, lp);
}

bool strendswith_any(const char *s, const char **earray) {
	for(const char **e = earray; *e; ++e) {
		if(strendswith(s, *e)) {
			return true;
		}
	}

	return false;
}

bool strstartswith_any(const char *s, const char **earray) {
	for(const char **e = earray; *e; ++e) {
		if(strstartswith(s, *e)) {
			return true;
		}
	}

	return false;
}

void stralloc(char **dest, const char *src) {
	free(*dest);

	if(src) {
		*dest = malloc(strlen(src)+1);
		strcpy(*dest, src);
	} else {
		*dest = NULL;
	}
}

char* strjoin(const char *first, ...) {
	va_list args;
	size_t size = strlen(first) + 1;
	char *str = malloc(size);

	strcpy(str, first);
	va_start(args, first);

	for(;;) {
		char *next = va_arg(args, char*);

		if(!next) {
			break;
		}

		size += strlen(next);
		str = realloc(str, size);
		strcat(str, next);
	}

	va_end(args);
	return str;
}

char* vstrfmt(const char *fmt, va_list args) {
	size_t written = 0;
	size_t fmtlen = strlen(fmt);
	size_t asize = 1;
	char *out = NULL;

	while(asize <= fmtlen)
		asize *= 2;

	for(;;) {
		out = realloc(out, asize);

		va_list nargs;
		va_copy(nargs, args);
		written = vsnprintf(out, asize, fmt, nargs);
		va_end(nargs);

		if(written < asize) {
			break;
		}

		asize = written + 1;
	}

	if(asize > strlen(out) + 1) {
		out = realloc(out, strlen(out) + 1);
	}

	return out;
}

char* strfmt(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *str = vstrfmt(fmt, args);
	va_end(args);
	return str;
}

char* strftimealloc(const char *fmt, const struct tm *timeinfo) {
	size_t sz_allocated = 64;

	while(true) {
		char str[sz_allocated];

		if(strftime(str, sz_allocated, fmt, timeinfo)) {
			return strdup(str);
		}

		sz_allocated *= 2;
	};
}

char* copy_segment(const char *text, const char *delim, int *size) {
	char *seg, *beg, *end;

	beg = strstr(text, delim);
	if(!beg)
		return NULL;
	beg += strlen(delim);

	end = strstr(beg, "%%");
	if(!end)
		return NULL;

	*size = end-beg;
	seg = malloc(*size+1);
	strlcpy(seg, beg, *size+1);

	return seg;
}

void strip_trailing_slashes(char *buf) {
	for(char *c = buf + strlen(buf) - 1; c >= buf && (*c == '/' || *c == '\\'); c--)
		*c = 0;
}

char* strappend(char **dst, char *src) {
	if(!*dst) {
		return *dst = strdup(src);
	}

	*dst = realloc(*dst, strlen(*dst) + strlen(src) + 1);
	strcat(*dst, src);
	return *dst;
}

uint32_t* ucs4chr(const uint32_t *ucs4, uint32_t chr) {
	for(; *ucs4 != chr; ++ucs4) {
		if(!*ucs4) {
			return NULL;
		}
	}

	return (uint32_t*)ucs4;
}

size_t ucs4len(const uint32_t *ucs4) {
	size_t len;
	for(len = 0; *ucs4; ++len, ++ucs4);
	return len;
}

uint32_t* utf8_to_ucs4(const char *utf8) {
	assert(utf8 != NULL);
	uint32_t *ucs4 = (uint32_t*)(void*)SDL_iconv_string("UCS-4", "UTF-8", utf8, strlen(utf8) + 1);
	assert(ucs4 != NULL);
	return ucs4;
}

char* ucs4_to_utf8(const uint32_t *ucs4) {
	assert(ucs4 != NULL);
	char *utf8 = SDL_iconv_string("UTF-8", "UCS-4", (const char*)ucs4, sizeof(uint32_t) * (ucs4len(ucs4) + 1));
	assert(utf8 != NULL);
	return utf8;
}

/*
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char* strtok_r(char *str, const char *delim, char **nextp) {
	char *ret;

	if(str == NULL) {
		str = *nextp;
	}

	str += strspn(str, delim);

	if(*str == '\0') {
		return NULL;
	}

	ret = str;
	str += strcspn(str, delim);

	if(*str) {
		*str++ = '\0';
	}

	*nextp = str;
	return ret;
}

//
// math utils
//

double approach(double v, double t, double d) {
	if(v < t) {
		v += d;
		if(v > t)
			return t;
	} else if(v > t) {
		v -= d;
		if(v < t)
			return t;
	}

	return v;
}

double psin(double x) {
	return 0.5 + 0.5 * sin(x);
}

double max(double a, double b) {
	return (a > b)? a : b;
}

double min(double a, double b) {
	return (a < b)? a : b;
}

double clamp(double f, double lower, double upper) {
	if(f < lower)
		return lower;
	if(f > upper)
		return upper;

	return f;
}

int sign(double x) {
	return (x > 0) - (x < 0);
}

double swing(double x, double s) {
	if(x <= 0.5) {
		x *= 2;
		return x * x * ((s + 1) * x - s) / 2;
	}

	x--;
	x *= 2;

	return x * x * ((s + 1) * x + s) / 2 + 1;
}

uint32_t topow2_u32(uint32_t x) {
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

uint64_t topow2_u64(uint64_t x) {
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

float ftopow2(float x) {
	// XXX: obviously this isn't the smallest possible power of two, but for our purposes, it could as well be.
	float y = 0.0625;
	while(y < x) y *= 2;
	return y;
}

float smooth(float x) {
	return 1.0 - (0.5 * cos(M_PI * x) + 0.5);
}

float smoothreclamp(float x, float old_min, float old_max, float new_min, float new_max) {
	x = (x - old_min) / (old_max - old_min);
	return new_min + (new_max - new_min) * smooth(x);
}

float sanitize_scale(float scale) {
	return max(0.1, scale);
}

bool point_in_ellipse(complex p, Ellipse e) {
	double Xp = creal(p);
	double Yp = cimag(p);
	double Xe = creal(e.origin);
	double Ye = cimag(e.origin);
	double a = e.angle;

	return (
		pow(cos(a) * (Xp - Xe) + sin(a) * (Yp - Ye), 2) / pow(creal(e.axes)/2, 2) +
		pow(sin(a) * (Xp - Xe) - cos(a) * (Yp - Ye), 2) / pow(cimag(e.axes)/2, 2)
	) <= 1;
}

// Is the point of shortest distance between the line through a and b
// and a point c between a and b and closer than r?
// if yes, return f so that a+f*(b-a) is that point.
// otherwise return -1.
double lineseg_circle_intersect(LineSegment seg, Circle c) {
	complex m, v;
	double projection, lv, lm, distance;

	m = seg.b - seg.a; // vector pointing along the line
	v = seg.a - c.origin; // vector from circle to point A

	lv = cabs(v);
	lm = cabs(m);

	if(lv < c.radius) {
		return 0;
	}

	if(lm == 0) {
		return -1;
	}

	projection = -creal(v*conj(m)) / lm; // project v onto the line

	// now the distance can be calculated by Pythagoras
	distance = sqrt(pow(lv, 2) - pow(projection, 2));

	if(distance <= c.radius) {
		double f = projection/lm;

		if(f >= 0 && f <= 1) { // it’s on the line!
			return f;
		}
	}

	return -1;
}

bool lineseg_ellipse_intersect(LineSegment seg, Ellipse e) {
	// Transform the coordinate system so that the ellipse becomes a circle
	// with origin at (0, 0) and diameter equal to its X axis. Then we can
	// calculate the segment-circle intersection.

	double ratio = creal(e.axes) / cimag(e.axes);
	complex rotation = cexp(I * -e.angle);
	seg.a *= rotation;
	seg.b *= rotation;
	seg.a = creal(seg.a) + I * ratio * cimag(seg.a);
	seg.b = creal(seg.b) + I * ratio * cimag(seg.b);

	Circle c = { .radius = creal(e.axes) / 2 };
	return lineseg_circle_intersect(seg, c) >= 0;
}

//
// gl/video utils
//

void set_ortho(float w, float h) {
	r_mat_mode(MM_PROJECTION);
	r_mat_ortho(0, w, h, 0, -100, 100);
	r_mat_mode(MM_MODELVIEW);

	// FIXME: should we take this out of here and call it explicitly instead?
	r_disable(RCAP_DEPTH_TEST);
}

void colorfill(float r, float g, float b, float a) {
	if(a <= 0) return;

	r_shader_standard_notex();
	r_color4(r,g,b,a);

	r_mat_push();
	r_mat_scale(SCREEN_W,SCREEN_H,1);
	r_mat_translate(0.5,0.5,0);

	r_draw_quad();
	r_mat_pop();

	r_color4(1,1,1,1);
	r_shader_standard();
}

void fade_out(float f) {
	colorfill(0, 0, 0, f);
}

void draw_stars(int x, int y, int numstars, int numfrags, int maxstars, int maxfrags, float alpha, float star_width) {
	Sprite *star = get_sprite("star");
	int i = 0;
	float scale = star_width/star->w;

	Color amul = rgba(alpha, alpha, alpha, alpha);
	Color fill_clr = multiply_colors(rgba(1.0f, 1.0f, 1.0f, 1.0f), amul);
	Color back_clr = multiply_colors(rgba(0.2f, 0.6f, 1.0f, 0.2f), amul);
	Color frag_clr = mix_colors(derive_color(back_clr, CLRMASK_A, alpha), fill_clr, 0.35f);

	// XXX: move to call site?
	y -= 2;

	ShaderProgram *prog_saved = r_shader_current();
	r_shader("sprite_circleclipped_indicator");
	r_uniform_rgba("back_color", back_clr);

	r_mat_push();
	r_mat_translate(x - star_width, y, 0);

	while(i < numstars) {
		r_mat_translate(star_width, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = star,
			.custom = 1,
			.color = fill_clr,
			.scale.both = scale,
		});
		i++;
	}

	if(numfrags) {
		r_mat_translate(star_width, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = star,
			.custom = numfrags / (float)maxfrags,
			.color = frag_clr,
			.scale.both = scale,
		});
		i++;
	}

	while(i < maxstars) {
		r_mat_translate(star_width, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = star,
			.custom = 0,
			.color = frag_clr,
			.scale.both = scale,
		});
		i++;
	}

	r_mat_pop();
	r_shader_ptr(prog_saved);
}

//
// i/o uitls
//

char* read_all(const char *filename, int *outsize) {
	char *text;
	size_t size;

	SDL_RWops *file = vfs_open(filename, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!file) {
		log_warn("VFS error: %s", vfs_get_error());
		return NULL;
	}

	size = SDL_RWsize(file);

	text = malloc(size+1);
	SDL_RWread(file, text, size, 1);
	text[size] = 0;

	SDL_RWclose(file);

	if(outsize) {
		*outsize = size;
	}

	return text;
}

bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data) {
	static const char separator[] = "= ";

	char buffer[256];
	int lineno = 0;
	int errors = 0;

	loopstart: while(SDL_RWgets(strm, buffer, sizeof(buffer))) {
		char *ptr = buffer;
		char *sep, *key, *val;

		++lineno;

		while(isspace(*ptr)) {
			if(!*(++ptr)) {
				// blank line
				goto loopstart;
			}
		}

		if(*ptr == '#') {
			// comment
			continue;
		}

		sep = strstr(ptr, separator);

		if(!sep) {
			++errors;
			log_warn("Syntax error on line %i: missing separator", lineno);
			continue;
		}

		// split it up
		*sep = 0;
		key = ptr;
		val = sep + sizeof(separator) - 1;

		// the separator may be preceeded by any kind of whitespace, so strip it from the key
		while(isspace(*(ptr = strchr(key, 0) - 1))) {
			*ptr = 0;
		}

		// strip any kind of line endings from the value
		while(strchr("\r\n", *(ptr = strchr(val, 0) - 1))) {
			*ptr = 0;
		}

		if(!callback(key, val, data)) {
			++errors;
			continue;
		}
	}

	return !errors;
}

bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data) {
	SDL_RWops *strm = vfs_open(filename, VFS_MODE_READ);

	if(!strm) {
		log_warn("VFS error: %s", vfs_get_error());
		return false;
	}

	bool status = parse_keyvalue_stream_cb(strm, callback, data);
	SDL_RWclose(strm);
	return status;
}

static bool kvcallback_hashtable(const char *key, const char *val, void *data) {
	Hashtable *ht = data;
	hashtable_set_string(ht, key, strdup((void*)val));
	return true;
}

Hashtable* parse_keyvalue_stream(SDL_RWops *strm) {
	Hashtable *ht = hashtable_new_stringkeys();

	if(!parse_keyvalue_stream_cb(strm, kvcallback_hashtable, ht)) {
		hashtable_free(ht);
		ht = NULL;
	}

	return ht;
}

Hashtable* parse_keyvalue_file(const char *filename) {
	Hashtable *ht = hashtable_new_stringkeys();

	if(!parse_keyvalue_file_cb(filename, kvcallback_hashtable, ht)) {
		hashtable_free(ht);
		ht = NULL;
	}

	return ht;
}

static bool kvcallback_spec(const char *key, const char *val, void *data) {
	KVSpec *spec = data;
	for(KVSpec *s = spec; s->name; ++s) {
		if(!strcmp(key, s->name)) {
			if(s->out_str) {
				stralloc(s->out_str, val);
			}

			if(s->out_int) {
				*s->out_int = strtol(val, NULL, 10);
			}

			if(s->out_float) {
				*s->out_float = strtod(val, NULL);
			}

			if(s->out_double) {
				*s->out_double = strtod(val, NULL);
			}

			return true;
		}
	}

	log_warn("Unknown key '%s' with value '%s' ignored", key, val);
	return true;
}

bool parse_keyvalue_stream_with_spec(SDL_RWops *strm, KVSpec *spec) {
	return parse_keyvalue_stream_cb(strm, kvcallback_spec, spec);
}

bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec) {
	return parse_keyvalue_file_cb(filename, kvcallback_spec, spec);
}

static void png_rwops_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	SDL_RWops *out = png_get_io_ptr(png_ptr);
	SDL_RWwrite(out, data, length, 1);
}

static void png_rwops_flush_data(png_structp png_ptr) {
	// no flush operation in SDL_RWops
}

static void png_rwops_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	SDL_RWops *out = png_get_io_ptr(png_ptr);
	SDL_RWread(out, data, length, 1);
}

void png_init_rwops_read(png_structp png, SDL_RWops *rwops) {
	png_set_read_fn(png, rwops, png_rwops_read_data);
}

void png_init_rwops_write(png_structp png, SDL_RWops *rwops) {
	png_set_write_fn(png, rwops, png_rwops_write_data, png_rwops_flush_data);
}

char* SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize) {
	char c, *ptr = buf, *end = buf + bufsize - 1;
	assert(end > ptr);

	while((c = SDL_ReadU8(rwops)) && ptr <= end) {
		if((*ptr++ = c) == '\n')
			break;
	}

	if(ptr == buf)
		return NULL;

	if(ptr > end) {
		*end = 0;
		log_warn("Line too long (%zu bytes max): %s", bufsize, buf);
	} else {
		*ptr = 0;
	}

	return buf;
}

size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *str = vstrfmt(fmt, args);
	va_end(args);

	size_t ret = SDL_RWwrite(rwops, str, 1, strlen(str));
	free(str);

	return ret;
}

void tsfprintf(FILE *out, const char *restrict fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(out, fmt, args);
	va_end(args);
}

char* try_path(const char *prefix, const char *name, const char *ext) {
	char *p = strjoin(prefix, name, ext, NULL);

	if(vfs_query(p).exists) {
		return p;
	}

	free(p);
	return NULL;
}

//
// misc utils
//

void* memdup(const void *src, size_t size) {
	void *data = malloc(size);
	memcpy(data, src, size);
	return data;
}

noreturn static void png_error_handler(png_structp png_ptr, png_const_charp error_msg) {
	log_warn("PNG error: %s", error_msg);
	png_longjmp(png_ptr, 1);
}

static void png_warning_handler(png_structp png_ptr, png_const_charp warning_msg) {
	log_warn("PNG warning: %s", warning_msg);
}

void png_setup_error_handlers(png_structp png) {
	png_set_error_fn(png, NULL, png_error_handler, png_warning_handler);
}

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32str(uint32_t crc, const char *str) {
	const uint8_t *p = (uint8_t*)str;
	crc = crc ^ ~0U;

	while(*p) {
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ ~0U;
}

#ifdef DEBUG

bool _in_draw_code;

// TODO: make this thread-safe (use a TLS)
static DebugInfo debug_info;
static DebugInfo debug_meta;

void _set_debug_info(DebugInfo *info, DebugInfo *meta) {
	// assume the char*s point to literals
	memcpy(&debug_info, info, sizeof(DebugInfo));
	memcpy(&debug_meta, meta, sizeof(DebugInfo));
}

DebugInfo* get_debug_info(void) {
	return &debug_info;
}

DebugInfo* get_debug_meta(void) {
	return &debug_meta;
}

#endif
