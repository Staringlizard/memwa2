/*
 * memwa2 vic component
 *
 * Copyright (c) 2016 Mathias Edman <mail@dicetec.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 *
 */


/**
 * VIC implementation (MOS6569).
 * This is by far the most difficult chip to emulate. In this
 * file you can find a pretty decent attempt to emulate the chip
 * as fast as possible without loosing too much compatability with
 * real HW. Many bugs still exist though (which will be quite visible
 * on screen).
 *
 * The canvas is written to by writing to the variable g_layer_addr_p.
 * The canvas is the emulators "screen memory". The hardware should
 * map this memory directly to the screen. The emulator will call swap
 * function (disp_flip_fp) when a new frame is done.
 *
 * There are also 2 sprite layers (forground and background) and one map
 * layer. When rasterline is drawing the screen it will look at the sprite
 * layers to see if any sprite data is present or not. So if sprite pixel exist
 * then this pixel will be transfered to screen layer. The map layer is necessary
 * to understand where all sprites are drawn and to get correct behaviour from
 * collisions.
 *
 * Sprites always needs to be erased before they are drawn again.
 */

#include "vic.h"
#include <stdlib.h>
#include <string.h>
#include "if.h"
#include "bus.h"
#include "cpu.h"
#include "tap.h"
#include "cia.h"

#define NO_OF_FRAMES_STATS      50
#define NO_OF_FRAMES_LOCK       2
#define SPRITE_NONE             0xFF
#define SPRITE_MAP_NONE         0x00
#define SPRITE_WIDTH            24
#define SPRITE_HEIGHT           21
#define SPRITE_DOT_MATRIX_SIZE  63
#define SPRITE_NO_POS           0xDEAD

#define CHAR_HORIZONTAL_LENGTH  8
#define CHAR_VERTICAL_LENGTH    8

#define UCYCLE_PER_PIXEL        (UCYCLES_LINE/PIXELS_MAX)
#define UCYCLES_LINE            (63000)
#define UCYCLES_BAD_LINE_WAIT   (UCYCLES_LINE/9)
#define UCYCLE_BAD_SPRITE       (2000)
#define UCYCLES_BAD_LINE        (43000)
#define LINE_MAX                (312)
#define PIXELS_MAX              (504)
#define PIXELS_EXT_LEFT_BORD    (7)
#define PIXELS_EXT_RIGHT_BORD   (9)

#define LINE_BORD_UPPER_START   (16)
#define LINE_BORD_LOWER_START   (251)
#define LINE_BLANK_START        (300)
#define LINE_DISP_WIND_START    (51)
#define LINE_BAD_LOWER_COND     (48)
#define LINE_BAD_UPPER_COND     (247)
#define LINE_EXT_UPPER_BORD     (55)
#define LINE_EXT_LOWER_BORD     (247)

#define PIXEL_LEFT_BORD_START   (76)
#define PIXEL_RIGHT_BORD_START  (444)
#define PIXEL_BAD_LINE_CHECK    (112)
#define PIXEL_DISP_WIND_START   (124)
#define PIXEL_LOAD_VCBASE       (464)
#define PIXEL_RIGHT_BLANK_START (480)
#define PIXEL_SPRITE_X_START    (PIXEL_LEFT_BORD_START + 24)

/* Switch case is more optimation friendly than calling function pointer */
#define OUTPUT_PIXEL() \
switch(g_graphic_mode) \
{ \
  case GRAPHIC_MODE_STM: \
    output_pixel_STM(); \
    break; \
  case GRAPHIC_MODE_MTM: \
    output_pixel_MTM(); \
    break; \
  case GRAPHIC_MODE_SBM: \
    output_pixel_SBM(); \
    break; \
  case GRAPHIC_MODE_MBM: \
    output_pixel_MBM(); \
    break; \
  case GRAPHIC_MODE_ECM: \
    output_pixel_ECM(); \
    break; \
  case GRAPHIC_MODE_ITM: \
  case GRAPHIC_MODE_IBM_1: \
  case GRAPHIC_MODE_IBM_2: \
    output_pixel_BAD(); \
    break; \
  case GRAPHIC_MODE_BORDER_EXT: \
    output_pixel_BORDER_EXT(); \
    break; \
  default: \
    output_pixel_BAD(); \
}

/* Switch case is more optimation friendly than calling function pointer */
#define LOAD_DOT_MATRIX() \
switch(g_graphic_mode) \
{ \
  case GRAPHIC_MODE_STM: \
  case GRAPHIC_MODE_MTM: \
    load_dot_matrix_STM_MTM(); \
    break; \
  case GRAPHIC_MODE_SBM: \
  case GRAPHIC_MODE_MBM: \
    load_dot_matrix_SBM_MBM(); \
    break; \
  case GRAPHIC_MODE_ECM: \
    load_dot_matrix_ECM(); \
    break; \
  case GRAPHIC_MODE_ITM: \
  case GRAPHIC_MODE_IBM_1: \
  case GRAPHIC_MODE_IBM_2: \
    load_dot_matrix_BAD(); \
    break; \
  case GRAPHIC_MODE_BORDER_EXT: \
    load_dot_matrix_BORDER(); \
    break; \
  default: \
    load_dot_matrix_BAD(); \
}

static void event_write_xsprite0(uint16_t addr, uint8_t value);
static void event_write_ysprite0(uint16_t addr, uint8_t value);
static void event_write_xsprite1(uint16_t addr, uint8_t value);
static void event_write_ysprite1(uint16_t addr, uint8_t value);
static void event_write_xsprite2(uint16_t addr, uint8_t value);
static void event_write_ysprite2(uint16_t addr, uint8_t value);
static void event_write_xsprite3(uint16_t addr, uint8_t value);
static void event_write_ysprite3(uint16_t addr, uint8_t value);
static void event_write_xsprite4(uint16_t addr, uint8_t value);
static void event_write_ysprite4(uint16_t addr, uint8_t value);
static void event_write_xsprite5(uint16_t addr, uint8_t value);
static void event_write_ysprite5(uint16_t addr, uint8_t value);
static void event_write_xsprite6(uint16_t addr, uint8_t value);
static void event_write_ysprite6(uint16_t addr, uint8_t value);
static void event_write_xsprite7(uint16_t addr, uint8_t value);
static void event_write_ysprite7(uint16_t addr, uint8_t value);
static void event_write_xmsb(uint16_t addr, uint8_t value);
static void event_write_cr1(uint16_t addr, uint8_t value);
static void event_write_raster(uint16_t addr, uint8_t value);
static void event_write_sprite_enabled(uint16_t addr, uint8_t value);
static void event_write_cr2(uint16_t addr, uint8_t value);
static void event_write_sprite_yexp(uint16_t addr, uint8_t value);
static void event_write_mem_pointers(uint16_t addr, uint8_t value);
static void event_write_irq_enable(uint16_t addr, uint8_t value);
static void event_write_sprite_prio(uint16_t addr, uint8_t value);
static void event_write_sprite_multicolor(uint16_t addr, uint8_t value);
static void event_write_sprite_xexp(uint16_t addr, uint8_t value);
static void event_write_sprite_coll(uint16_t addr, uint8_t value);
static void event_write_fg_coll(uint16_t addr, uint8_t value);
static void event_write_sprite_multicolor0(uint16_t addr, uint8_t value);
static void event_write_sprite_multicolor1(uint16_t addr, uint8_t value);
static void event_write_sprite_color0(uint16_t addr, uint8_t value);
static void event_write_sprite_color1(uint16_t addr, uint8_t value);
static void event_write_sprite_color2(uint16_t addr, uint8_t value);
static void event_write_sprite_color3(uint16_t addr, uint8_t value);
static void event_write_sprite_color4(uint16_t addr, uint8_t value);
static void event_write_sprite_color5(uint16_t addr, uint8_t value);
static void event_write_sprite_color6(uint16_t addr, uint8_t value);
static void event_write_sprite_color7(uint16_t addr, uint8_t value);
static void event_write_irq(uint16_t addr, uint8_t value);
static uint8_t event_read_cr2(uint16_t addr);
static uint8_t event_read_irq_enable(uint16_t addr);
static uint8_t event_read_irq(uint16_t addr);
static uint8_t event_read_color_border(uint16_t addr);
static uint8_t event_read_color_ram(uint16_t addr);
static uint8_t event_read_bg_0(uint16_t addr);
static uint8_t event_read_bg_1(uint16_t addr);
static uint8_t event_read_bg_2(uint16_t addr);
static uint8_t event_read_bg_3(uint16_t addr);
static uint8_t event_read_mcolor_0(uint16_t addr);
static uint8_t event_read_mcolor_1(uint16_t addr);
static uint8_t event_read_color_sprite_0(uint16_t addr);
static uint8_t event_read_color_sprite_1(uint16_t addr);
static uint8_t event_read_color_sprite_2(uint16_t addr);
static uint8_t event_read_color_sprite_3(uint16_t addr);
static uint8_t event_read_color_sprite_4(uint16_t addr);
static uint8_t event_read_color_sprite_5(uint16_t addr);
static uint8_t event_read_color_sprite_6(uint16_t addr);
static uint8_t event_read_color_sprite_7(uint16_t addr);
static uint8_t event_read_ss_coll(uint16_t addr);
static uint8_t event_read_sg_coll(uint16_t addr);
static uint8_t event_read_unused(uint16_t addr);
static inline void load_dot_matrix_STM_MTM();
static inline void load_dot_matrix_SBM_MBM();
static inline void load_dot_matrix_ECM();
static inline void load_dot_matrix_BORDER();
static inline void load_dot_matrix_BAD();
static void graphic_mode();
static void erase_sprite(uint32_t sprite, uint32_t at_x, uint32_t at_y);
static void draw_sprite(uint32_t sprite, uint32_t at_x, uint32_t at_y);
static void move_sprite(uint32_t sprite, uint32_t from_x, uint32_t from_y, uint32_t to_x, uint32_t to_y);
static void render_sprite(uint32_t sprite, uint32_t checksum, uint8_t *base_p);
static void handle_sprite_redraw_queue();
static void refresh_sprites();
static void new_line();
static void new_sprite_line();
static void new_text_line();
static void new_frame();
static inline void output_pixel_STM();
static inline void output_pixel_ECM();
static inline void output_pixel_MTM();
static inline void output_pixel_SBM();
static inline void output_pixel_MBM();
static inline void output_pixel_BORDER_EXT();
static inline void output_pixel_BORDER();
static inline void output_pixel_BAD();
static uint8_t calc_fps(uint32_t time_now, uint32_t time_start);

typedef struct
{
  uint32_t prio_changed;
  uint32_t y_exp_changed;
  uint32_t x_exp_changed;
  uint32_t multi_color_mode_changed;
  uint32_t color_changed;
  uint32_t enabled;
  uint32_t prio;
  uint32_t y_exp;
  uint32_t x_exp;
  uint32_t y_exp_factor;
  uint32_t x_exp_factor;
  uint32_t x_prev_slayer_pos;
  uint32_t y_prev_slayer_pos;
  uint32_t x;
  uint32_t y;
  uint32_t multi_color_mode; /* True when in multi color mode */
  uint32_t color; /* This is the color of the sprite */
  uint32_t bitmap_multi_color_0; /* Is set when bitmap rendered */
  uint32_t bitmap_multi_color_1; /* Is set when bitmap rendered */
  uint32_t bitmap_color; /* Is set when bitmap rendered */
  uint8_t bitmap_a[SPRITE_HEIGHT * SPRITE_WIDTH]; /* Is set when sprite is rendered */
  uint32_t checksum; /* Checksum for the dot matrix. Makes it possible to know if bitmap needs refresh */
} sprite_t;

