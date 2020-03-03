/* Copyright 2005-2020 Kendall F. Morris

This file is part of the USF Brainstem Data Visualization suite.

    The Brainstem Data Visualiation suite is free software: you can
    redistribute it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    The suite is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the suite.  If not, see <https://www.gnu.org/licenses/>.
*/

/* This is a fake .glsl file. It is really C++ code. But I put it into a .glsl
   file so my vim syntax highlighting editor would think it is (mostly) shader code
   and would highlight the text accordingly.
   I use the syntax code from:
      https://github.com/beyondmarc/glsl.vim

   To filter out the raw string text, add this to end of your glsl syntax file:

      syntax region ignoreRawStart start='R"' end='('
      syntax region ignoreRawEnd start=')"' end=';'

   A clever person could probably find a way to break this.

   There are also some keywords marked as unsupported, but some of these seem
   to be out of date. For just this file, add this:

   syn keyword glslStorageClass uimage2D uimageBuffer
*/

/* These are from the STEREO_MODE enum in brainstem.h
   stereo = 0  control only
   stereo = 1  control stereo 
   stereo = 2  ctl and stim pair 
   stereo = 3  ctl part of pair (draw on 1st pass)
   stereo = 4  stim part of pair (draw on 2nd pass)
   stereo = 5  stim only
   stereo = 6  stim stereo 
   stereo = 7  delta only
   stereo = 8  delta stereo
*/

// V shader for oulines and axes
const char* vertexVSrc =
R"(
#version 430
in vec3 vp;
void main() {
  gl_Position = vec4(vp,1.0);
}
)";

const char* vertexGSrc =
R"(
#version 430
#define CONTROL_ONLY     0
#define CONTROL_STEREO   1
#define CTL_STIM_PAIR    2
#define CTL_PART         3
#define STIM_PART        4
#define STIM_ONLY        5
#define STIM_STEREO      6
#define DELTA_ONLY       7
#define DELTA_STEREO     8
layout (lines) in;
layout (invocations = 2) in;
layout (line_strip, max_vertices=2) out;
layout (std140,binding=1) uniform vertexUbo{ mat4 mvp[2];};
layout (binding=4) uniform useStereo{int stereo;};
void main() {
   int i;
   bool is_stereo;
   if (stereo==CONTROL_STEREO || stereo==CTL_STIM_PAIR || 
       stereo==STIM_STEREO || stereo==DELTA_STEREO)
      is_stereo = true;
   else
     is_stereo=false;
        // draw nothing on 2nd invocation if in non-stereo mode
   if (is_stereo || ((!is_stereo) && gl_InvocationID == 0))
   {
      for (i = 0; i < gl_in.length(); i++)
      {
         gl_ViewportIndex = gl_InvocationID;
         gl_Position = mvp[gl_InvocationID] * gl_in[i].gl_Position;
         EmitVertex();
      }
      EndPrimitive();
   }
}
)";


// shader for outline colors
const char* outlineFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (early_fragment_tests) in;
layout (binding = 0,r32ui) uniform uimage2D head_pointer;
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (std140,binding=2) uniform nodesUbo{ uint max_nodes;};

uniform vec4 outlineColor;
out vec4 fcolor;
void main() {

   fcolor  = outlineColor;
   if (fcolor.a > 0)
   {
      uint index = atomicCounterIncrement(list_counter);
      if (index < max_nodes)
      {
         uint old_head = imageAtomicExchange(head_pointer, ivec2(gl_FragCoord.xy),index);
         nodes[index].color=fcolor;
         nodes[index].depth=gl_FragCoord.z;
         nodes[index].next=old_head;
      }
   }
}
)";

// shader for axes colors
const char* axesFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (early_fragment_tests) in;
layout (binding = 0,r32ui) uniform uimage2D head_pointer;
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (std140,binding=2) uniform nodesUbo{ uint max_nodes;};
uniform uint MaxNodes;
uniform vec4 axesColor; 
out vec4 fcolor;
void main() {
   fcolor = axesColor;
   if (fcolor.a > 0)
   {
      uint index = atomicCounterIncrement(list_counter);
      if (index < max_nodes)
      {
         uint old_head = imageAtomicExchange(head_pointer, ivec2(gl_FragCoord.xy),index);
         nodes[index].color=fcolor;
         nodes[index].depth=gl_FragCoord.z;
         nodes[index].next=old_head;
      }
   }
}
)";


