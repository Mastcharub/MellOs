#ifdef VGA_VESA

#include "vesa.h"
#include "test_font.h"
#include "../utils/typedefs.h"
#include "../misc/colours.h"
#include "../utils/conversions.h"
#include "../memory/dynamic_mem.h"
#include "../utils/math.h"

Framebuffer vga_fb_true;
Framebuffer* vga_fb;
static uint8_t bytes_per_pixel;

void _vesa_framebuffer_init(PIXEL addr){
	vga_fb_true.fb = (volatile PIXEL*)addr;
    vga_fb_true.width = HRES;
    vga_fb_true.height = VRES;
    vga_fb_true.pitch = PITCH;
    vga_fb = &vga_fb_true;

	clear_screen(vga_fb);
}

void fb_clear_screen(Framebuffer fb) {
    fb_clear_screen_col_VESA(VESA_BLACK, fb);
}

void clear_screen() {
    fb_clear_screen(vga_fb_true);
}

void fb_clear_screen_col_VESA(VESA_Colour col, Framebuffer fb){
    uint32_t offset = 0;
    for (int i = 0; i < fb.height /*TODO: Remove -1 as below*/; i++) {
        for (int j = 0; j < fb.width; j++) {
            fb.fb[offset + j] = col.val;
        }
        offset += fb.pitch; // TODO: This goes at the end of the loop, once memory issues are figured out
    }
}

void clear_screen_col_VESA(VESA_Colour col) {
    fb_clear_screen_col_VESA(col, vga_fb_true);
}

void fb_clear_screen_col(Colour col, Framebuffer fb) {
    VESA_Colour vesa_col = vga2vesa(col);
    fb_clear_screen_col_VESA(vesa_col, fb);
}

void clear_screen_col(Colour col) {
    fb_clear_screen_col(col, vga_fb_true);
}

void fb_draw_line(int x1, int y1, int x2, int y2, size_t thickness, VESA_Colour col, Framebuffer* fb){
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    float Xinc = dx / (float)steps;
    float Yinc = dy / (float)steps;
    float X = x1;
    float Y = y1;
    for (int i = 0; i <= steps; i++) {
        fb_fill_rect(X, Y, thickness, thickness, col, *fb);
        X += Xinc;
        Y += Yinc;
    }
}

void fb_draw_rect(int x, int y, size_t width, size_t height, size_t thickness, VESA_Colour col, Framebuffer* fb){
    fb_draw_line(x, y, x + width, y, thickness, col, fb);
    fb_draw_line(x, y, x, y + height, thickness, col, fb);
    fb_draw_line(x + width, y, x + width, y + height, thickness, col, fb);
    fb_draw_line(x, y + height, x + width, y + height, thickness, col, fb);
}

void fb_fill_square(int x, int y, int size, VESA_Colour col, Framebuffer fb){
	fb_fill_rect(x, y, size, size, col, fb);
}

void fill_square(int x, int y, int size, VESA_Colour col) {
    fb_fill_rect(x, y, size, size, col, vga_fb_true);
}

void fb_fill_rect(int x, int y, int width, int height, VESA_Colour col, Framebuffer fb) {
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    width = min(width, fb.width - x);
    height = min(height, fb.height - y);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            fb.fb[(y + i) * fb.pitch + (x + j)] = col.val;
        }
    }
}

void fill_rect(int x, int y, int width, int height, VESA_Colour col) {
    fb_fill_rect(x, y, width, height, col, vga_fb_true);
}

void fb_fill_circle(int x, int y, int radius, VESA_Colour col, Framebuffer fb) {
	for (int i = -radius; i < radius; i++) {
		for (int j = -radius; j < radius; j++) {
			if (i * i + j * j < radius * radius) {
				fb.fb[(y + i) * fb.pitch + (x + j)] = col.val;
			}
		}
	}
}

void fill_circle(int x, int y, int radius, VESA_Colour col) {
    fb_fill_circle(x, y, radius, col, vga_fb_true);
}

void fb_draw_char(uint16_t x, uint16_t y, char c, VESA_Colour colour, float scaleX, float scaleY, Framebuffer fb) {
    uint8_t* font_char = font8x8_basic[(uint8_t)c];
    for (int row = 0; row < FONT_HEIGHT * scaleY; row++) {
        uint8_t pixels = font_char[(int)(row / scaleY)];
        for (int col = 0; col < FONT_WIDTH * scaleX; col++) {
            if (pixels & (1 << (int)(col / scaleX))) { // Check if the bit is set
                fb.fb[((y + row) * fb.pitch) + (x + col)] = colour.val;
            }
        }
    }
}

void draw_char(uint16_t x, uint16_t y, char c, VESA_Colour colour, float scaleX, float scaleY) {
    fb_draw_char(x, y, c, colour, scaleX, scaleY, vga_fb_true);
}