typedef enum
{
    VIC_STATE_HOLD,
    VIC_STATE_VERTICAL_BLANKING,
    VIC_STATE_VERTICAL_UPPER_BORDER,
    VIC_STATE_VERTICAL_LOWER_BORDER,
    VIC_STATE_HORIZONTAL_LEFT_BLANKING,
    VIC_STATE_HORIZONTAL_LEFT_BORDER,
    VIC_STATE_DISPLAY_WINDOW,
    VIC_STATE_HORIZONTAL_RIGHT_BORDER,
    VIC_STATE_HORIZONTAL_RIGHT_BLANKING,
    VIC_STATE_VERTICAL_WAIT_BAD_LINE
} vic_state_t;

extern memory_t g_memory; /* Memory interface */
extern if_host_t g_if_host; /* Main interface */

static uint8_t *g_layer_addr_p; /* Memory pointer used to draw pixels with */
static uint8_t *g_layer_addr_start_p; /* Start addr for graphic memory */
static uint8_t *g_sprite_layer_addr_aap[VIC_MEM_MAX]; /* Memory to keep track of sprites */
static uint8_t *g_sprite_layer_addr_start_aap[VIC_MEM_MAX]; /* Memory to keep track of sprites */

static uint8_t *g_pixel_sprite_mapping_p; /* Tracking which pixel is drawn by witch sprite */
static uint8_t *g_pixel_sprite_mapping_start_p; /* Tracking which pixel is drawn by witch sprite */
static uint8_t *g_sprite_pointer_ap[8]; /* Contains pointers to sprite bitmaps */
static uint8_t *g_crom_p; /* Pointer to start of character rom */
static uint8_t *g_ram_p; /* Pointer to start of ram */
static uint8_t *g_border_color_p; /* Pointer to vic memory */
static uint8_t *g_screen_ram_p; /* Pointer to video matrix memory area */
static uint8_t *g_fg_color_p; /* Pointer to vic memory */
static uint8_t *g_bg_color_pp[4]; /* Pointer to vic memory */
static uint8_t g_x_scroll;
static uint8_t g_y_scroll;
static uint8_t g_window_row_cnt; /* AKA "RC" */
static uint8_t g_sprite_presence_a[LINE_MAX + SPRITE_HEIGHT*2]; /* Tracking sprites for line */

/*
 * The simple way is just to move a sprite when it changes its x and y values.
 * However, some games are moving sprites over 1000 times a second (e.g. commando) and
 * for no apparant reason. This will totally choke the emulation.
 * The fix for this is to instead of drawing the sprite directly, queue it up and
 * draw it only when needed, i.e when the raster line meets the y coordinate of the sprite.
 */
static uint8_t g_sprite_redraw_queue_a[LINE_MAX];
static uint8_t g_full_frame_rate;
static uint8_t g_lock_frame_rate;
static uint8_t g_display_frame; /* Hold every second frame (graphics fg and bg) to gain performance */
static uint8_t g_char_pointers_a[41]; /* Char pointers are loaded when bad line occurs */

static uint32_t g_sprite_present_on_current_line;
static uint32_t g_fps_saved_time;
static uint32_t g_fps_stats_time;
static uint32_t g_frames_until_stats;
static uint32_t g_frames_until_lock;
static uint32_t g_raster_line_irq;
static uint32_t g_sprite_multi_color_0;
static uint32_t g_sprite_multi_color_1;
static uint32_t g_raster_irq_en;
static uint32_t g_mem_pointer;
static uint32_t g_char_gen_offset;
static uint32_t g_bitmap_gen_offset;
static uint32_t g_screen_ram_offset;
static uint32_t g_bank;
static uint32_t g_dot_matrix; /* 8 bit bitmap to draw */
static uint32_t g_dot_matrix_color; /* The color when in bitmap mode */
static uint32_t g_graphic_mode;
static uint32_t g_sg_irq_set; /* Set if sprite graphic collision latch locked */
static uint32_t g_ss_irq_set; /* Set if sprite sprite collision latch locked */
static uint32_t g_sg_irq_en;
static uint32_t g_ss_irq_en;
static uint32_t g_screen_line_ucycle_cnt; /* Micro cycles counter for one whole screen line */
static uint32_t g_screen_line_cnt; /* Line counter for the screen */
static uint32_t g_window_line_cnt; /* Line counter for the display window */
static uint32_t g_window_text_line_cnt; /* Counter for a text line, i.e. 8 horizontal lines */
static uint32_t g_window_bit_cnt; /* Bit counter for the display window (3 bits) */
static uint32_t g_window_video_cnt; /* AKA "VC" */
static uint32_t g_window_video_base_cnt; /* AKA "VCBASE" */
static uint32_t g_window_text_line_char_cnt; /* Char cnt for display window, resets when line ends */
static uint32_t g_wait_bad_line_cnt; /* Assistant counter for bad lines */
static uint32_t g_ECM; /* Graphic mode */
static uint32_t g_BMM; /* Graphic mode */
static uint32_t g_MCM; /* Graphic mode */
static uint32_t g_DEN; /* Screen enabled */
static uint32_t g_CSEL; /* CSEL value (border expand/shrink) */
static uint32_t g_RSEL; /* RSEL value (border expand/shrink) */
static uint32_t g_RSEL_active; /* When RSEL is 0 the display is 24 lines instead of 25 */
static uint32_t g_CSEL_active; /* When CSEL is 0 the display is 38 columns instead of 40 */

static int32_t g_ucycles_in_queue; /* Micro cycles left in vic queue waiting to be used */

static sprite_t g_sprites_a[8];
static vic_state_t g_vic_state;

static void event_write_xsprite0(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[0].x & 0xFF) != value)
  {
    g_sprites_a[0].x &= 0x100;
    g_sprites_a[0].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[0].y] |= 1 << 0;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite0(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[0].y != value)
  {
    /*
     * Remove sprite if it has been drawn on sprite layer. Otherwise
     * it might be drawn at wrong position if sprite has been
     * moved downward.
     */
    erase_sprite(0, g_sprites_a[0].x_prev_slayer_pos, g_sprites_a[0].y_prev_slayer_pos);

    /* Remove the old queue marker if any before entering a new */
    g_sprite_redraw_queue_a[g_sprites_a[0].y] &= ~(1 << 0);
    g_sprites_a[0].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[0].y] |= (1 << 0);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite1(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[1].x & 0xFF) != value)
  {
    g_sprites_a[1].x &= 0x100;
    g_sprites_a[1].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[1].y] |= 1 << 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite1(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[1].y != value)
  {
    erase_sprite(1, g_sprites_a[1].x_prev_slayer_pos, g_sprites_a[1].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[1].y] &= ~(1 << 1);
    g_sprites_a[1].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[1].y] |= (1 << 1);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite2(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[2].x & 0xFF) != value)
  {
    g_sprites_a[2].x &= 0x100;
    g_sprites_a[2].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[2].y] |= 1 << 2;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite2(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[2].y != value)
  {
    erase_sprite(2, g_sprites_a[2].x_prev_slayer_pos, g_sprites_a[2].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[2].y] &= ~(1 << 2);
    g_sprites_a[2].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[2].y] |= (1 << 2);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite3(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[3].x & 0xFF) != value)
  {
    g_sprites_a[3].x &= 0x100;
    g_sprites_a[3].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[3].y] |= 1 << 3;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite3(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[3].y != value)
  {
    erase_sprite(3, g_sprites_a[3].x_prev_slayer_pos, g_sprites_a[3].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[3].y] &= ~(1 << 3);
    g_sprites_a[3].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[3].y] |= (1 << 3);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite4(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[4].x & 0xFF) != value)
  {
    g_sprites_a[4].x &= 0x100;
    g_sprites_a[4].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[4].y] |= 1 << 4;
  }
  g_memory.io_p[addr] = value;
}