// using instance drawing.  We draw a sphere, which has this called many times
// but the cell_pos and other vars advance to the next location once per sphere.
const char* cellVSrc =
R"(
#version 430
layout (location = 0) in vec3 vp;           // sphere vertices
layout (location = 1) in vec3 norm;         // sphere normals
layout (location = 2) in vec3 cell_pos0;    // control, once per instance for location
layout (location = 3) in vec3 cell_pos1;    // stim 
layout (location = 4) in vec3 cell_pos2;    // delta 
layout (location = 5) in int colorsel0;     // control
layout (location = 6) in int colorsel1;     // stim
layout (location = 7) in int colorsel2;     // delta
layout (location = 4) uniform int scale=90;
out flat int ctl_cidx;
out flat int stim_cidx;
out flat int delta_cidx;
out flat vec4 ctl_pos;
out flat vec4 stim_pos;
out flat vec4 delta_pos;
out vec3 colornorm;
void main() {
   ctl_pos    = vec4((vp/scale + cell_pos0),1.0);
   stim_pos   = vec4((vp/scale + cell_pos1),1.0);
   delta_pos  = vec4((vp/scale + cell_pos2),1.0);
   ctl_cidx   = colorsel0;
   stim_cidx  = colorsel1;
   delta_cidx = colorsel2;
   colornorm  = norm;
}
)";


// geometry shader for cells  
// input from vert, outputs to frag
const char* cellGSrc =
R"(
#version 430
#define CONTROL_ONLY     0
#define CONTROL_STEREO   1
#define CTL_STIM_PAIR    2
#define CTL_PART         3
#define STIM_PART        4
#define STIM_ONLY        5
#define STIM_STEREO      6
#define DELTA_ONLY       7
#define DELTA_STEREO     8
layout(triangles, invocations=2) in;
layout(triangle_strip,max_vertices=3) out;
layout (location=5) uniform bool hideOff = false;
layout (std140,binding=3) buffer deltaColorTable {vec4 dtab[];}; // buffer
layout (std140,binding=2) buffer colorTable {vec4 ctab[];};
layout (std140,binding=1) uniform vertexUbo {mat4 mvp[2];};
layout (binding = 3, std140) uniform normUbo {mat4 mv[2];}; //  uniform
layout (binding=4) uniform useStereo {int stereo;};
in flat int ctl_cidx[];
in flat int stim_cidx[];
in flat int delta_cidx[];
in vec3 colornorm[];
in flat vec4 ctl_pos[];
in flat vec4 stim_pos[];
in flat vec4 delta_pos[];
out vec4 c_color;
out vec3 c_norm;
void main() {
   int gcidx;
   bool drawpt = true;
   int i;
   bool is_stereo;
   bool ctlstim=false;
   bool second_list=false;
   if (stereo==CONTROL_STEREO || stereo==CTL_STIM_PAIR || 
       stereo==STIM_STEREO || stereo==DELTA_STEREO)
      is_stereo = true;
   else
      is_stereo=false;

      // draw ctl pts & colors in this mode
   if (stereo==CTL_PART || stereo==STIM_PART)
   {
      if (stereo==CTL_PART && gl_InvocationID==0) // ctl pts on 1st pass
      {
         for (i = 0; i < gl_in.length(); i++)
         {
            gl_Position = mvp[gl_InvocationID] * ctl_pos[i];
            gl_ViewportIndex = gl_InvocationID;
            c_norm = normalize(mat3(mv[gl_InvocationID])*colornorm[i]);
            gcidx = ctl_cidx[i];
            c_color = ctab[gcidx];
            if (hideOff == true && drawpt == true) // once false always false
            {
               if ((c_color[0] == 0.0) && (c_color[1] == 0.0) && (c_color[2] == 0.0))
                  drawpt = false;
            }
            if (drawpt == true)
            {
               EmitVertex();
            }
         }
         EndPrimitive();
      }
      else if (stereo==STIM_PART && gl_InvocationID==1) // stim pts/colors
      {
         for (i = 0; i < gl_in.length(); i++)
         {
            gl_Position = mvp[gl_InvocationID] * stim_pos[i];
            gl_ViewportIndex = gl_InvocationID;
            c_norm = normalize(mat3(mv[gl_InvocationID])*colornorm[i]);
            gcidx = stim_cidx[i];
            c_color = ctab[gcidx];
            if (hideOff == true && drawpt == true) // once false always false
            {
               if ((c_color[0] == 0.0) && (c_color[1] == 0.0) && (c_color[2] == 0.0))
                  drawpt = false;
            }
            if (drawpt == true)
            {
               EmitVertex();
            }
         }
         EndPrimitive();
      }
   }
   else if (is_stereo || ((!is_stereo) && gl_InvocationID == 0))
   {
      for (i = 0; i < gl_in.length(); i++)
      {
         if (stereo==CONTROL_ONLY || stereo == CONTROL_STEREO)
         {
            gl_Position = mvp[gl_InvocationID] * ctl_pos[i];
            gcidx = ctl_cidx[i];
            c_color = ctab[gcidx];
         }
         else if (stereo==STIM_ONLY || stereo==STIM_STEREO)
         {
            gl_Position = mvp[gl_InvocationID] * stim_pos[i];
            gcidx = stim_cidx[i];
            c_color = ctab[gcidx];
         }
         else if (stereo==DELTA_ONLY || stereo==DELTA_STEREO)
         {
            gl_Position = mvp[gl_InvocationID] * delta_pos[i];
            gcidx = delta_cidx[i];
            c_color = dtab[gcidx];
         }

         gl_ViewportIndex = gl_InvocationID;
         c_norm = normalize(mat3(mv[gl_InvocationID])*colornorm[i]);
         if (hideOff == true && drawpt == true) // once false always false
         {
            if ((c_color[0] == 0.0) && (c_color[1] == 0.0) && (c_color[2] == 0.0))
               drawpt = false;
         }
         if (drawpt == true)
         {
            EmitVertex();
         }
      }
      EndPrimitive();
   }
}
)";

