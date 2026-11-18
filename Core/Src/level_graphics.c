#include "level_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Original compact 5x7 font: columns, least significant bit at the top. */
static const uint8_t digits[10][5] = {
 {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},
 {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
 {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},
 {0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e}
};
static const uint8_t letters[26][5] = {
 {0x7e,9,9,9,0x7e},{0x7f,0x49,0x49,0x49,0x36},
 {0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},
 {0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},
 {0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,1},
 {0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},
 {0x3e,0x41,0x41,0x41,0x3e},{0x7f,9,9,9,6},
 {0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},
 {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
 {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
 {7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
};
static void Pixel(uint8_t *p, int x, int y)
{
    if (x >= 0 && x < 128 && y >= 0 && y < 64)
        p[x + (y / 8) * 128] |= (uint8_t)(1U << (y % 8));
}
static void Line(uint8_t *p, int x, int y, int x1, int y1)
{
    int dx = abs(x1-x), sx = x < x1 ? 1 : -1;
    int dy = -abs(y1-y), sy = y < y1 ? 1 : -1, error = dx+dy;
    for (;;) {
        Pixel(p,x,y);
        if (x == x1 && y == y1) break;
        int twice = 2*error;
        if (twice >= dy) { error += dy; x += sx; }
        if (twice <= dx) { error += dx; y += sy; }
    }
}
static void Text(uint8_t *p, int x, int y, const char *s)
{
    for (; *s; ++s, x += 6) {
        uint8_t glyph[5] = {0};
        if (*s >= '0' && *s <= '9') memcpy(glyph, digits[*s-'0'], 5);
        else if (*s >= 'A' && *s <= 'Z') memcpy(glyph, letters[*s-'A'], 5);
        else if (*s == '-') memset(glyph, 8, 5);
        else if (*s == '+') { memset(glyph,8,5); glyph[2]=0x3e; }
        else if (*s == ':') glyph[2]=0x14;
        for (int i=0;i<5;++i)
            for (int bit=0;bit<7;++bit)
                if (glyph[i] & (1U<<bit)) Pixel(p,x+i,y+bit);
    }
}
void LevelGraphics_Render(uint8_t p[LEVEL_SCREEN_BYTES], const LevelFrame *f)
{
    memset(p,0,LEVEL_SCREEN_BYTES);
    Text(p,0,0,f->mode == LEVEL_HORIZ ? "LEVEL HORIZ" : "LEVEL ARROW");
    for (unsigned i=0;i<3;++i) {
        char text[16];
        if (f->sensor_ok)
            (void)snprintf(text,sizeof(text),"%c:%+ld MG",'X'+i,(long)f->mg[i]);
        else (void)snprintf(text,sizeof(text),"%c:--- MG",'X'+i);
        Text(p,0,16+(int)i*12,text);
    }
    Text(p,0,56,f->sensor_ok ? "SENSOR OK" : "SENSOR ERROR");
    Line(p,83,11,126,11); Line(p,126,11,126,53);
    Line(p,126,53,83,53); Line(p,83,53,83,11);
    const int cx=104, cy=32;
    Pixel(p,cx-1,cy); Pixel(p,cx+1,cy); Pixel(p,cx,cy-1); Pixel(p,cx,cy+1);
    if (!f->sensor_ok) return;
    int dx=f->dx,dy=f->dy;
    if (f->mode == LEVEL_HORIZ) Line(p,cx-dx,cy-dy,cx+dx,cy+dy);
    else if (dx || dy) {
        Line(p,cx,cy,cx+dx,cy+dy);
        Line(p,cx+dx,cy+dy,cx+dx-dx/3-dy/4,cy+dy-dy/3+dx/4);
        Line(p,cx+dx,cy+dy,cx+dx-dx/3+dy/4,cy+dy-dy/3-dx/4);
    } else {
        for (int y=-2;y<=2;++y)
            for (int x=-2;x<=2;++x) Pixel(p,cx+x,cy+y);
    }
}