static void event_write_ysprite4(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[4].y != value)
  {
    erase_sprite(4, g_sprites_a[4].x_prev_slayer_pos, g_sprites_a[4].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[4].y] &= ~(1 << 4);
    g_sprites_a[4].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[4].y] |= (1 << 4);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite5(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[5].x & 0xFF) != value)
  {
    g_sprites_a[5].x &= 0x100;
    g_sprites_a[5].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[5].y] |= 1 << 5;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite5(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[5].y != value)
  {
    erase_sprite(5, g_sprites_a[5].x_prev_slayer_pos, g_sprites_a[5].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[5].y] &= ~(1 << 5);
    g_sprites_a[5].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[5].y] |= (1 << 5);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite6(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[6].x & 0xFF) != value)
  {
    g_sprites_a[6].x &= 0x100;
    g_sprites_a[6].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[6].y] |= 1 << 6;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite6(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[6].y != value)
  {
    erase_sprite(6, g_sprites_a[6].x_prev_slayer_pos, g_sprites_a[6].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[6].y] &= ~(1 << 6);
    g_sprites_a[6].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[6].y] |= (1 << 6);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xsprite7(uint16_t addr, uint8_t value)
{
  if((g_sprites_a[7].x & 0xFF) != value)
  {
    g_sprites_a[7].x &= 0x100;
    g_sprites_a[7].x |= value;
    g_sprite_redraw_queue_a[g_sprites_a[7].y] |= 1 << 7;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_ysprite7(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[7].y != value)
  {
    erase_sprite(7, g_sprites_a[7].x_prev_slayer_pos, g_sprites_a[7].y_prev_slayer_pos);

    g_sprite_redraw_queue_a[g_sprites_a[7].y] &= ~(1 << 7);
    g_sprites_a[7].y = value;
    g_sprite_redraw_queue_a[g_sprites_a[7].y] |= (1 << 7);
  }

  g_memory.io_p[addr] = value;
}

static void event_write_xmsb(uint16_t addr, uint8_t value)
{
  uint8_t i;

  for(i = 0; i < 8; i++)
  {
    uint32_t masked = (value << (8 - i) & 0x100);
    if((g_sprites_a[i].x & 0x100) != masked)  
    {
      if(masked)
      {
        g_sprites_a[i].x |= 0x100;
      }
      else
      {
        g_sprites_a[i].x &= ~0x100;
      }
      g_sprite_redraw_queue_a[g_sprites_a[i].y] |= (1 << i);
    }
  }

  g_memory.io_p[addr] = value;
}

static void event_write_cr1(uint16_t addr, uint8_t value)
{
  g_ECM = value & MASK_CR_1_ECM;
  g_BMM = value & MASK_CR_1_BMM;
  g_DEN = value & MASK_CR_1_DEN;
  g_RSEL = value & MASK_CR_1_RSEL;
  g_y_scroll = value & MASK_CR_1_YSCROLL;

  graphic_mode();

  /* Bit 7 in this reg is owned by raster (will not be written) */
  g_raster_line_irq &= 0x00FF;
  g_raster_line_irq |= (value & 0x80) << 1;
  g_memory.io_p[addr] &= ~0x7F;
  g_memory.io_p[addr] |= value & 0x7F;
}

static void event_write_raster(uint16_t addr, uint8_t value)
{
  /* Writing to this reg will set the raster line for irq */
  g_raster_line_irq &= 0xFF00;
  g_raster_line_irq |= (value & 0xFF);

  /*
   * Do not update memory, memory should always contain the
   * current raster line for the screen!
   */
}

static void event_write_sprite_enabled(uint16_t addr, uint8_t value)
{
  uint32_t i;
  uint32_t masked;

  for(i = 0; i < 8; i++)
  {
    masked = (value & (MASK_SPRITE_ENABLED_0 << i));
    if(g_sprites_a[i].enabled != masked)
    {
      /*
       * This needs to be handled directly, since otherwise the combination of a
       * disable + move + enable will mess things up.
       */
      if(masked)
      {
        g_sprites_a[i].enabled = masked;
        draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      }
      else
      {
        erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
        g_sprites_a[i].enabled = masked; /* Yes, this should be after erase */
      }
    }
  }

  g_memory.io_p[addr] = value;
}

static void event_write_cr2(uint16_t addr, uint8_t value)
{
  g_MCM = value & MASK_CR_2_MCM;
  g_CSEL = value & MASK_CR_2_CSEL;
  g_x_scroll = value & MASK_CR_2_XSCROLL;
  graphic_mode();
  g_memory.io_p[addr] = value;
}

static void event_write_sprite_yexp(uint16_t addr, uint8_t value)
{
  uint32_t i;
  uint32_t masked;

  for(i = 0; i < 8; i++)
  {
    masked = (value & (MASK_SPRITE_Y_EXP_0 << i));
    if(g_sprites_a[i].y_exp != masked)
    {
      g_sprites_a[i].y_exp = masked;
      g_sprites_a[i].y_exp_changed = 1;
    }
  }
  
  g_memory.io_p[addr] = value;
}

static void event_write_mem_pointers(uint16_t addr, uint8_t value)
{
  g_mem_pointer = value;

  /* CB13, CB12, CB11 */
  g_char_gen_offset = (value & 0x0E) << 10;

  /* CB13 only, used in bitmap modes */
  g_bitmap_gen_offset = (value & 0x08) << 10;

  /* VM13, VM12, VM11, VM10 */
  g_screen_ram_offset = ((value & 0xF0) << 6);

  g_memory.io_p[addr] = value;
}

static void event_write_irq_enable(uint16_t addr, uint8_t value)
{
  /* Raster interrupt enabled */
  if(value & MASK_INTRRUPT_ENABLE_ERST)
  {
    g_raster_irq_en = 1;
  }
  else
  {
    g_raster_irq_en = 0;
  }

  /* Collition sprite background */
  if(value & MASK_INTRRUPT_ENABLE_MBC)
  {
    g_sg_irq_en = 1;
  }
  else
  {
    g_sg_irq_en = 0;
  }

  /* Collition sprite sprite */
  if(value & MASK_INTRRUPT_ENABLE_MCM)
  {
    g_ss_irq_en = 1;
  }
  else
  {
    g_ss_irq_en = 0;
  }

  /* Light pen */
  if(value & MASK_INTRRUPT_ENABLE_LP)
  {
    ;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_prio(uint16_t addr, uint8_t value)
{
  uint32_t i;
  uint32_t masked;

  for(i = 0; i < 8; i++)
  {
    masked = (value & (MASK_SPRITE_DATA_PRIO_0 << i));
    if(g_sprites_a[i].prio != masked)
    {
      g_sprites_a[i].prio = masked;
      g_sprites_a[i].prio_changed = 1;
    }
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_multicolor(uint16_t addr, uint8_t value)
{
  uint32_t i;
  uint32_t masked;

  for(i = 0; i < 8; i++)
  {
    masked = (value & (MASK_SPRITE_MCOLOR_0 << i));
    if(g_sprites_a[i].multi_color_mode != masked)
    {
      g_sprites_a[i].multi_color_mode = masked;
      g_sprites_a[i].multi_color_mode_changed = 1;
    }
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_xexp(uint16_t addr, uint8_t value)
{
  uint32_t i;
  uint32_t masked;

  for(i = 0; i < 8; i++)
  {
    masked = (value & (MASK_SPRITE_X_EXP_0 << i));
    if(g_sprites_a[i].x_exp != masked)
    {
      g_sprites_a[i].x_exp = masked;
      g_sprites_a[i].x_exp_changed = 1;
    }
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_coll(uint16_t addr, uint8_t value)
{
  ; /* Cannot be written */
}

static void event_write_fg_coll(uint16_t addr, uint8_t value)
{
  ; /* Cannot be written */
}

static void event_write_sprite_multicolor0(uint16_t addr, uint8_t value)
{
  uint8_t i;

  g_sprite_multi_color_0 = value & MASK_SPRITE_MCOLOR_MM0;
  g_memory.io_p[addr] = value;

  for(i = 0; i < 8; i++)
  {
    g_sprites_a[i].multi_color_mode_changed = 1;
  }
}

static void event_write_sprite_multicolor1(uint16_t addr, uint8_t value)
{
  uint8_t i;

  g_sprite_multi_color_1 = value & MASK_SPRITE_MCOLOR_MM1;
  g_memory.io_p[addr] = value;

  for(i = 0; i < 8; i++)
  {
    g_sprites_a[i].multi_color_mode_changed = 1;
  }
}

static void event_write_sprite_color0(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[0].color != (value & MASK_COLOR_SPRITE_0_M0C))
  {    
    g_sprites_a[0].color = value & MASK_COLOR_SPRITE_0_M0C;
    g_sprites_a[0].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color1(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[1].color != (value & MASK_COLOR_SPRITE_1_M1C))
  {
    g_sprites_a[1].color = value & MASK_COLOR_SPRITE_1_M1C;
    g_sprites_a[1].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color2(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[2].color != (value & MASK_COLOR_SPRITE_2_M2C))
  {
    g_sprites_a[2].color = value & MASK_COLOR_SPRITE_2_M2C;
    g_sprites_a[2].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color3(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[3].color != (value & MASK_COLOR_SPRITE_3_M3C))
  {
    g_sprites_a[3].color = value & MASK_COLOR_SPRITE_3_M3C;
    g_sprites_a[3].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color4(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[4].color != (value & MASK_COLOR_SPRITE_4_M4C))
  {
    g_sprites_a[4].color = value & MASK_COLOR_SPRITE_4_M4C;
    g_sprites_a[4].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color5(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[5].color != (value & MASK_COLOR_SPRITE_5_M5C))
  {
    g_sprites_a[5].color = value & MASK_COLOR_SPRITE_5_M5C;
    g_sprites_a[5].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color6(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[6].color != (value & MASK_COLOR_SPRITE_6_M6C))
  {
    g_sprites_a[6].color = value & MASK_COLOR_SPRITE_6_M6C;
    g_sprites_a[6].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_sprite_color7(uint16_t addr, uint8_t value)
{
  if(g_sprites_a[7].color != (value & MASK_COLOR_SPRITE_7_M7C))
  {
    g_sprites_a[7].color = value & MASK_COLOR_SPRITE_7_M7C;
    g_sprites_a[7].color_changed = 1;
  }

  g_memory.io_p[addr] = value;
}

static void event_write_irq(uint16_t addr, uint8_t value)
{
  g_memory.io_p[addr] &= ~(value & 0x7F);

  if((g_memory.io_p[addr] & 0x0F) == 0)
  {
    g_memory.io_p[addr] &= ~MASK_INTRRUPT_IRQ; /* Reset bit, i.e no irq */
  }
}

static uint8_t event_read_color_ram(uint16_t addr)
{
  /*
   * Note that only the 4 lower bits are connected to char rom,
   * the higher "nybble" will usually be of random nature.
   */
  uint8_t reg = g_memory.io_p[addr];
  reg &= 0x0F;
  reg |= g_dot_matrix & 0xF0;
  return reg;
}

static uint8_t event_read_cr2(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xC0; /* 2 Bits are always read as 1 */
}

static uint8_t event_read_irq_enable(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_irq(uint16_t addr)
{
  return g_memory.io_p[addr] | 0x70; /* 3 Bits are always read as 1 */
}

static uint8_t event_read_color_border(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_bg_0(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_bg_1(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_bg_2(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_bg_3(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_mcolor_0(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_mcolor_1(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_0(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_1(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_2(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_3(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_4(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_5(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_6(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_color_sprite_7(uint16_t addr)
{
  return g_memory.io_p[addr] | 0xF0; /* 4 Bits are always read as 1 */
}

static uint8_t event_read_ss_coll(uint16_t addr)
{
  uint8_t reg = g_memory.io_p[addr];
  g_memory.io_p[addr] = 0x00; /* Is cleared when read */
  g_ss_irq_set = 0;
  return reg;
}

static uint8_t event_read_sg_coll(uint16_t addr)
{
  uint8_t reg = g_memory.io_p[addr];
  g_memory.io_p[addr] = 0x00; /* Is cleared when read */
  g_sg_irq_set = 0;
  return reg;
}

static uint8_t event_read_unused(uint16_t addr)
{
  return 0xFF;
}

static inline void load_dot_matrix_STM_MTM()
{
  /* First calculate offset */
  uint32_t char_offset =
      (g_char_pointers_a[g_window_text_line_char_cnt] << 3) + /* AKA D0-D7 */
      g_window_row_cnt; /* AKA RC0-RC2 */

  /* Vic will always see char rom at 0x1000 - 0x1FFF for bank 0 and 2 */
  if((g_char_gen_offset == 0x1000 || g_char_gen_offset == 0x1800) && /* AKA CB11-CB13 */
    (g_bank == 0 || g_bank == 2))
  {
    g_dot_matrix = *(g_crom_p + (g_char_gen_offset & 0x0800) + char_offset); /* AKA g-access */
  }
  else
  {
    /*
     * AKA g-access.
     * When reading from ram, the character code offset needs to be complemented
     * with the vic bank address offset (1) and the character generator offset (2).
     * The offset to dot matrix will be represented as BANK | CB11-CB13 | D0-D7 | RC0-RC2.
     */
    uint32_t ram_offset = (g_bank << 14) | g_char_gen_offset | char_offset;
    g_dot_matrix = *(g_ram_p + ram_offset);
  }
}

static inline void load_dot_matrix_SBM_MBM()
{
  /* AKA g-access */
  uint32_t bitmap_offset = (g_bank << 14) +
      g_bitmap_gen_offset + /* AKA CB13 */
      (g_window_video_cnt << 3) + /* AKA VC0-VC9 */
      g_window_row_cnt; /* AKA RC0-RC2 */

  g_dot_matrix = *(g_ram_p + bitmap_offset);
  g_dot_matrix_color = g_screen_ram_p[g_window_video_cnt]; /* AKA VC0-VC9 */
}

static inline void load_dot_matrix_ECM()
{
  /* First calculate offset */
  uint32_t char_offset =
      ((g_char_pointers_a[g_window_text_line_char_cnt] & 0x3F) << 3) + /* AKA D0-D7 */
      (g_window_row_cnt); /* AKA RC0-RC2 */

  /* Vic will always see char rom at 0x1000 - 0x1FFF for bank 0 and 2 */
  if((g_char_gen_offset == 0x1000 || g_char_gen_offset == 0x1800) && /* AKA CB11-CB13 */
    (g_bank == 0 || g_bank == 2))
  {
    g_dot_matrix = *(g_crom_p + (g_char_gen_offset & 0x0800) + char_offset); /* AKA g-access */
  }
  else
  {
    /*
     * AKA g-access.
     * When reading from ram, the character code offset needs to be complemented
     * with the vic bank address offset (1) and the character generator offset (2).
     * The offset to dot matrix will be represented as BANK | CB11-CB13 | D0-D7 | RC0-RC2.
     */
    uint32_t ram_offset = (g_bank << 14) | g_char_gen_offset | char_offset;
    g_dot_matrix = *(g_ram_p + ram_offset);
  }
}

static inline void load_dot_matrix_BORDER()
{
  g_dot_matrix = 0x00;
}

static inline void load_dot_matrix_BAD()
{
  g_dot_matrix = 0x00; 
}

static void graphic_mode()
{
  if(g_DEN == 0 || g_RSEL_active || g_CSEL_active)
  {
    g_graphic_mode = GRAPHIC_MODE_BORDER_EXT;
  }
  else if((g_BMM | g_ECM | g_MCM) == 0)
  {
    g_graphic_mode = GRAPHIC_MODE_STM;
  }
  else if((g_BMM | g_ECM | g_MCM) == MASK_CR_2_MCM)
  {
    g_graphic_mode = GRAPHIC_MODE_MTM;
  }
  else if((g_BMM | g_ECM | g_MCM) == (MASK_CR_1_BMM))
  {
    g_graphic_mode = GRAPHIC_MODE_SBM;
  }
  else if((g_BMM | g_ECM | g_MCM) == (MASK_CR_2_MCM | MASK_CR_1_BMM))
  {
    g_graphic_mode = GRAPHIC_MODE_MBM;
  }
  else if((g_BMM | g_ECM | g_MCM) == MASK_CR_1_ECM)
  {
    g_graphic_mode = GRAPHIC_MODE_ECM;
  }
  else if((g_BMM | g_ECM | g_MCM) == (MASK_CR_1_ECM | MASK_CR_2_MCM))
  {
    g_graphic_mode = GRAPHIC_MODE_ITM;
  }
  else if((g_BMM | g_ECM | g_MCM) == (MASK_CR_1_ECM | MASK_CR_1_BMM))
  {
    g_graphic_mode = GRAPHIC_MODE_IBM_1;
  }
  else if((g_BMM | g_ECM | g_MCM) == (MASK_CR_1_ECM | MASK_CR_1_BMM | MASK_CR_2_MCM))
  {
    g_graphic_mode = GRAPHIC_MODE_IBM_2;
  }
}

static void erase_sprite(uint32_t sprite, uint32_t at_x, uint32_t at_y)
{
  uint32_t x;
  uint32_t y;
  uint8_t *fg_sprite_p = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_FORGROUND];
  uint8_t *bg_sprite_p = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_BACKGROUND];
  uint8_t *map_p = g_pixel_sprite_mapping_start_p;
  uint8_t y_exp_factor = g_sprites_a[sprite].y_exp_factor;
  uint8_t x_exp_factor = g_sprites_a[sprite].x_exp_factor;
  uint32_t sprite_bitmap_offset = 0;
  uint8_t sprite_bf = (0x01 << sprite);

  /* Don't do anything if sprite is not there */
  if(!g_sprites_a[sprite].enabled || g_sprites_a[sprite].x_prev_slayer_pos == SPRITE_NO_POS)
  {
    return;
  }

  g_sprites_a[sprite].x_prev_slayer_pos = SPRITE_NO_POS;
  g_sprites_a[sprite].y_prev_slayer_pos = SPRITE_NO_POS;

  /* Set pointers to the start of relevant pixel */
  fg_sprite_p += at_y * PIXELS_MAX + at_x + PIXEL_SPRITE_X_START;
  bg_sprite_p += at_y * PIXELS_MAX + at_x + PIXEL_SPRITE_X_START;
  map_p += at_y * PIXELS_MAX + at_x + PIXEL_SPRITE_X_START;

  for(y = 0; y < SPRITE_HEIGHT * y_exp_factor; y++)
  {
    for(x = 0; x < SPRITE_WIDTH * x_exp_factor; x++)
    {
      if((map_p[x] & ~sprite_bf) == 0x00)
      {
        /* If no sprite pixel left at this location, then just remove pixel */
        fg_sprite_p[x] = SPRITE_NONE;
        bg_sprite_p[x] = SPRITE_NONE;
      }
      else
      {
        uint8_t map_tmp = map_p[x];
        uint8_t sprite_tmp = sprite_bf;
        uint8_t cnt = 0;
        uint8_t redraw_and_exit = 0;

        /*
         * There are at least one more sprite at this position, this
         * while loop will find out if action is needed and what is to be done.
         */
        while(cnt != 8)
        {
          if(redraw_and_exit &&
            (map_tmp & 0x01)) /* See if other sprite with lower prio exist and then render that pixel */
          {
            /* Some adjustments are needed, since the two sprites are not necessarily aligned */
            uint32_t x_sum = (at_x - g_sprites_a[cnt].x_prev_slayer_pos + x);
            uint32_t y_sum = (at_y - g_sprites_a[cnt].y_prev_slayer_pos + y);

            if(g_sprites_a[sprite].x_exp_factor == 0x02)
            {
              x_sum /=2;
            }

            if(g_sprites_a[sprite].y_exp_factor == 0x02)
            {
              y_sum /=2;              
            }

            //if(g_sprites_a[cnt].bitmap_a[y_sum * SPRITE_WIDTH + x_sum] == SPRITE_NONE) while(1){;}
            /* Now just render the sprite that was found "underneath" */
            if(!g_sprites_a[cnt].prio) /* 0 prio means rendering in front of fg pixels */
            {
              fg_sprite_p[x] = g_sprites_a[cnt].bitmap_a[y_sum * SPRITE_WIDTH + x_sum];
            }
            else
            {
              bg_sprite_p[x] = g_sprites_a[cnt].bitmap_a[y_sum * SPRITE_WIDTH + x_sum];
            }

            break;
          }
          else if((map_tmp & 0x01) &&
                  (sprite_tmp & 0x01) &&
                  !redraw_and_exit)
          {
            /*
             * Since sprite pixel has highest priority in the map, pixel needs to be erased
             * and replaced if other sprite exist at this location.
             */
            fg_sprite_p[x] = SPRITE_NONE;
            bg_sprite_p[x] = SPRITE_NONE;
            redraw_and_exit = 1;
          }
          else if((map_tmp & 0x01) &&
                  !(sprite_tmp & 0x01) &&
                  !redraw_and_exit)
          {
            /* No need to erase pixel, map contains sprite with higher priority than the sprite at hand */
            break;
          }
          else if(!(map_tmp & 0x01) &&
                  (sprite_tmp & 0x01) &&
                  !redraw_and_exit)
          {
            /* Dont do anything since there is no pixel to erase (sprite is not part of map) */
            break;
          }

          map_tmp >>= 1;
          sprite_tmp >>= 1;
          cnt++;
        }
      }

      map_p[x] &= ~sprite_bf;

      if(x_exp_factor == 0x02)
      {
        if(x % 2 == 1)
        {
          sprite_bitmap_offset++;
        }
      }
      else
      {
        sprite_bitmap_offset++;
      }
    }
    fg_sprite_p += PIXELS_MAX;
    bg_sprite_p += PIXELS_MAX;
    map_p += PIXELS_MAX;

    if(y_exp_factor == 0x02)
    {
      if(y % 2 == 0)
      {
        sprite_bitmap_offset -= SPRITE_WIDTH;
      }
    }

    /* Sprite is not on this row anymore */
    g_sprite_presence_a[at_y + y] &= ~sprite_bf;
  }
}

static void draw_sprite(uint32_t sprite, uint32_t at_x, uint32_t at_y)
{
  uint32_t x;
  uint32_t y;
  uint8_t *fg_sprite_p = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_FORGROUND];
  uint8_t *bg_sprite_p = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_BACKGROUND];
  uint8_t *map_p = g_pixel_sprite_mapping_start_p;
  uint8_t *pixel_p;
  uint8_t y_exp_factor = g_sprites_a[sprite].y_exp_factor;
  uint8_t x_exp_factor = g_sprites_a[sprite].x_exp_factor;
  uint8_t ss_coll = 0x00;
  uint8_t sprite_bf = (0x01 << sprite);

  /*
   * Don't do anything if sprite is disabled or if it is outside of scope
   * Don't forget x2 in the case the sprite is extended, dont care to check
   * if this sprite is expanded since it is not shown anyway.
   */
  if(!g_sprites_a[sprite].enabled || g_sprites_a[sprite].x > (PIXELS_MAX - SPRITE_WIDTH*2))
  {
    return;
  }

  /* Set pointer to the start of relevant pixel */
  fg_sprite_p += at_y * PIXELS_MAX + at_x + PIXEL_SPRITE_X_START;
  bg_sprite_p += at_y * PIXELS_MAX + at_x + PIXEL_SPRITE_X_START;
  map_p += at_y * PIXELS_MAX + at_x + PIXEL_SPRITE_X_START;

  /*
   * AKA g-access.
   * This will copy the dot matrix of whole sprite onto the sprite layer.
   * The offset to dot matrix will be represented as BANK | MP7-MP0 | MC5-MC0
   */
  pixel_p = g_sprites_a[sprite].bitmap_a;

  for(y = 0; y < SPRITE_HEIGHT * y_exp_factor; y++)
  {
    for(x = 0; x < SPRITE_WIDTH * x_exp_factor; x++)
    {
      if(*pixel_p != SPRITE_NONE)
      {
        uint8_t map_tmp = map_p[x];

        if(g_sprites_a[sprite].y >= LINE_DISP_WIND_START &&
           g_sprites_a[sprite].y <= LINE_BORD_LOWER_START) /* Only count collisions within display window lines */
        {
          /*
           * Sometime a sprite is redrawn when not first erased and this means that it will be part
           * of the map, so do not include the sprite at hand with looking for collisions
           */
          ss_coll |= map_p[x] & ~sprite_bf;
        }

        /*
         * Only draw if existing pixel has lower prio (higher number) than the
         * sprite at hand.
         */
        map_tmp <<= (8 - sprite);
        if(map_tmp == 0x00)
        {
          if(!g_sprites_a[sprite].prio) /* 0 prio means that sprite should be displayed in front of fg pixels */
          {
            fg_sprite_p[x] = *pixel_p;
          }
          else
          {
            bg_sprite_p[x] = *pixel_p;
          }
        }
        map_p[x] |= sprite_bf;
      }

      if(x_exp_factor == 0x02)
      {
        if(x % 2 == 1)
        {
          pixel_p++;
        }
      }
      else
      {
        pixel_p++;
      }
    }
    fg_sprite_p += PIXELS_MAX;
    bg_sprite_p += PIXELS_MAX;
    map_p += PIXELS_MAX;

    if(y_exp_factor == 0x02)
    {
      if(y % 2 == 0)
      {
        pixel_p -= SPRITE_WIDTH;
      }
    }

    g_sprite_presence_a[at_y + y] |= sprite_bf;
  }

  /*
   * The sprite has been drawn, now remember the position so that it
   * can be erased when needed.
   */
  g_sprites_a[sprite].x_prev_slayer_pos = at_x;
  g_sprites_a[sprite].y_prev_slayer_pos = at_y;

  /*
   * Sprite - Sprite collision
   *
   * This is breaking spec, vic hw will only detect collisions at
   * the point of rendering two non-transparent pixel at raster time,
   * not when program is moving sprites. However, it is not likely to cause
   * problems. Rationale is to gain some performance.
   */
  if(ss_coll != 0x00)
  {
    /* Update dedicated collision register */
    g_memory.io_p[REG_SS_COLL] |= ss_coll | sprite_bf;

    /* TODO: this should be here I think, but giving me problems */
    //g_memory.io_p[REG_INTRRUPT] |= MASK_INTRRUPT_IMMC;

    /* Will be ignored if locked (when ss coll reg is not read) */
    if(g_ss_irq_en && !g_ss_irq_set)
    {
      /* Set ss collision latch */
      g_memory.io_p[REG_INTRRUPT] |= MASK_INTRRUPT_IRQ | MASK_INTRRUPT_IMMC; /* Set IRQ line low*/
      g_ss_irq_set = 1;
    }
  }
}

static void move_sprite(uint32_t sprite, uint32_t from_x, uint32_t from_y, uint32_t to_x, uint32_t to_y)
{
  /* Erase the sprite from display layer */
  erase_sprite(sprite, from_x, from_y);

  /* Draw the sprite at its new location */
  draw_sprite(sprite, to_x, to_y);
}

static void render_sprite(uint32_t sprite, uint32_t checksum, uint8_t *base_p)
{
  uint32_t x;
  uint32_t y;

  /*
   * AKA g-access.
   * This will copy the dot matrix of whole sprite onto the sprite bitmap area.
   * The offset to dot matrix will be represented as BANK | MP7-MP0 | MC5-MC0
   */
  for(y = 0; y < SPRITE_HEIGHT; y++)
  {
    for(x = 0; x < SPRITE_WIDTH/8; x++) /* 3 bytes per sprite bitmap line */
    {
      uint32_t i;
      uint8_t dot_matrix = *(base_p + y * SPRITE_WIDTH/8 + x);

      if(g_sprites_a[sprite].multi_color_mode)
      {
        /* Rendering takes place here */
        for(i = 0; i < 4; i++)
        {
          uint8_t color = SPRITE_NONE;

          switch(dot_matrix & 0xC0)
          {
          case 0x00:
            /* transparent */
            break;
          case 0x40:
            color = g_sprite_multi_color_0;
            break;
          case 0x80:
            color = g_sprites_a[sprite].color;
            break;
          case 0xC0:
            color = g_sprite_multi_color_1;
            break;
          }

          dot_matrix <<= 2;
          g_sprites_a[sprite].bitmap_a[y * SPRITE_WIDTH + x * 8 + i * 2] = color;
          g_sprites_a[sprite].bitmap_a[y * SPRITE_WIDTH + x * 8 + i * 2 + 1] = color;
        }
      }
      else /* hi-res mode */
      {
        for(i = 0; i < 8; i++)
        {
          uint8_t color = SPRITE_NONE;

          if(dot_matrix & 0x80)
          {
            color = g_sprites_a[sprite].color;
          }
          else
          {
            ; /* Transparent */
          }

          dot_matrix <<= 1;
          g_sprites_a[sprite].bitmap_a[y * SPRITE_WIDTH + x * 8 + i] = color;
        }
      }
    }
  }

  g_sprites_a[sprite].checksum = checksum;
}

static void handle_sprite_redraw_queue()
{
  /*
   * Are there any sprites that needs to be redrawn due to movement?
   * Calling this function needs to be done at very specific locations.
   */
  if(g_sprite_redraw_queue_a[g_screen_line_cnt])
  {
    uint8_t i;
    uint8_t tmp = g_sprite_redraw_queue_a[g_screen_line_cnt];
    for(i = 0; tmp != 0; i++)
    {
      if(tmp & 0x01)
      {
        /* Remember that this call will affect g_sprite_presence_a !! */
        move_sprite(i,
                    g_sprites_a[i].x_prev_slayer_pos,
                    g_sprites_a[i].y_prev_slayer_pos,
                    g_sprites_a[i].x,
                    g_sprites_a[i].y);
      }
      tmp >>= 1;
    }

    /* Remove redraw marker */
    g_sprite_redraw_queue_a[g_screen_line_cnt] = 0;
  }
}

static void refresh_sprites()
{
  int32_t i;

  for(i = 7; i >= 0; i--)
  {
    uint32_t checksum;
    uint8_t *base_p;
    uint32_t sprite_offset;

    if(!g_sprites_a[i].enabled) /* Just bail if not relevant */
    {
      continue;
    }

    if((g_sprite_presence_a[g_screen_line_cnt] & (1 << i)) == 0x00)
    {
      continue;
    }

    /* First calculate offset */
    sprite_offset =
        (*g_sprite_pointer_ap[i] << 6) + /* AKA MP7-MP0 */
        0; /* AKA MC5-MC0 */

    /* Vic will always see char rom at 0x1000 - 0x1FFF for bank 0 and 2 */
    if((sprite_offset >= 0x1000 && sprite_offset < 0x2000) &&
      (g_bank == 0 || g_bank == 2))
    {
      base_p = g_crom_p + sprite_offset;
    }
    else
    {
      base_p = g_ram_p + (g_bank << 14) + sprite_offset;
    }

    /* First gnerate the checksum for bitmaps */
    checksum = g_if_host.if_host_calc.calc_checksum_fp((uint8_t *)base_p,
                                                     SPRITE_DOT_MATRIX_SIZE);

    /* If checksum has changed, then sprite needs to be rendered */
    if(checksum != g_sprites_a[i].checksum)
    {
      /* Since draw_sprite skips transparent pixels */
      erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
      render_sprite(i, checksum, base_p);
      draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
    }

    /* If sprite is disabled or enabled is handled directly and not here */

    /* If sprite prio changed */
    if(g_sprites_a[i].prio_changed)
    {
      erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
      draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      g_sprites_a[i].prio_changed = 0;
    }

    if(g_sprites_a[i].y_exp_changed)
    {
      if(g_sprites_a[i].y_exp)
      {
        erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
        g_sprites_a[i].y_exp_factor = 0x2; /* sprite x2 */
        draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      }
      else
      {
        erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
        g_sprites_a[i].y_exp_factor = 0x1; /* sprite x1 */
        draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      }
      g_sprites_a[i].y_exp_changed = 0; 
    }

    if(g_sprites_a[i].x_exp_changed)
    {
      if(g_sprites_a[i].x_exp)
      {
        erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
        g_sprites_a[i].x_exp_factor = 0x2; /* sprite x2 */
        draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      }
      else
      {
        erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
        g_sprites_a[i].x_exp_factor = 0x1; /* sprite x1 */
        draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      }
      g_sprites_a[i].x_exp_changed = 0; 
    }

    /* If sprite color/multi color changed */
    if(g_sprites_a[i].color_changed || g_sprites_a[i].multi_color_mode_changed)
    {
      erase_sprite(i, g_sprites_a[i].x_prev_slayer_pos, g_sprites_a[i].y_prev_slayer_pos);
      render_sprite(i, checksum, base_p);
      draw_sprite(i, g_sprites_a[i].x, g_sprites_a[i].y);
      g_sprites_a[i].color_changed = 0;
      g_sprites_a[i].multi_color_mode_changed = 0;
    }
  }
}

static void new_sprite_line()
{
  uint32_t i;
  uint8_t *base_p;

  base_p = g_memory.ram_p +
      (g_bank << 14) +
      g_screen_ram_offset +
      (0x7F << 3);

  for(i = 0; i < 8; i++)
  {
    /*
     * AKA p-access.
     * Sprite pointer will be represented as BANK | VM13-VM10 | sprite number
     */
    g_sprite_pointer_ap[i] = base_p + i;
  }
}

static void new_line()
{
  g_memory.io_p[REG_CR_1] &= ~MASK_CR_1_RST8;
  g_memory.io_p[REG_CR_1] |= (g_screen_line_cnt >> 1) & MASK_CR_1_RST8;
  g_memory.io_p[REG_RASTER] = g_screen_line_cnt & 0xFF;

  if(g_screen_line_cnt == g_raster_line_irq)
  {
    /*
     * Set raster match latch.
     */
    g_memory.io_p[REG_INTRRUPT] |= MASK_INTRRUPT_IRST;

    if(g_raster_irq_en)
    {
      g_memory.io_p[REG_INTRRUPT] |= MASK_INTRRUPT_IRQ; /* Set IRQ line low*/
    }
  }
}

static void new_text_line()
{
  uint32_t vmli;

  g_screen_ram_p = g_memory.ram_p +
    (g_bank << 14) +
    g_screen_ram_offset;   /* AKA VM13-VM10 */

  /* Bad line in progress, fetch all character pointers for the text line */
  for(vmli = 0; vmli < 40; vmli++)
  {
    /*
     * AKA c-access.
     * Char pointer will be represented as BANK | VM13-VM10 | VC9-VC0
     */
    g_char_pointers_a[vmli] = g_screen_ram_p[g_window_video_cnt + vmli];
  }
}

static void new_frame()
{
  uint32_t fps_time;

  if(g_display_frame) /* Only flip buffer if it was actually used */
  {
    /* Flip the buffer and get a new one */
    g_if_host.if_host_disp.disp_flip_fp(&g_layer_addr_start_p);
  }

  if(g_lock_frame_rate)
  {
    if(g_frames_until_lock == 0)
    {
      /* One frame should take 20ms for PAL */
      do
      {
        fps_time = g_if_host.if_host_time.time_get_ms_fp();
      } while((fps_time - g_fps_saved_time) < NO_OF_FRAMES_LOCK * 20);

      g_frames_until_lock = NO_OF_FRAMES_LOCK;
      g_fps_saved_time = fps_time;
    }
    g_frames_until_lock--;
  }

  if(g_frames_until_stats == 0)
  {
    fps_time = g_if_host.if_host_time.time_get_ms_fp();
    g_if_host.if_host_stats.stats_fps_fp(calc_fps(fps_time, g_fps_stats_time));
    g_frames_until_stats = NO_OF_FRAMES_STATS;
    g_fps_stats_time = fps_time;
  }
  g_frames_until_stats--;
}

void sg_coll_detected(uint8_t sprite_bf)
{
  /* Update dedicated collision register */
  g_memory.io_p[REG_SG_COLL] |= sprite_bf;

  /* TODO: I think it should be set here, but giving me problems ... */
  // g_memory.io_p[REG_INTRRUPT] |= MASK_INTRRUPT_IMBC;

  /* Will be ignored if locked (when sg coll reg is not read) */
  if(g_sg_irq_en && !g_sg_irq_set)
  {
    /* Set sg collision latch */
    g_memory.io_p[REG_INTRRUPT] |= MASK_INTRRUPT_IRQ | MASK_INTRRUPT_IMBC; /* Set IRQ line low*/
    g_sg_irq_set = 1;
  }
}

static inline void output_pixel_STM()
{
  uint8_t pixel1;
  uint8_t sprite_fg_pixel1 = SPRITE_NONE;
  uint8_t sprite_bg_pixel1 = SPRITE_NONE;
  uint8_t graphic_fg_pixel = 0;
  
  if((g_dot_matrix >> (7 - g_window_bit_cnt)) & 0x1)
  {
    pixel1 = g_fg_color_p[g_window_video_cnt] & 0x0F;
    graphic_fg_pixel = 1;
  }
  else
  {
    pixel1 = *g_bg_color_pp[0] & MASK_COLOR_BG_B0C;
  }

  /* Check if any sprite exist on this row */
  if(g_sprite_present_on_current_line)
  {
    /* Check if any sprite is on this exact location */
    if(*g_pixel_sprite_mapping_p)
    {
      /* Check if fg sprite exist */
      sprite_fg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;

      /* Also check for bg sprite */
      sprite_bg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;

      /* If fg sprite pixel exist, then it will be visible */
      if(sprite_fg_pixel1 != SPRITE_NONE)
      {
        pixel1 = sprite_fg_pixel1;
      }
      /* bg sprite pixel will only be plotted in front of bg graphic */
      else if(!graphic_fg_pixel) /* bg sprite pixel cannot be SPRITE_NONE here so no need to check */
      {
        pixel1 = sprite_bg_pixel1;
      }

      /* Check any collisions with fg graphics */
      if(graphic_fg_pixel)
      {
        sg_coll_detected(*g_pixel_sprite_mapping_p);
      }
    }
    else
    {
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
    }
  }

  /* Plot pixel on screen */
  *g_layer_addr_p++ = pixel1;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = pixel1;
#endif

  /* Step forward */
  g_pixel_sprite_mapping_p++;
  g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
  g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
  g_window_bit_cnt++;
}

static inline void output_pixel_ECM()
{
  uint8_t pixel1 = 0x0000;
  uint8_t sprite_fg_pixel1 = SPRITE_NONE;
  uint8_t sprite_bg_pixel1 = SPRITE_NONE;
  uint8_t graphic_fg_pixel = 0;

  if((g_dot_matrix >> (7 - g_window_bit_cnt)) & 0x1)
  {
    pixel1 = g_fg_color_p[g_window_video_cnt] & 0x0F;
    graphic_fg_pixel = 1;
  }
  else /* Backgound color determined by VM13, VM12 bits of mem pointer */
  {
    switch(g_mem_pointer & 0xC0)
    {
    case 0x00:
      pixel1 = *g_bg_color_pp[0] & MASK_COLOR_BG_B0C;
      break;
    case 0x40:
      pixel1 = *g_bg_color_pp[1] & MASK_COLOR_BG_B1C;
      break;
    case 0x80:
      pixel1 = *g_bg_color_pp[2] & MASK_COLOR_BG_B2C;
      break;
    case 0xC0:
      pixel1 = *g_bg_color_pp[3] & MASK_COLOR_BG_B3C;
      break;
    default:
      ;
    }
  }

  if(g_sprite_present_on_current_line)
  {
    if(*g_pixel_sprite_mapping_p)
    {
      sprite_fg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      sprite_bg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;

      if(sprite_fg_pixel1 != SPRITE_NONE)
      {
        pixel1 = sprite_fg_pixel1;
      }
      else if(!graphic_fg_pixel)
      {
        pixel1 = sprite_bg_pixel1;
      }

      if(graphic_fg_pixel)
      {
        sg_coll_detected(*g_pixel_sprite_mapping_p);
      }
    }
    else
    {
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
    }
  }

  *g_layer_addr_p++ = pixel1;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = pixel1;
#endif

  g_pixel_sprite_mapping_p++;
  g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
  g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
  g_window_bit_cnt++;
}

static inline void output_pixel_MTM()
{
  uint8_t pixel1;
  uint8_t sprite_fg_pixel1 = SPRITE_NONE;
  uint8_t sprite_bg_pixel1 = SPRITE_NONE;
  uint8_t graphic_fg_pixel = 0;

  if(g_fg_color_p[g_window_video_cnt] & MASK_MC_FLAG)
  {
    uint8_t window_bit_cnt = g_window_bit_cnt;

    if(window_bit_cnt & 0x01)
    {
      window_bit_cnt--;
    }

    switch((g_dot_matrix >> (6 - window_bit_cnt)) & 0x3)
    {
    case 0x00:
        pixel1 = *g_bg_color_pp[0] & MASK_COLOR_BG_B0C;
      break;
    case 0x01:
        pixel1 = *g_bg_color_pp[1] & MASK_COLOR_BG_B1C;
      break;
    case 0x02:
        pixel1 = *g_bg_color_pp[2] & MASK_COLOR_BG_B2C;
        graphic_fg_pixel = 1;
      break;
    case 0x03:
        pixel1 = g_fg_color_p[g_window_video_cnt] & MASK_COLOR_MTM;
        graphic_fg_pixel = 1;
      break;
    default:
      pixel1 = 0;
    }

    if(g_sprite_present_on_current_line)
    {
      if(*g_pixel_sprite_mapping_p)
      {
        sprite_fg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
        sprite_bg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;

        if(sprite_fg_pixel1 != SPRITE_NONE)
        {
          pixel1 = sprite_fg_pixel1;
        }
        else if(!graphic_fg_pixel)
        {
          pixel1 = sprite_bg_pixel1;
        }

        if(graphic_fg_pixel)
        {
          sg_coll_detected(*g_pixel_sprite_mapping_p);
        }
      }
      else
      {
        g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
        g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
      }
    }

    *g_layer_addr_p++ = pixel1;
#ifdef SCREEN_X2
    *g_layer_addr_p++ = pixel1;
#endif

    g_pixel_sprite_mapping_p++;
    g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
    g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
    g_window_bit_cnt++;
  }
  else
  {
    if((g_dot_matrix >> (7 - g_window_bit_cnt)) & 0x1)
    {
      pixel1 = g_fg_color_p[g_window_video_cnt] & MASK_COLOR_MTM;
      graphic_fg_pixel = 1;
    }
    else
    {
      pixel1 = *g_bg_color_pp[0] & MASK_COLOR_BG_B0C;
    }

    if(g_sprite_present_on_current_line)
    {
      if(*g_pixel_sprite_mapping_p)
      {
        sprite_fg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
        sprite_bg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;

        if(sprite_fg_pixel1 != SPRITE_NONE)
        {
          pixel1 = sprite_fg_pixel1;
        }
        else if(!graphic_fg_pixel)
        {
          pixel1 = sprite_bg_pixel1;
        }

        if(graphic_fg_pixel)
        {
          sg_coll_detected(*g_pixel_sprite_mapping_p);
        }
      }
      else
      {
        g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
        g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
      }
    }

    *g_layer_addr_p++ = pixel1;
#ifdef SCREEN_X2
    *g_layer_addr_p++ = pixel1;
#endif

    g_pixel_sprite_mapping_p++;
    g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
    g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
    g_window_bit_cnt++;
  }
}

static inline void output_pixel_SBM()
{
  uint8_t pixel1;
  uint8_t sprite_fg_pixel1 = SPRITE_NONE;
  uint8_t sprite_bg_pixel1 = SPRITE_NONE;
  uint8_t graphic_fg_pixel = 0;
  
  if((g_dot_matrix >> (7 - g_window_bit_cnt)) & 0x1)
  {
    pixel1 = g_dot_matrix_color >> 4;
    graphic_fg_pixel = 1;
  }
  else
  {
    pixel1 = g_dot_matrix_color & 0x0F;
  }

  if(g_sprite_present_on_current_line)
  {
    if(*g_pixel_sprite_mapping_p)
    {
      sprite_fg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      sprite_bg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;

      if(sprite_fg_pixel1 != SPRITE_NONE)
      {
        pixel1 = sprite_fg_pixel1;
      }
      else if(!graphic_fg_pixel)
      {
        pixel1 = sprite_bg_pixel1;
      }

      if(graphic_fg_pixel)
      {
        sg_coll_detected(*g_pixel_sprite_mapping_p);
      }
    }
    else
    {
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
    }
  }

  *g_layer_addr_p++ = pixel1;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = pixel1;
#endif

  g_pixel_sprite_mapping_p++;
  g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
  g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
  g_window_bit_cnt++;
}

static inline void output_pixel_MBM()
{
  uint8_t pixel1;
  uint8_t sprite_fg_pixel1 = SPRITE_NONE;
  uint8_t sprite_bg_pixel1 = SPRITE_NONE;
  uint8_t graphic_fg_pixel = 0;
  uint8_t window_bit_cnt = g_window_bit_cnt;

  if(window_bit_cnt & 0x01)
  {
    window_bit_cnt--;
  }

  switch((g_dot_matrix >> (6 - window_bit_cnt)) & 0x3)
  {
  case 0x00:
      pixel1 = *g_bg_color_pp[0] & MASK_COLOR_BG_B0C;
    break;
  case 0x01:
      pixel1 = g_dot_matrix_color >> 4;
    break;
  case 0x02:
      pixel1 = g_dot_matrix_color & 0x0F;
      graphic_fg_pixel = 1;
    break;
  case 0x03:
      pixel1 = g_fg_color_p[g_window_video_cnt] & 0x0F;
      graphic_fg_pixel = 1;
    break;
  default:
    pixel1 = 0;
  }

  if(g_sprite_present_on_current_line)
  {
    if(*g_pixel_sprite_mapping_p)
    {
      sprite_fg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      sprite_bg_pixel1 = *g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;

      if(sprite_fg_pixel1 != SPRITE_NONE)
      {
        pixel1 = sprite_fg_pixel1;
      }
      else if(!graphic_fg_pixel)
      {
        pixel1 = sprite_bg_pixel1;
      }

      if(graphic_fg_pixel)
      {
        sg_coll_detected(*g_pixel_sprite_mapping_p);
      }
    }
    else
    {
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
      g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
    }
  }

  *g_layer_addr_p++ = pixel1;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = pixel1;
#endif

  g_pixel_sprite_mapping_p++;
  g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
  g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
  g_window_bit_cnt++;
}

static inline void output_pixel_BORDER_EXT()
{
  uint8_t color = *g_border_color_p & MASK_COLOR_BORDER_EC;

  /*
   * Dont forget about the sprites, otherwise they will be
   * off in position if extended borders are used.
   */
  if(g_sprite_present_on_current_line)
  {
    g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
    g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
  }

  *g_layer_addr_p++ = color;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = color;
#endif

  g_pixel_sprite_mapping_p++;
  g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
  g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
  g_window_bit_cnt++;
}

static inline void output_pixel_BORDER()
{
  uint8_t color = *g_border_color_p & MASK_COLOR_BORDER_EC;

  *g_layer_addr_p++ = color;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = color;
#endif
}

static inline void output_pixel_BAD()
{
  uint8_t color;
  color = 0;

  if(g_sprite_present_on_current_line)
  {
    g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND]++;
    g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND]++;
  }

  *g_layer_addr_p++ = color;
#ifdef SCREEN_X2
  *g_layer_addr_p++ = color;
#endif

  g_pixel_sprite_mapping_p++;
  g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
  g_ucycles_in_queue -= UCYCLE_PER_PIXEL;
  g_window_bit_cnt++;
}

static uint8_t calc_fps(uint32_t time_now, uint32_t time_start)
{
  return (1000 * NO_OF_FRAMES_STATS) / (time_now - time_start);
}

void vic_init()
{
  uint32_t i;

  /* Populate pointers */
  g_crom_p = g_memory.crom_p + OFFSET_CROM;
  g_ram_p = g_memory.ram_p + OFFSET_RAM;
  g_border_color_p = g_memory.io_p + REG_COLOR_BORDER;
  g_fg_color_p = g_memory.io_p + OFFSET_COLOR_RAM;
  g_bg_color_pp[0] = g_memory.io_p + REG_COLOR_BG_0;
  g_bg_color_pp[1] = g_memory.io_p + REG_COLOR_BG_1;
  g_bg_color_pp[2] = g_memory.io_p + REG_COLOR_BG_2;
  g_bg_color_pp[3] = g_memory.io_p + REG_COLOR_BG_3;

  /* Set initial variables */
  g_vic_state = VIC_STATE_VERTICAL_BLANKING;
  g_screen_line_cnt = 0;
  g_screen_line_ucycle_cnt = 0;
  g_ucycles_in_queue = 0;
  g_graphic_mode = GRAPHIC_MODE_STM;
  g_frames_until_stats = NO_OF_FRAMES_STATS;
  g_frames_until_lock = NO_OF_FRAMES_LOCK;
  g_x_scroll = 0;
  g_y_scroll = 3;

  /* Clear sprite memory */
  for(i = 0; i < LINE_MAX * PIXELS_MAX; i++)
  {
    *(g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND] + i) = SPRITE_NONE;
  }

  for(i = 0; i < LINE_MAX * PIXELS_MAX; i++)
  {
    *(g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND] + i) = SPRITE_NONE;
  }

  /* Subscribe for bus write events */
  bus_event_write_subscribe(REG_X_COOR_SPRITE_0, event_write_xsprite0);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_0, event_write_ysprite0);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_1, event_write_xsprite1);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_1, event_write_ysprite1);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_2, event_write_xsprite2);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_2, event_write_ysprite2);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_3, event_write_xsprite3);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_3, event_write_ysprite3);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_4, event_write_xsprite4);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_4, event_write_ysprite4);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_5, event_write_xsprite5);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_5, event_write_ysprite5);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_6, event_write_xsprite6);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_6, event_write_ysprite6);
  bus_event_write_subscribe(REG_X_COOR_SPRITE_7, event_write_xsprite7);
  bus_event_write_subscribe(REG_Y_COOR_SPRITE_7, event_write_ysprite7);
  bus_event_write_subscribe(REG_MSB_X_COOR, event_write_xmsb);
  bus_event_write_subscribe(REG_CR_1, event_write_cr1);
  bus_event_write_subscribe(REG_RASTER, event_write_raster);
  bus_event_write_subscribe(REG_SPRITE_ENABLED, event_write_sprite_enabled);
  bus_event_write_subscribe(REG_CR_2, event_write_cr2);
  bus_event_write_subscribe(REG_SPRITE_Y_EXP, event_write_sprite_yexp);
  bus_event_write_subscribe(REG_MEM_POINTERS, event_write_mem_pointers);
  bus_event_write_subscribe(REG_INTRRUPT_ENABLE, event_write_irq_enable);
  bus_event_write_subscribe(REG_SPRITE_DATA_PRIO, event_write_sprite_prio);
  bus_event_write_subscribe(REG_SPRITE_MULTICOLOR, event_write_sprite_multicolor);
  bus_event_write_subscribe(REG_SPRITE_X_EXP, event_write_sprite_xexp);
  bus_event_write_subscribe(REG_SS_COLL, event_write_sprite_coll);
  bus_event_write_subscribe(REG_SG_COLL, event_write_fg_coll);
  bus_event_write_subscribe(REG_SPRITE_MCOLOR_0, event_write_sprite_multicolor0);
  bus_event_write_subscribe(REG_SPRITE_MCOLOR_1, event_write_sprite_multicolor1);
  bus_event_write_subscribe(REG_COLOR_SPRITE_0, event_write_sprite_color0);
  bus_event_write_subscribe(REG_COLOR_SPRITE_1, event_write_sprite_color1);
  bus_event_write_subscribe(REG_COLOR_SPRITE_2, event_write_sprite_color2);
  bus_event_write_subscribe(REG_COLOR_SPRITE_3, event_write_sprite_color3);
  bus_event_write_subscribe(REG_COLOR_SPRITE_4, event_write_sprite_color4);
  bus_event_write_subscribe(REG_COLOR_SPRITE_5, event_write_sprite_color5);
  bus_event_write_subscribe(REG_COLOR_SPRITE_6, event_write_sprite_color6);
  bus_event_write_subscribe(REG_COLOR_SPRITE_7, event_write_sprite_color7);
  bus_event_write_subscribe(REG_INTRRUPT, event_write_irq);

  /* Subscribe for bus read events */
  bus_event_read_subscribe(REG_CR_2, event_read_cr2);
  bus_event_read_subscribe(REG_INTRRUPT_ENABLE, event_read_irq_enable);
  bus_event_read_subscribe(REG_INTRRUPT, event_read_irq);
  bus_event_read_subscribe(REG_COLOR_BORDER, event_read_color_border);
  bus_event_read_subscribe(REG_COLOR_BG_0, event_read_bg_0);
  bus_event_read_subscribe(REG_COLOR_BG_1, event_read_bg_1);
  bus_event_read_subscribe(REG_COLOR_BG_2, event_read_bg_2);
  bus_event_read_subscribe(REG_COLOR_BG_3, event_read_bg_3);
  bus_event_read_subscribe(REG_SPRITE_MCOLOR_0, event_read_mcolor_0);
  bus_event_read_subscribe(REG_SPRITE_MCOLOR_1, event_read_mcolor_1);
  bus_event_read_subscribe(REG_COLOR_SPRITE_0, event_read_color_sprite_0);
  bus_event_read_subscribe(REG_COLOR_SPRITE_1, event_read_color_sprite_1);
  bus_event_read_subscribe(REG_COLOR_SPRITE_2, event_read_color_sprite_2);
  bus_event_read_subscribe(REG_COLOR_SPRITE_3, event_read_color_sprite_3);
  bus_event_read_subscribe(REG_COLOR_SPRITE_4, event_read_color_sprite_4);
  bus_event_read_subscribe(REG_COLOR_SPRITE_5, event_read_color_sprite_5);
  bus_event_read_subscribe(REG_COLOR_SPRITE_6, event_read_color_sprite_6);
  bus_event_read_subscribe(REG_COLOR_SPRITE_7, event_read_color_sprite_7);
  bus_event_read_subscribe(REG_SS_COLL, event_read_ss_coll);
  bus_event_read_subscribe(REG_SG_COLL, event_read_sg_coll);

  /* Subscribe to color ram */
  for(i = 0; i < 1000; i++)
  {
    bus_event_read_subscribe(0xD800 + i, event_read_color_ram);
  }

  /* Subscribe to unused registers (e.g. some programs use 0xD030 to determine C128 or C64) */
  for(i = 0; i < 0x10; i++)
  {
    bus_event_read_subscribe(0xD02F + i, event_read_unused);
  }

  /* Clear pixel to sprite mapping */
  for(i = 0; i < LINE_MAX * PIXELS_MAX; i++)
  {
    g_pixel_sprite_mapping_start_p[i] = SPRITE_MAP_NONE;
  }

  /* Set initial values for sprites */
  for(i = 0; i < 8; i++)
  {
    g_sprites_a[i].y_exp_factor = 0x01;
    g_sprites_a[i].x_exp_factor = 0x01;
    g_sprites_a[i].y_prev_slayer_pos = SPRITE_NO_POS;
    g_sprites_a[i].x_prev_slayer_pos = SPRITE_NO_POS;
  }
}

void vic_set_bank(uint8_t value) /* Called by CIA2 that is subscribing for 0xDD00 */
{
  g_bank = (~value) & 0x3;
}

void vic_set_layer(uint8_t *layer_addr_p)
{
  g_layer_addr_p = layer_addr_p;
  g_layer_addr_start_p = layer_addr_p;
}

void vic_set_memory(uint8_t *mem_addr_p, vic_mem_sprite_t vic_mem_sprite)
{
  switch(vic_mem_sprite)
  {
    case VIC_MEM_SPRITE_BACKGROUND:
    case VIC_MEM_SPRITE_FORGROUND:
      g_sprite_layer_addr_aap[vic_mem_sprite] = mem_addr_p;
      g_sprite_layer_addr_start_aap[vic_mem_sprite] = mem_addr_p;
      break;
    case VIC_MEM_SPRITE_MAP:
      g_pixel_sprite_mapping_p = (uint8_t *)mem_addr_p;
      g_pixel_sprite_mapping_start_p = (uint8_t *)mem_addr_p;
      break;
    default:
      return;
  }
}

/*
+--------------------------------------------------------------+
|                       VERTICAL BLANKING                      |
+----+----------------------------------------------------+----+
|    |                                                    |    |
|    |                        BORDER                      |    |
|    |                                                    |    |
|    +---------+--------------------------------+---------+    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
| HB | BORDER  |         DISPLAY WINDOW         | BORDER  | HB |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    |         |                                |         |    |
|    +---------+--------------------------------+---------+    |
|    |                                                    |    |
|    |                        BORDER                      |    |
|    |                                                    |    |
+----+----------------------------------------------------+----+
|                       VERTICAL BLANKING                      |
+--------------------------------------------------------------+
*/

void vic_step(uint32_t cc)
{
  uint32_t go_again = 0;
  g_ucycles_in_queue += cc * UCYCLE_PER_PIXEL * 8;

GO_AGAIN:

  switch(g_vic_state)
  {
    case VIC_STATE_HOLD:
    {
      if(g_ucycles_in_queue >= UCYCLES_LINE)
      {
        g_screen_line_cnt++;
        g_ucycles_in_queue -= UCYCLES_LINE;

        new_line();

        /* Doubtful if this is needed */
        /*if(((g_screen_line_cnt & 0x7) == g_y_scroll) &&
          g_screen_line_cnt >= LINE_DISP_WIND_START &&
          g_screen_line_cnt < LINE_BORD_LOWER_START &&
          g_DEN)
        {
          g_ucycles_in_queue += UCYCLES_BAD_LINE;
        }*/

        if(g_screen_line_cnt == (LINE_MAX - 1))
        {
          g_vic_state = VIC_STATE_VERTICAL_BLANKING;
        }
      }
    }
    break;
    /* 28 lines are blanking lines (300 to 15) */
    case VIC_STATE_VERTICAL_BLANKING:
    {
      if(g_ucycles_in_queue >= UCYCLES_LINE)
      {
        g_screen_line_cnt++;
        g_ucycles_in_queue -= UCYCLES_LINE;

        if(g_screen_line_cnt == LINE_MAX)
        {
          g_screen_line_cnt = 0;
          g_window_video_base_cnt = 0;
          g_window_line_cnt = 0;
          g_window_text_line_cnt = 0;
          g_window_row_cnt = 0;

          new_frame();

          if(g_full_frame_rate)
          {
            g_display_frame = 1;
          }
          else
          {
            g_display_frame = !g_display_frame;
            if(g_display_frame == 0)
            {
              /* Skipping one frame ... */
              g_vic_state = VIC_STATE_HOLD;
              new_line(); /* Do not forget to report line */
              break;
            }
          }

          /* Reset the memory pointer to which pixels are drawn */
          g_layer_addr_p = g_layer_addr_start_p;

          /* Set the sprite layer so that it points to the very first pixel in display window */
          /* TODO: change when supporting sprites on whole screen */
          g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND] = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_FORGROUND] +
                                                           LINE_DISP_WIND_START * PIXELS_MAX +
                                                           PIXEL_DISP_WIND_START;

          g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND] = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_BACKGROUND] +
                                                           LINE_DISP_WIND_START * PIXELS_MAX +
                                                           PIXEL_DISP_WIND_START;
          g_pixel_sprite_mapping_p = g_pixel_sprite_mapping_start_p + LINE_DISP_WIND_START * PIXELS_MAX + PIXEL_DISP_WIND_START;
        }

        new_line();

        /* Last vblank line is 15 */
        if(g_screen_line_cnt == LINE_BORD_UPPER_START)
        {
          g_vic_state = VIC_STATE_VERTICAL_UPPER_BORDER;
        }
      }
    }
    break;
    case VIC_STATE_VERTICAL_UPPER_BORDER:
    {
      if(g_ucycles_in_queue >= UCYCLES_LINE)
      {
#ifdef HAVE_BORDERS
        {
          uint32_t i;

          for(i = 0; i < FORGROUND_WIDTH; i++)
          {
            output_pixel_BORDER();
          }
        }

#ifdef SCREEN_X2
        g_layer_addr_p += FORGROUND_WIDTH*2;
#endif
#endif

        g_screen_line_cnt++;
        g_ucycles_in_queue -= UCYCLES_LINE;

        new_line();

        /*
         * This must be done here since otherwise sprites with coordinates
         * that are in upper border will not be displayed at all.
         */
        handle_sprite_redraw_queue();

        /* Bad line condition can happen if 0x30 >= line <=0xF7 */
        if(g_screen_line_cnt == LINE_BAD_LOWER_COND)
        {
          g_vic_state = VIC_STATE_VERTICAL_WAIT_BAD_LINE;

          /*
           * Set the counter so that next state can know both when a line is done and
           * when the 14 cycle mark is crossed.
           */
          g_wait_bad_line_cnt = 0; /* 9 * 7 = 63 cycles */
        }
      }
    }
    break;
    case VIC_STATE_VERTICAL_WAIT_BAD_LINE: /* This state is only for bad lines outside of display window! */
    {
      /* Lets step 7 cycles at a time to speed things up */
      while(g_ucycles_in_queue >= UCYCLES_BAD_LINE_WAIT)
      {
        g_ucycles_in_queue -= UCYCLES_BAD_LINE_WAIT;
        g_wait_bad_line_cnt++;

        /*
         * In the first phase of cycle 14 of each line, VC is loaded from VCBASE
         * (VCBASE->VC) and VMLI is cleared. If there is a Bad Line Condition in
         * this phase, RC is also reset to zero.
         */
        if(g_wait_bad_line_cnt == 2)
        {
          g_window_video_cnt = g_window_video_base_cnt; /* (VCBASE->VC) */

          /* Bad line condition */
          if(((g_screen_line_cnt & 0x7) == g_y_scroll) &&
             g_DEN)
          {
            g_window_text_line_cnt++;
            g_window_row_cnt = 0;
            new_text_line();

            /* Give 40 cycles extra to vic itself */
            g_ucycles_in_queue += UCYCLES_BAD_LINE;
          }
        }

        if(g_wait_bad_line_cnt == 9)
        {
#ifdef HAVE_BORDERS
          {
            uint32_t i;

            for(i = 0; i < FORGROUND_WIDTH; i++)
            {
              output_pixel_BORDER();
            }
          }

#ifdef SCREEN_X2
          g_layer_addr_p += FORGROUND_WIDTH*2;
#endif
#endif

          g_wait_bad_line_cnt = 0; /* 9 * 7 = 63 cycles */
          g_screen_line_cnt++;
          g_window_row_cnt++;

          new_line();

          handle_sprite_redraw_queue();

          /* Last upper border line is 50 */
          if(g_screen_line_cnt == LINE_DISP_WIND_START)
          {
            g_vic_state = VIC_STATE_HORIZONTAL_LEFT_BLANKING;

            if(!g_RSEL)
            {
              /*
               * If display is set to 24 lines (RSEL=0) then line 51 to 54
               * should be part of the border. Is is the same with lines
               * 247 to 250. This is handled by overriding the graphic mode,
               * so that when entering the display window the border will be
               * rendered instead of the "real" curent graphic mode.
               */
              g_RSEL_active = 1;
              graphic_mode();
            }
            else
            {
              g_RSEL_active = 0;
              graphic_mode();
            }

            go_again = 1; /* Might have more cycles to spend! */
            break;
          }
        }
      }
    }
    break;
    case VIC_STATE_HORIZONTAL_LEFT_BLANKING:
    {
      while(g_ucycles_in_queue >= UCYCLE_PER_PIXEL)
      {
        g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
        g_ucycles_in_queue -= UCYCLE_PER_PIXEL;

        if(g_RSEL_active && (g_screen_line_cnt == LINE_EXT_UPPER_BORD))
        {
          /*
           * At line 55 the display should always be rendered using the
           * "real" current graphic mode regardless of RSEL.
           */
          g_RSEL_active = 0;
          graphic_mode();
        }

        if(!g_RSEL && (g_screen_line_cnt == LINE_EXT_LOWER_BORD))
        {
          /*
           * At line 247 the display should be rendered as border if
           * RSEL is 0.
           */
          g_RSEL_active = 1;
          graphic_mode();
        }

        if(g_screen_line_ucycle_cnt == PIXEL_LEFT_BORD_START * UCYCLE_PER_PIXEL)
        {
          g_vic_state = VIC_STATE_HORIZONTAL_LEFT_BORDER;

          go_again = 1; /* Might have more cycles to spend! */
          break;
        }
      }
    }
    break;
    case VIC_STATE_HORIZONTAL_LEFT_BORDER:
    {
      while(g_ucycles_in_queue >= UCYCLE_PER_PIXEL)
      {
#ifdef HAVE_BORDERS
        if(g_screen_line_ucycle_cnt >= (PIXEL_LEFT_BORD_START + 4) * UCYCLE_PER_PIXEL)
        {
          output_pixel_BORDER();
        }
#endif
        g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
        g_ucycles_in_queue -= UCYCLE_PER_PIXEL;

        /* At cycle 14 VC is loaded and if bad line, then RC is set to zero */
        if(g_screen_line_ucycle_cnt == PIXEL_BAD_LINE_CHECK * UCYCLE_PER_PIXEL)
        {
          g_window_video_cnt = g_window_video_base_cnt;

          /* Bad line condition */
          if((g_screen_line_cnt <= LINE_BAD_UPPER_COND) &&
            ((g_screen_line_cnt & 0x7) == g_y_scroll) &&
            g_DEN)
          {
            g_window_text_line_cnt++;
            g_window_row_cnt = 0;
            new_text_line();

            /* Give 40 cycles extra to vic itself */
            g_ucycles_in_queue += UCYCLES_BAD_LINE;
          }

          handle_sprite_redraw_queue();

          /* If sprites present, then get memory pointers and refresh them if needed */
          if(g_sprite_presence_a[g_screen_line_cnt])
          {
            g_sprite_present_on_current_line = 1;
            new_sprite_line();
            refresh_sprites();

            /* Vic will steal 2 cycles for every sprite on this row */
            g_ucycles_in_queue += __builtin_popcount(g_sprite_presence_a[g_screen_line_cnt]) * UCYCLE_BAD_SPRITE;
          }
          else
          {
            g_sprite_present_on_current_line = 0;
          }
        }

        if(g_screen_line_ucycle_cnt == PIXEL_DISP_WIND_START * UCYCLE_PER_PIXEL)
        {
          g_vic_state = VIC_STATE_DISPLAY_WINDOW;

          /*
           * If CSEL=0 the left border is extended by 7 pixels and the
           * right one by 9 pixels. However, since xscroll may also print
           * some extra border pixels so take that into account here.
           */
          if(!g_CSEL && !g_RSEL_active)
          {
            uint32_t i;

            for(i = 0; i < PIXELS_EXT_LEFT_BORD; i++)
            {
              output_pixel_BORDER_EXT();
            }

            g_window_bit_cnt -= g_x_scroll;
          }
          /*
           * Make x scroll function by skipping some pixels in the start
           * of the line in display window. The pixels still needs to be
           * rendered of course, but the bit counter is not incremented.
           */
          else if(g_x_scroll != 0)
          {
            uint32_t i;

            uint8_t window_bit_cnt_saved = g_window_bit_cnt;
            for(i = 0; i < g_x_scroll; i++)
            {
                /* Update pixel */
                OUTPUT_PIXEL();
            }
            g_window_bit_cnt = window_bit_cnt_saved;
          }

          LOAD_DOT_MATRIX();
          go_again = 1; /* Might have more cycles to spend! */
          break;
        }
      }
    }
    break;
    case VIC_STATE_DISPLAY_WINDOW:
    {
      while(g_ucycles_in_queue >= UCYCLE_PER_PIXEL)
      {
        /* Update pixel */
        OUTPUT_PIXEL();

        /* Column change */
        if(g_window_bit_cnt == CHAR_HORIZONTAL_LENGTH)
        {
          g_window_bit_cnt = 0;

          /* VC and VMLI are incremented after each g-access in display state */
          g_window_video_cnt++;
          g_window_text_line_char_cnt++;

          /* AKA g-access */
          LOAD_DOT_MATRIX();
        }

        if(!g_CSEL && !g_RSEL_active)
        {
          if(g_screen_line_ucycle_cnt == (PIXEL_RIGHT_BORD_START - PIXELS_EXT_RIGHT_BORD) * UCYCLE_PER_PIXEL)
          {
            uint32_t i;

            g_vic_state = VIC_STATE_HORIZONTAL_RIGHT_BORDER;

            for(i = 0; i < PIXELS_EXT_RIGHT_BORD; i++)
            {
              output_pixel_BORDER_EXT();
            }

            /*
             * If the window bit counter is not zero, it means that x scroll
             * was not zero when line started. This also means that the "real" line
             * (i.e. line excluding possible border pixels) will be shorter and
             * the last increment of video counter and tile counter will not happen.
             * It is needed to be done here.
             */
            if(g_window_bit_cnt != 0 && (g_window_video_cnt % 40 != 0))
            {
              g_window_bit_cnt = 0;
              g_window_video_cnt += 2;
              g_window_text_line_char_cnt += 2;
            }

            go_again = 1; /* Might have more cycles to spend! */
            break;
          }
        }

        if(g_screen_line_ucycle_cnt == PIXEL_RIGHT_BORD_START * UCYCLE_PER_PIXEL)
        {
          g_vic_state = VIC_STATE_HORIZONTAL_RIGHT_BORDER;

          /*
           * See below comment at same condition.
           */
          if(g_window_bit_cnt != 0 && (g_window_video_cnt % 40 != 0))
          {
            g_window_bit_cnt = 0;
            g_window_video_cnt++; 
            g_window_text_line_char_cnt++;
          }

          go_again = 1; /* Might have more cycles to spend! */
          break;
        }
      }
    }
    break;
    case VIC_STATE_HORIZONTAL_RIGHT_BORDER:
    {
      while(g_ucycles_in_queue >= UCYCLE_PER_PIXEL)
      {
#ifdef HAVE_BORDERS
        output_pixel_BORDER();
#endif
        g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
        g_ucycles_in_queue -= UCYCLE_PER_PIXEL;

        /* At cycle 58 vic checks RC == 7, if so then VCBASE is loaded from VC */
        if(g_screen_line_ucycle_cnt == PIXEL_LOAD_VCBASE * UCYCLE_PER_PIXEL)
        {
          if(g_window_row_cnt == 7)
          {
            g_window_video_base_cnt = g_window_video_cnt;
          }
        }

        if(g_screen_line_ucycle_cnt == PIXEL_RIGHT_BLANK_START * UCYCLE_PER_PIXEL)
        {
          g_vic_state = VIC_STATE_HORIZONTAL_RIGHT_BLANKING;
          go_again = 1; /* Might have more cycles to spend! */
          break;
        }
      }
    }
    break;
    case VIC_STATE_HORIZONTAL_RIGHT_BLANKING:
    {
      while(g_ucycles_in_queue >= UCYCLE_PER_PIXEL)
      {
        g_screen_line_ucycle_cnt += UCYCLE_PER_PIXEL;
        g_ucycles_in_queue -= UCYCLE_PER_PIXEL;

        if(g_screen_line_ucycle_cnt == PIXELS_MAX * UCYCLE_PER_PIXEL) /* New line */
        {
          g_screen_line_ucycle_cnt = 0;
          g_window_bit_cnt = 0;
          g_screen_line_cnt++;
          g_window_text_line_char_cnt = 0;

          new_line();

          g_window_line_cnt++;

          /* Last display line is 250 */
          if(g_screen_line_cnt == LINE_BORD_LOWER_START)
          {
            g_vic_state = VIC_STATE_VERTICAL_LOWER_BORDER;


#ifdef HAVE_BORDERS
#ifdef SCREEN_X2
            g_layer_addr_p += FORGROUND_WIDTH*2;
#else
            g_layer_addr_p += FORGROUND_WIDTH;
#endif
#endif

            go_again = 0;
            break;
          }
          else
          {
            g_vic_state = VIC_STATE_HORIZONTAL_LEFT_BLANKING;
            /* window row counter AKA RC is incremented if in display state */
            g_window_row_cnt++;

#ifdef SCREEN_X2
#ifdef HAVE_BORDERS
            g_layer_addr_p += FORGROUND_WIDTH*2;
#else
            g_layer_addr_p = g_layer_addr_start_p +
                                                        (g_screen_line_cnt*2 - LINE_DISP_WIND_START*2) * FORGROUND_WIDTH*2;
#endif
#else
            g_layer_addr_p = g_layer_addr_start_p +
                                                        (g_screen_line_cnt - LINE_DISP_WIND_START) * FORGROUND_WIDTH;
#endif

            /* Set the sprite layer so that it points to the very first pixel for the current line in display window */
            g_sprite_layer_addr_aap[VIC_MEM_SPRITE_FORGROUND] = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_FORGROUND] +
                                                             g_screen_line_cnt * PIXELS_MAX +
                                                             PIXEL_DISP_WIND_START;

            g_sprite_layer_addr_aap[VIC_MEM_SPRITE_BACKGROUND] = g_sprite_layer_addr_start_aap[VIC_MEM_SPRITE_BACKGROUND] +
                                                             g_screen_line_cnt * PIXELS_MAX +
                                                             PIXEL_DISP_WIND_START;

            g_pixel_sprite_mapping_p = g_pixel_sprite_mapping_start_p + g_screen_line_cnt * PIXELS_MAX + PIXEL_DISP_WIND_START;

            go_again = 1; /* Might have more cycles to spend! */
            break;
          }
        }
      }
    }
    break;
    case VIC_STATE_VERTICAL_LOWER_BORDER:
    {
      if(g_ucycles_in_queue >= UCYCLES_LINE)
      {
#ifdef HAVE_BORDERS
        {
          uint32_t i;

          for(i = 0; i < FORGROUND_WIDTH; i++)
          {
            output_pixel_BORDER();
          }
        }

#ifdef SCREEN_X2
        g_layer_addr_p += FORGROUND_WIDTH*2;
#endif
#endif
        g_screen_line_cnt++;
        g_ucycles_in_queue -= UCYCLES_LINE;

        new_line();

        /* Last lower border line is 299 */
        if(g_screen_line_cnt == LINE_BLANK_START)
        {
          g_vic_state = VIC_STATE_VERTICAL_BLANKING;
        }
      }
    }
    break;
  }

  if(go_again && (g_ucycles_in_queue > 0))
  {
    goto GO_AGAIN;
  }
}

void vic_set_half_frame_rate()
{
    g_full_frame_rate = 0;
}
void vic_set_full_frame_rate()
{
    g_full_frame_rate = 1;
}

void vic_unlock_frame_rate()
{
    g_lock_frame_rate = 0;
}

void vic_lock_frame_rate()
{
    g_lock_frame_rate = 1;
}