// shader for cell colors
const char* cellFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (early_fragment_tests) in;
layout (binding = 0,r32ui) uniform uimage2D head_pointer;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (location = 11) uniform vec3 ambient = vec3(0.0,0.0,0.0);
layout (location = 12) uniform vec3 light_color = vec3(1.0,1.0,1.0);
layout (location = 13) uniform vec3 light_dir = vec3(0.0,0.0,1.0);
layout (location = 14) uniform float trans = 0.5;
layout (std140,binding=2) uniform nodesUbo{ uint max_nodes;};
in vec4 c_color;
in vec3 c_norm;
out vec4 fcolor;
void main() {
   vec3 facenorm;
   vec3 justcol=vec3(c_color);

   if (gl_FrontFacing)
      facenorm = c_norm;
   else
      facenorm = -c_norm;

   float diffuse = max(0.0,dot(facenorm,light_dir));
   vec3 scattered = ambient + light_color * diffuse;

// specular reflection, comment out the next line and then 
// uncomment the block after it to have shiny spot on cells
   vec3 rgb = min(justcol*scattered,vec3(1.0));

/*
// specular reflection code
vec3 view_dir=vec3(0.0,0.0,-1.0);
vec3 halfvect = normalize(light_dir+view_dir);
float power = 128.0;
float specular = max(dot(facenorm,halfvect),0.0);
if (diffuse == 0.0)
   specular = 0.0;
else
   specular = pow(specular,power);
vec3 reflected = light_color * specular;
vec3 rgb = min(justcol * scattered + reflected, vec3(1.0));
*/

   fcolor = vec4(rgb,trans);
   if (fcolor.a > 0)
   {
      uint index = atomicCounterIncrement(list_counter);
      if (index < max_nodes)
      {
         uint old_head = imageAtomicExchange(head_pointer, ivec2(gl_FragCoord.xy),index);
         nodes[index].color=fcolor;
         nodes[index].depth=gl_FragCoord.z;
         nodes[index].next=old_head;
      }
   }
}
)";