void fb_draw_string(uint16_t x, uint16_t y, const char* s, VESA_Colour colour, float scaleX, float scaleY, Framebuffer fb) {
    while (*s) {
        fb_draw_char(x, y, *s, colour, scaleX, scaleY, fb);
        x += FONT_WIDTH * scaleX;
        s++;
    }
}

Framebuffer* allocate_framebuffer(uint32_t width, uint32_t height) {
    Framebuffer* out = kmalloc(sizeof(Framebuffer));
    uint32_t size = width * height * bytes_per_pixel;
    out->fb = kmalloc(size);
    out->width = width;
    out->height = height;
    out->pitch = width; /*pitch*/
    // fb_clear_screen(out);
    return out;
}

Framebuffer* allocate_full_screen_framebuffer() {
    return allocate_framebuffer(HRES, VRES);
}

void deallocate_framebuffer(Framebuffer* fb) {
    kfree((void*)fb->fb, fb->width * fb->height * bytes_per_pixel);
    kfree((void*)fb, sizeof(Framebuffer));
}

void _blit(Framebuffer src, Framebuffer dest, int x, int y, uint32_t width, uint32_t height, int from_x, int from_y, int to_x, int to_y) {
    if (width < 0 || height < 0 || x > ((int)dest.width) || y > ((int)dest.height)) return;
    if (from_x > src.width || from_x > dest.width || from_y > src.height || from_y > dest.height) return;
    if (from_x < 0) from_x = 0;
    if (from_y < 0) from_y = 0;
    
    uint32_t src_offset = 0;

    if (y < 0) {
        // The following is negative because y is negative
        src_offset = -y * src.pitch;
        height += y;
        y = 0;
    }

    if (x < 0) {
        // Again negative because x is negative
        src_offset -= x;
        width += x;
        x = 0;
    }

    uint32_t dest_offset = y * dest.pitch + x;

    src_offset += from_y * src.pitch;
    dest_offset += from_y * dest.pitch;
    height -= from_y;

    width = min(width, to_x - x);
    height = min(height, to_y - from_y);

    width = min(width, dest.width - x);
    height = min(height, dest.height - y);

    width = min(width, src.width);
    height = min(height, src.height);

    // if (from_x != 0) {
    //     fb_draw_rect(from_x, from_y, width, height + to_y, 4, VESA_RED, &dest);
    //     return;
    // }

    // if (from_x != 0) {
    //     kprint_dec(from_x);
    //     kprint(" ");
    //     kprint_dec(from_y);
    //     kprint(" ");
    //     kprint_dec(width);
    //     kprint(" ");
    //     kprint_dec(height);
    //     kprint("\n");
    //     return;
    // }
    
    // src_offset -= from_x;
    uint32_t count = 0;
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = from_x; j < width; j++) {
            if (((VESA_Colour)(src.fb[src_offset + j])).a) {// Handle transparency
                // if (from_x != 0){
                //     dest.fb[dest_offset + j] = (src_offset / src.pitch) << 16 | (j) << 8 | 0xFF;
                //     continue;
                // }
                dest.fb[dest_offset + j] = src.fb[src_offset + j];
            }
        }
        dest_offset += dest.pitch;
        src_offset += src.pitch;
    }
}

void blit(Framebuffer src, Framebuffer dest, int x, int y, uint32_t width, uint32_t height) {
    _blit(src, dest, x, y, width, height, 0, 0, dest.width, dest.height);
}

void blit_all_at(Framebuffer* src, Framebuffer* dest, int x, int y) {
    blit(*src, *dest, x, y, src->width, src->height);
}

void blit_all_at_only(Framebuffer* src, Framebuffer* dest, int x, int y, int from_x, int from_y, int to_x, int to_y) {
    _blit(*src, *dest, x, y, src->width, src->height, from_x, from_y, to_x, to_y);
}


void fb_draw_gradient(int x, int y, int width, int height, VESA_Colour col1, VESA_Colour col2, Framebuffer* fb) {
    for (int i = 0; i < height; i++) {
        VESA_Colour col = vesa_interpolate(col1, col2, (float)i / height);
        for (int j = 0; j < width; j++) {
            fb->fb[(y + i) * fb->pitch + (x + j)] = col.val;
        }
    }
}

VESA_Colour vesa_interpolate(VESA_Colour col1, VESA_Colour col2, float t) {
    VESA_Colour out;
    out.r = col1.r + (col2.r - col1.r) * t;
    out.g = col1.g + (col2.g - col1.g) * t;
    out.b = col1.b + (col2.b - col1.b) * t;
    out.a = col1.a + (col2.a - col1.a) * t;
    return out;
}

#endif