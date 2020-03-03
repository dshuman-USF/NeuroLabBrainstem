#include <X11/Xlib.h>
#include <GL/glx.h>
#include <FTGL/ftgl.h>
#include "util.h"

typedef struct {
 float refx,refy,refz;
 float camx,camy,camz;
 float upx,upy,upz;
 float field_of_view;
 float front,back;
 float projection;
} camera_arg;

typedef struct
{
  Window win;
  float p1x;
  float p1y;
  float p1z;
  float p2x;
  float p2y;
  float p2z;

  float xmin;
  float ymin;
  float zmin;
  float xmax;
  float ymax;
  float zmax;

  float px1;
  float py1;
  float px2;
  float py2;

  float vx1;
  float vy1;
  float vz1;
  float vx2;
  float vy2;  
  float vz2;

  int mapping_distort;
  float left;
  float bottom;
  int width;
  int height;
  float S[3];
  float T[3];

  float char_height;
  int tax, tay;
  float text_r, text_g, text_b;
  float line_r, line_g, line_b;
  float fill_r, fill_g, fill_b;
  int font_index;
  XYZ up, base;
  char *string;
  int stringlen;
  FTGLfont *font[7];
  int resolution;
  float pen_x, pen_y, pen_z;
  int clear_mode;
  int interior_style_val, edged_val;
  float peri_r, peri_g, peri_b;
  float repeat_length;
  int line_type;
  float character_expansion_factor;
  float character_width;
  int character_width_valid;
  float text_x, text_y, text_z;
  GLXContext expose_context;
} WinData;

extern float wc_to_vdc_matrix[16];