const char* printVSrc =
R"(
#version 430
layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 vt;
out vec2 tc;
void main() {
   tc = vt;
   gl_Position = vec4(vp,1.0);
}
)";

// binding below is the n value in GL_TEXTUREn define
const char* printFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (early_fragment_tests) in;
layout (binding = 0,r32ui) uniform uimage2D head_pointer;
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (binding=0) uniform sampler2D textstuff;
layout (std140,binding=2) uniform nodesUbo{ uint max_nodes;};
in vec2 tc;
out vec4 tcolor;
void main() {
   tcolor = texture(textstuff,tc);
   if (tcolor.a > 0)
   {
      uint index = atomicCounterIncrement(list_counter);
      if (index < max_nodes)
      {
         uint old_head = imageAtomicExchange(head_pointer, ivec2(gl_FragCoord.xy),index);
         nodes[index].color=tcolor;
         nodes[index].depth=gl_FragCoord.z;
         nodes[index].next=old_head;
      }
   }
}
)";


// skin shaders
const char* sort_skinVSrc =
R"(
#version 430
layout (location=0) in vec3 vp;
layout (location=1) in vec3 norm;
out vec3 colornorm;
void main() {
  gl_Position = vec4(vp,1.0);
  colornorm = norm;
}
)";

const char* sort_skinGSrc =
R"(
#version 430
#define CONTROL_ONLY     0
#define CONTROL_STEREO   1
#define CTL_STIM_PAIR    2
#define CTL_PART         3
#define STIM_PART        4
#define STIM_ONLY        5
#define STIM_STEREO      6
#define DELTA_ONLY       7
#define DELTA_STEREO     8
layout(triangles, invocations=2) in;
layout(triangle_strip,max_vertices=3) out;
layout (std140,binding=1) uniform vertexUbo{ mat4 mvp[2];};
layout (binding=3, std140) uniform normUbo {mat4 mv[2];};
layout (binding=4) uniform useStereo{ int stereo;};
in vec3 colornorm[];
out vec3 c_norm;
void main() {
   int i;
   bool is_stereo;
   if (stereo==CONTROL_STEREO || stereo==CTL_STIM_PAIR || 
       stereo==STIM_STEREO || stereo==DELTA_STEREO)
      is_stereo = true;
   else
      is_stereo=false;

      // draw nothing on 2nd invocation if in non-stereo mode
   if (is_stereo || ((!is_stereo) && gl_InvocationID == 0))
   {
      for (i = 0; i < gl_in.length(); i++)
      {
         c_norm = normalize(mat3(mv[gl_InvocationID])*colornorm[i]);
         gl_Position = mvp[gl_InvocationID] * gl_in[i].gl_Position;
         gl_ViewportIndex = gl_InvocationID;
         EmitVertex();
      }
      EndPrimitive();
   }
}
)";


const char* sort_skinFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (early_fragment_tests) in;
layout (binding = 0,r32ui) uniform uimage2D head_pointer;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (location = 1) uniform float trans = 0.7;
layout (location = 2) uniform vec3 ambient = vec3(0.0,0.0,0.0);
layout (location = 3) uniform vec3 light_color = vec3(1.0,1.0,1.0);
layout (location = 4) uniform vec3 light_dir = vec3(0.0,0.0,0.5);
layout (location = 5) uniform int surfaceSel = 0;
layout (location = 6) uniform vec3 skinColorT = vec3(0.82,0.71,0.55);
layout (location = 7) uniform vec3 skinColorW = vec3(0.81,0.81,0.81);
layout (std140,binding=2) uniform nodesUbo{ uint max_nodes;};
in vec3 c_norm;
out vec4 fcolor;
void main() {
   vec3 facenorm;
   vec3 currcolor;
   uint index;
   uint old_head;
   uvec4 item;

   if (surfaceSel == 0)
      currcolor = skinColorT;
   else
      currcolor = skinColorW;

   if (gl_FrontFacing)
     facenorm = c_norm;
   else
     facenorm = -c_norm;

   float diffuse = max(0.0,dot(facenorm,light_dir));
   vec3 scattered = ambient + light_color * diffuse;
   vec3 rgb = min(currcolor * scattered,vec3(1.0));
   fcolor = vec4(rgb,trans);
   if (fcolor.a > 0)
   {
      index = atomicCounterIncrement(list_counter);
      if (index < max_nodes)
      {
      old_head = imageAtomicExchange(head_pointer, ivec2(gl_FragCoord.xy),index);
      nodes[index].color=fcolor;
      nodes[index].depth=gl_FragCoord.z;
      nodes[index].next=old_head;
      }
   }
}
)";

// brain structure shaders
const char* structVSrc =
R"(
#version 430
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 norm;
out vec3 colornorm;
void main() {
   gl_Position = vec4(vp,1.0);
   colornorm = norm;
}
)";

const char* structGSrc =
R"(
#version 430
#define CONTROL_ONLY     0
#define CONTROL_STEREO   1
#define CTL_STIM_PAIR    2
#define CTL_PART         3
#define STIM_PART        4
#define STIM_ONLY        5
#define STIM_STEREO      6
#define DELTA_ONLY       7
#define DELTA_STEREO     8
layout(triangles, invocations=2) in;
layout(triangle_strip,max_vertices=3) out;
layout (std140,binding=1) uniform vertexUbo { mat4 mvp[2];};
layout (binding=3, std140) uniform normUbo {mat4 mv[2];};
layout (binding=4) uniform useStereo { int stereo;};
in vec3 colornorm[];
out vec3 c_norm;
void main() {
   int i;
   bool is_stereo;
   if (stereo==CONTROL_STEREO || stereo==CTL_STIM_PAIR || 
       stereo==STIM_STEREO || stereo==DELTA_STEREO)
      is_stereo = true;
   else
      is_stereo=false;

      // draw nothing on 2nd invocation if in non-stereo mode
   if (is_stereo || ((!is_stereo) && gl_InvocationID == 0))
   {
      for (i = 0; i < gl_in.length(); i++)
      {
         c_norm = normalize(mat3(mv[gl_InvocationID])*colornorm[i]);
         gl_Position = mvp[gl_InvocationID] * gl_in[i].gl_Position;
         gl_ViewportIndex = gl_InvocationID;
         EmitVertex();
      }
      EndPrimitive();
   }
}
)";

const char* structFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (early_fragment_tests) in;
layout (binding = 0,r32ui) uniform uimage2D head_pointer;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (location = 0) uniform vec3 structColor = vec3(1.0,1.0,1.0);
layout (location = 1) uniform float trans = 1.0;
layout (location = 2) uniform vec3 ambient = vec3(0.0,0.0,0.0);
layout (location = 3) uniform vec3 light_color = vec3(1.0,1.0,1.0);
layout (location = 4) uniform vec3 light_dir = vec3(0.0,0.0,0.5);
layout (std140,binding=2) uniform nodesUbo{ uint max_nodes;};
in vec3 c_norm;
out vec4 fcolor;
void main() {
   vec3 facenorm;
   uint index;
   uint old_head;
   uvec4 item;

  if (gl_FrontFacing)
     facenorm = c_norm;
  else
     facenorm = -c_norm;

   float diffuse = max(0.0,dot(facenorm,light_dir));
   vec3 scattered = ambient + light_color * diffuse;
   vec3 rgb = min(structColor*scattered,vec3(1.0));
   fcolor = vec4(rgb,trans);

   if (fcolor.a > 0)
   {
      index = atomicCounterIncrement(list_counter);
      if (index < max_nodes)
      {
         old_head = imageAtomicExchange(head_pointer, ivec2(gl_FragCoord.xy),index);
         nodes[index].color=fcolor;
         nodes[index].depth=gl_FragCoord.z;
         nodes[index].next=old_head;
      }
   }
}
)";



// all of the above created a list of sorted vertices.
// Now render the result to the frame buffer
const char* finalRenderVSrc =
R"(
#version 430
in vec4 vp;
void main() {
  gl_Position = vp;
}
)";


const char* finalRenderGSrc =
R"(
#version 430
#define CONTROL_ONLY     0
#define CONTROL_STEREO   1
#define CTL_STIM_PAIR    2
#define CTL_PART         3
#define STIM_PART        4
#define STIM_ONLY        5
#define STIM_STEREO      6
#define DELTA_ONLY       7
#define DELTA_STEREO     8
layout(triangles, invocations=2) in;
layout(triangle_strip,max_vertices=3) out;
layout (binding=4) uniform useStereo{ int stereo;};
void main() {
   int i;
   bool is_stereo;
   if (stereo==CONTROL_STEREO || stereo==CTL_STIM_PAIR || 
       stereo==STIM_STEREO || stereo==DELTA_STEREO)
      is_stereo = true;
   else
      is_stereo=false;

      // draw nothing on 2nd invocation if in non-stereo mode
   if (is_stereo || ((!is_stereo) && gl_InvocationID == 0))
   {
      for (i = 0; i < gl_in.length(); i++)
      {
         gl_Position = gl_in[i].gl_Position;
         gl_ViewportIndex = gl_InvocationID;
         EmitVertex();
      }
      EndPrimitive();
   }
}
)";

// #if 0
// Sorting the frags is a bottleneck, alternative sorts were tried.
// merge sort method -- THIS WAS FASTEST
// The first render pass created an unordered list of fragments for each screen
// pix. Now we sort the list and use it to render all objects back to front.
const char* finalRenderFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (binding = 0, r32ui) uniform uimage2D head_pointer;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
//layout (binding = 9) buffer  NC { uint node_counter; };
layout (location = 0) out vec4 color;
layout (location = 1) uniform vec4 backColor;
#define MAX_FRAGMENTS 400      // Max number of overlapping fragments per (x,y)
void main(void)
{
   OITNode sort_list[MAX_FRAGMENTS]; // Temporary array used for sorting fragments
   OITNode buff[MAX_FRAGMENTS/2];
   OITNode next_ode;
   uint index;
   uint count = 0;
   vec4 final_color = vec4(backColor.xyz,0); // trasparent background by default
   uvec4 pix;
   vec4 next_color;
   uint i, j1, j2, k;
   uint a, b, c;
   uint s_step = 1;

     // The per-pixel image containing the head pointers for each (x,y)
   pix = imageLoad(head_pointer, ivec2(gl_FragCoord).xy);
   index = pix.x;

    // walk the list for this (x,y) and create local array of all
    // frags stacked up on this (x,y)
   while (index != 0xFFFFFFFF && count < MAX_FRAGMENTS)
   {
      sort_list[count] = nodes[index];
      index = sort_list[count].next;
      count++;
   }
//   atomicMax(node_counter,count); // xpments show about 240 is max

   while (s_step <= count)
   {
      i = 0;
      while (i < count - s_step)
      {
         a = i;
         b = i + s_step;
         c = (i + s_step + s_step) >= count ? count : (i + s_step + s_step);
         for (k = 0; k < s_step; k++)
            buff[k] = sort_list[a + k];
         j1 = 0;
         j2 = 0;
         for (k = a; k < c; k++)
         {
           if (b + j1 >= c || (j2 < s_step && buff[j2].depth > sort_list[b + j1].depth))
              sort_list[k] = buff[j2++];
           else
              sort_list[k] = sort_list[b + j1++];
         }
         i += 2 * s_step;
      }
      s_step *= 2;
   }
   float r0 = 0;
   float g0 = 0;
   float b0 = 0;
       // figure out final color
   for (i = 0; i < count; i++)      // work sorted list, front to back
   {
      next_color = sort_list[i].color;
      final_color = mix(final_color,next_color,next_color.a);
       // if any pix is at same z, all bets are off
       // later...it turns out there sometimes a lot of hits, more
       // than 20 or 30...so....?
       if (i > 0 && sort_list[i-1] == sort_list[i])
       {
          if (r0 < 1.0)
             r0 = r0+0.05;
          else if (g0 < 1.0)
             g0 = g0+0.05;
//          else if (b0 < 1.0)
//             b0 = b0+0.05;
       }
   }
    // debugging -- array limis
//   if ( i == 0)
//      final_color=vec4(1,0,0,1);
//   else if (i == 1)
//      final_color=vec4(0,1,0,1);
//   else if (count >= MAX_FRAGMENTS)
//      final_color=vec4(1,1,1,1);
   if (count >= MAX_FRAGMENTS)
      final_color=vec4(1,0,1,1);
  if (r0 != 0)
     final_color = vec4(r0,g0,b0,1);
  color = final_color;
//   if (count > MAX_FRAGMENTS-1) // test so see if we ran out of frag buffer
//      color = vec4(1,1,1,1);             // force to solid black
                            // debug -- how many frags?
// color = min(vec4(1), vec4(1.0/float(count)));
}
)";
//#endif

#if 0
//////////////////////////
// Sorting the frags is a bottleneck, alternative sorts were tried.
// insert sort method -- Better then bubble and quicksort
// The first render pass created an unordered list of fragments for each screen
// pix. Now we sort the list and use it to render the transparent objects,
// back to front.
const char* finalRenderFSrc =
R"(
#version 430
struct OITNode {
   vec4 color;
   float depth;
   uint next;
   };
layout (binding = 0, r32ui) uniform uimage2D head_pointer;
layout (binding = 0,std430) buffer list_buffer { OITNode nodes[]; };
layout (location = 0) out vec4 color;
layout (location = 1) uniform vec4 backColor;
#define MAX_FRAGMENTS 400      // Max number of overlapping fragments per (x,y)
OITNode sort_list[MAX_FRAGMENTS]; // Temporary array used for sorting fragments

void main(void)
{
   uint index;
   uint count = 0;
   uvec4 pix, frag1, frag2;
   OITNode frag;
   float depth1, depth2;
   vec4 next_color;
   uint i, j;

     // The per-pixel image containing the head pointers for each (x,y)
   pix = imageLoad(head_pointer, ivec2(gl_FragCoord).xy);
   index = pix.x;

      // walk the list for this (x,y) and create local array of all
      // frags stacked up on this (x,y)
   while (index != 0xFFFFFFFF && count < MAX_FRAGMENTS)
   {
      sort_list[count] = nodes[index];
      index = sort_list[count].next;
      count++;
   }
     // sort
   for (i = 1; i < count; ++i)
   {
      frag = sort_list[i];
      j = i;
      while (j>0 && frag.depth > sort_list[j-1].depth)
      {
         sort_list[j] = sort_list[j-1];
         --j;
      }
      sort_list[j] = frag;
   }

    // figure out final color
   vec4 final_color = vec4(backColor.xyz,0); // trasparent background by default
   for (i = 0; i < count; i++)      // work sorted list, front to back
   {
      next_color = sort_list[i].color;
      final_color = mix(final_color,next_color,next_color.a);
   }
  color = final_color;
}
)";
#endif

// sorting the frags is a bottleneck, alternative sorts were tried.
#if 0 
// bubble sort method, kind of slow
const char* finalRenderFSrc =
R"(
#version 430 core
layout (binding = 0, r32ui) uniform uimage2D head_pointer;
layout (binding = 1, rgba32ui) uniform uimageBuffer list_buffer;
layout (location = 0) out vec4 color;
layout (location = 1) uniform vec4 backColor;
#define MAX_FRAGMENTS 200      // Max number of overlapping fragments per (x,y)
uvec4 sort_list[MAX_FRAGMENTS]; // Temporary array used for sorting fragments

void main(void)
{
   uint index;
   uint fragment_count = 0;
   uvec4 pix, frag1, frag2;
   float depth1, depth2;
   vec4 next_color;
   uint i, j;

     // The per-pixel image containing the head pointers for each (x,y)
   pix = imageLoad(head_pointer, ivec2(gl_FragCoord).xy);
   index = pix.x;

      // walk the list for this (x,y) and create local array of all
      // frags stacked up on this (x,y)
   while (index != 0xFFFFFFFF && fragment_count < MAX_FRAGMENTS)
   {
      uvec4 fragment = imageLoad(list_buffer, int(index));
      sort_list[fragment_count] = fragment;
      index = fragment.x;
      fragment_count++;
   }

   if (fragment_count > 1)   // sort list 
   {
      for (i = 0; i < fragment_count - 1; i++)
      {
         for (j = i + 1; j < fragment_count; j++)
         {
            frag1 = sort_list[i];
            frag2 = sort_list[j];
            depth1 = uintBitsToFloat(frag1.z);
            depth2 = uintBitsToFloat(frag2.z);
            if (depth1 < depth2)
            {
               sort_list[i] = frag2;
               sort_list[j] = frag1;
            }
         }
      }
   }
   vec4 final_color = vec4(backColor.xyz,0);
   for (i = 0; i < fragment_count; i++)
   {
      next_color = unpackUnorm4x8(sort_list[i].y);
      final_color = mix(final_color,next_color,next_color.a);
   }
   color = final_color;
//   if (fragment_count > MAX_FRAGMENTS-1)
//      color = vec4(1,1,1,1);
                            // debug -- how many frags?
// color = min(vec4(1), vec4(1.0/float(fragment_count)));

}
)";
#endif

#if 0
// Non-recursive quick sort. THIS WAS ALMOST TWICE AS SLOW AS INSERT SORT.
const char* finalRenderFSrc =
R"(
#version 430 core
layout (binding = 0, r32ui) uniform uimage2D head_pointer;
layout (binding = 1, rgba32ui) uniform uimageBuffer list_buffer;
layout (location = 0) out vec4 color;
layout (location = 1) uniform vec4 backColor;
#define MAX_FRAGMENTS 200      // Max number of overlapping fragments per (x,y)
uvec4 sort_list[MAX_FRAGMENTS];
int beg[MAX_FRAGMENTS+1];
int end[MAX_FRAGMENTS+1];

void main(void)
{
   uint index;
   int fragment_count = 0;
   uvec4 pix;
   vec4 next_color;
   int i, L, R;
   uvec4 piv;
   i = 0;
     // The per-pixel image containing the head pointers for each (x,y)
   pix = imageLoad(head_pointer, ivec2(gl_FragCoord).xy);
   index = pix.x;

      // walk the list for this (x,y) and create local array of all
      // frags stacked up on this (x,y)
   while (index != 0xFFFFFFFF && fragment_count < MAX_FRAGMENTS)
   {
      uvec4 fragment = imageLoad(list_buffer, int(index));
      sort_list[fragment_count] = fragment;
      index = fragment.x;
      fragment_count++;
   }
 
   if (fragment_count > 1)   // non-recursive quicksort
   {
      beg[0] = 0;
      end[0] = fragment_count;
      while (i >= 0)
      {
         L = beg[i];
         R = end[i] - 1;
         if (L < R)
         {
            piv = sort_list[L];
            if (i == MAX_FRAGMENTS-1)
               break;
            while (L < R)
            {
               while (uintBitsToFloat(sort_list[R].z) >= uintBitsToFloat(piv.z) && L < R)
                 R--;
               if (L < R)
                  sort_list[L++] = sort_list[R];
               while (uintBitsToFloat(sort_list[L].z) <= uintBitsToFloat(piv.z) && L < R)
                  L++;
               if (L < R)
                  sort_list[R--] = sort_list[L];
            }
            sort_list[L] = piv;
            beg[i+1]=L+1;
            end[i+1]=end[i];
            end[i]=L;
            ++i;
         }
         else
         {
            i--;
         }
       }
    }
    vec4 final_color = vec4(backColor.xyz,0);
    for (i = 0; i < fragment_count; i++)
    {
      next_color = unpackUnorm4x8(sort_list[i].y);
      final_color = mix(final_color,next_color,next_color.a);
   }
   color = final_color;
                            // debug -- how many frags?
// color = min(vec4(1), vec4(1.0/float(fragment_count)));

}
)";
#endif

