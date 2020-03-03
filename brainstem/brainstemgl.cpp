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

/* This is a widget wrapped around openGL.

   Most of the visual stuff that this app does happens here.  However, it has
   noaccess to the GUI elements, so it can generate events that the parent can
   receive to perform  GUI functions.
*/


#include "brainstem.h"
#include <QSurfaceFormat>
#include <QtOpenGL>
#include <QOpenGLExtraFunctions>
#include "brainstemgl.glsl"


extern bool Debug;
extern "C" {
extern GLfloat (*plate[20])[3];
extern int point_count[20];
extern int plate_count;
extern int objSizes[][2];
extern GLfloat (*allStructs [])[3];
extern GLfloat (*objNorms [])[3];
extern const int numStructs;
extern const int numNormStructs;
extern GLfloat brainSkin[][3];
extern GLfloat skinNorms[][3];
extern GLfloat sphereVert[][3];
extern GLfloat sphereNorm[][3];
extern const int numbrainSkin, numskinNorms;
extern const int numsphereVert, numsphereNorm;
}

const int settingsMarker=0xFEEE;
const int figSettingsVer=0x0001;
const GLuint RESTART_MARKER=0xFFFF;

const float XLEN = 12.0;
const float YLEN = 8.0;
const float ZLENP = 8.0;
const float ZLENN = 15.0;
const float MAJOR_TICK = 1.0;
const float MINOR_TICK = 0.5;
const float MAJOR_TICK_LEN = 0.4;
const float MINOR_TICK_LEN = 0.2;
const int COLOR_STEPS = 16;      // These MUST be > 2
const int D_COLOR_STEPS = 8;

const int DRAWBOX_W = 340;
const int DRAWBOX_H = 30;
const int PHRENIC_W = 150;
const int PHRENIC_H = 30;
const int PHRENIC_X = DRAWBOX_W-PHRENIC_W;
const int PHRENIC_Y = 0;
const int PHRENIC_I_START = PHRENIC_X+10;   // offsets from png image in GIMP
const int PHRENIC_E_START = PHRENIC_X+77;
const int PHRENIC_E_END   = PHRENIC_X+139;

// rect we use to get pixel (x,y) values while really rendering sorted vertices
static const GLfloat showVerts[] =
{
   -1.0f, -1.0f,
   1.0f, -1.0f,
   -1.0f,  1.0f,
   1.0f,  1.0f
};

// box at top for text, other UI stuff
const GLfloat box_aspect = 1.0-(1.0/((GLfloat)DRAWBOX_W/DRAWBOX_H));
GLfloat drawBox[] =
{
                      // vvv determines in front of / back of
 -0.5f,  box_aspect,    -0.9,
  0.5f,  box_aspect,    -0.9,
  0.5f,  1.0f,          -0.9,
  0.5f,  1.0f,          -0.9,
 -0.5f,  1.0f,          -0.9,
 -0.5f,  box_aspect,    -0.9
};


GLfloat drawTextureBox[] =
{
   0.0f, 0.0f,
   1.0f, 0.0f,
   1.0f, 1.0f,
    1.0f, 1.0f, 
   0.0f, 1.0f,
   0.0f, 0.0f
};


BrainStemGL::BrainStemGL(QWidget *parent) : QOpenGLWidget(parent)
{
    // limit these for multiple or 4K monitors
   setMaximumWidth(MAX_FB_WIDTH);
   setMaximumHeight(MAX_FB_HEIGHT);
}

BrainStemGL::~BrainStemGL()
{
   makeCurrent();
   clearClusts();
   glDeleteProgram(outlineProg);
   glDeleteProgram(axesProg);
   glDeleteProgram(sort_skinProg);
   if (cellProg)
      glDeleteProgram(cellProg);
}

QByteArray BrainStemGL::saveSettings()
{
   QByteArray settings;
   QDataStream stream(&settings,QIODevice::WriteOnly);

   stream << settingsMarker;
   stream << view_rotx;
   stream << view_roty;
   stream << view_rotz;
   stream << view_trx;
   stream << view_try;
   stream << view_trz;
   stream << angle;
   stream << radius;
   stream << backColor;
   stream << outlineColorVal[0];
   stream << showOutlines;
   stream << showAxes;
   return settings;
}

void BrainStemGL::loadSettings(QByteArray settings)
{
   userPrefs = settings;
}

void BrainStemGL::applyPrefs()
{
   QDataStream stream(&userPrefs,QIODevice::ReadOnly);
   int marker;

   stream >> marker;
   if (marker != settingsMarker)
      return;
   stream >> view_rotx;
   stream >> view_roty;
   stream >> view_rotz;
   stream >> view_trx;
   stream >> view_try;
   stream >> view_trz;
   stream >> angle;
   stream >> radius;
   stream >> backColor;
   stream >> outlineColorVal[0];
   stream >> showOutlines;
   stream >> showAxes;
   outlineColorVal[1] = outlineColorVal[2] = outlineColorVal[0];
   outlineColorVal[3] = 1.0;
}


// One-time init, called very early, before paint & resize
void BrainStemGL::initializeGL()
{
   QSurfaceFormat surfFormat;

   initializeOpenGLFunctions();
   applyPrefs();
   glClearColor(backColor,backColor,backColor,1.0);
   glm::vec3 dist(dX,dY,dZ);
   glm::vec3 amb(ambient/100.0,ambient/100.0,ambient/100.0);
   glm::vec3 dif(diffuse/100.0,diffuse/100.0,diffuse/100.0);

    // display interesting info
   QString msg;
   QTextStream(&msg) << "RENDERER: " << (char *)glGetString(GL_RENDERER) << endl;
   QTextStream(&msg) << "VERSION: " << (char *)glGetString(GL_VERSION) << endl;
   QTextStream(&msg) << "VENDOR: " << (char *)glGetString(GL_VENDOR) << endl;
   QTextStream(&msg) << "SHADING_LANGUAGE_VER: " << (char *)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
  surfFormat = format();
  QTextStream(&msg) << "OPENGL VERSION: " << surfFormat.majorVersion() << "." << surfFormat.minorVersion() << endl;

   if (!context()->hasExtension(QByteArrayLiteral("GL_KHR_debug")))
   {
      QTextStream(&msg) << "Debugging not supported" << endl;
      Debug = false;
   }

   if (Debug)
   {
       QTextStream(&msg) << "Profile:  ";
      if (surfFormat.profile() == QSurfaceFormat::CoreProfile ||
          surfFormat.profile() == QSurfaceFormat::NoProfile)
          QTextStream(&msg) << "Core" << endl;
      else if (surfFormat.profile() == QSurfaceFormat::CompatibilityProfile)
          QTextStream(&msg) << "Compatibility" << endl;
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(this);
      if (logger->initialize())
      {
         connect(logger, &QOpenGLDebugLogger::messageLogged, this, BrainStemGL::glDebugMsgs);
         logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
      }
      GLint intval = 0;
      glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &intval);
      QTextStream(&msg) << "Max supported vertex attribs: " << intval << endl;
      glGetIntegerv (GL_MAX_PATCH_VERTICES, &intval);
      QTextStream(&msg) << "Max supported patch vertices: " << intval << endl;
      glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &intval);
      QTextStream(&msg)<< "GL_MAX_UNIFORM_BUFFER_BINDINGS: " << intval << endl;
      glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &intval);
      QTextStream(&msg)<< "GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS: " << intval << endl;
      glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &intval);
      QTextStream(&msg)<< "GL_MAX_MAX_VERTEX_UNIFORM_COMPONENTS: " << intval << endl; 
      glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &intval);
      QTextStream(&msg)<< "GL_MAX_MAX_FRAGMENT_UNIFORM_COMPONENTS: " << intval << endl; 
      glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &intval);
      QTextStream(&msg)<< "MAX_COMBINED_GL_TEXTURE_UNITS: " << intval << endl; 
    }

   emit(chatBox(msg));

   phrenicLinePos = PHRENIC_I_START;
   glDisable(GL_CULL_FACE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
   glEnable(GL_PROGRAM_POINT_SIZE);
   glPrimitiveRestartIndex(RESTART_MARKER);

   phrenicImageG.load(":/phrenic_scaled_norm_transp_green.png"); // pngs in program image
   noPhrenicImage.load(":/nophrenic_transp.png");

   vertVShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertVShader,1,&vertexVSrc,nullptr);
   glCompileShader(vertVShader);
   chkcomp(vertVShader,"vertVShader");

   vertGShader = glCreateShader(GL_GEOMETRY_SHADER);
   glShaderSource(vertGShader,1,&vertexGSrc,nullptr);
   glCompileShader(vertGShader);
   chkcomp(vertGShader,"vertGShader");

   outlineFShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(outlineFShader,1,&outlineFSrc,nullptr);
   glCompileShader(outlineFShader);
   chkcomp(outlineFShader,"outlineFShader");

   axesFShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(axesFShader,1,&axesFSrc,nullptr);
   glCompileShader(axesFShader);
   chkcomp(axesFShader,"axesFShader");

      // create & link shader programs
   outlineProg = glCreateProgram();
   glAttachShader(outlineProg,vertVShader);
   glAttachShader(outlineProg,vertGShader);
   glAttachShader(outlineProg,outlineFShader);
   glLinkProgram(outlineProg);
   chklink(outlineProg,"outlineProg");

   axesProg = glCreateProgram();
   glAttachShader(axesProg,vertVShader);
   glAttachShader(axesProg,vertGShader); // shared
   glAttachShader(axesProg,axesFShader);
   glLinkProgram(axesProg);
   chklink(axesProg,"axesProg");

     // build a uniform buffer object (UBO) to share vertex shader transform matrix
   mvpSize = sizeof(mvpMat);
   glGenBuffers(1,&vUboBuff);
   glBindBuffer(GL_UNIFORM_BUFFER,vUboBuff);
   glBufferData(GL_UNIFORM_BUFFER, mvpSize, nullptr,GL_DYNAMIC_DRAW); // reserve space

      // build ubo for shared lighting normal transform matrix
   mvSize = sizeof(mvMat);
   glGenBuffers(1,&nUboBuff);
   glBindBuffer(GL_UNIFORM_BUFFER,nUboBuff);
   glBufferData(GL_UNIFORM_BUFFER, mvSize, nullptr,GL_DYNAMIC_DRAW); // reserve space

   glGenBuffers(1,&sUboBuff);
   glBindBuffer(GL_UNIFORM_BUFFER,sUboBuff);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(stereoMode), nullptr,GL_DYNAMIC_DRAW);
   void* stereo_ptr = glMapBufferRange(GL_UNIFORM_BUFFER,0,sizeof(stereoMode),
                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
   memcpy(stereo_ptr,&stereoMode,sizeof(stereoMode));
   glUnmapBuffer(GL_UNIFORM_BUFFER);

     // init shader vars
   glUseProgram(outlineProg);
   outlineColor = glGetUniformLocation(outlineProg,"outlineColor");
   glUniform4fv(outlineColor,1,glm::value_ptr(outlineColorVal));

   glUseProgram(axesProg);
   axesColor = glGetUniformLocation(axesProg,"axesColor");
   glUniform4fv(axesColor,1,glm::value_ptr(axesColorVec));

     // rectangle we print on.  This lives in fixed 
     // world space coords, so no transforms
   printVShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(printVShader,1,&printVSrc,nullptr);
   glCompileShader(printVShader);
   chkcomp(printVShader,"printShader");
   printFShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(printFShader,1,&printFSrc,nullptr);
   glCompileShader(printFShader);
   chkcomp(printFShader,"printFShader");
   printProg = glCreateProgram();
   glAttachShader(printProg,printVShader);
   glAttachShader(printProg,printFShader);
   glLinkProgram(printProg);
   chklink(printProg,"printProg");

     // cell spheres
   cellVShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(cellVShader,1,&cellVSrc,nullptr);
   glCompileShader(cellVShader);
   chkcomp(cellVShader,"cellVShader");

   cellGShader = glCreateShader(GL_GEOMETRY_SHADER);
   glShaderSource(cellGShader,1,&cellGSrc,nullptr);
   glCompileShader(cellGShader);
   chkcomp(cellGShader,"cellGShader");

   cellFShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(cellFShader,1,&cellFSrc,nullptr);
   glCompileShader(cellFShader);
   chkcomp(cellFShader,"cellFShader");

   cellProg = glCreateProgram();
   glAttachShader(cellProg,cellVShader);
   glAttachShader(cellProg,cellGShader);
   glAttachShader(cellProg,cellFShader);
   glLinkProgram(cellProg);
   chklink(cellProg,"cellProg");
   glUseProgram(cellProg);
   glUniform1i(5,hideCells);
   glUniform1i(4,ptSize);
   glUniform3fv(11,1,glm::value_ptr(amb));
   glUniform3fv(12,1,glm::value_ptr(dif));
   glUniform3fv(13,1,glm::value_ptr(dist));
   glUniform1f(14,cellTrans);
   glGenBuffers(1,&colorTabVbo);
   glGenBuffers(1,&deltaTabVbo);

    // skin on outlines
   sortVs = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(sortVs,1,&sort_skinVSrc,nullptr);
   glCompileShader(sortVs);
   chkcomp(sortVs,"sortVs");
   sortGs = glCreateShader(GL_GEOMETRY_SHADER);
   glShaderSource(sortGs,1,&sort_skinGSrc,nullptr);
   glCompileShader(sortGs);
   chkcomp(sortGs,"sortGs");
   sortFs = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(sortFs,1,&sort_skinFSrc,nullptr);
   glCompileShader(sortFs);
   chkcomp(sortFs,"sortFs");

   sort_skinProg = glCreateProgram();
   glAttachShader(sort_skinProg,sortVs);
   glAttachShader(sort_skinProg,sortGs);
   glAttachShader(sort_skinProg,sortFs);
   glLinkProgram(sort_skinProg);
   chklink(sort_skinProg,"sort_skinProg");
   glUseProgram(sort_skinProg);
   glUniform1f(1,skinTrans);
   glUniform3fv(2,1,glm::value_ptr(amb));
   glUniform3fv(3,1,glm::value_ptr(dif));
   glUniform3fv(4,1,glm::value_ptr(dist));

      // shaders for second pass OIT rendering
   finalRenderVs = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(finalRenderVs,1,&finalRenderVSrc,nullptr);
   glCompileShader(finalRenderVs);
   chkcomp(finalRenderVs,"finalRenderVs");
   finalRenderGs = glCreateShader(GL_GEOMETRY_SHADER);
   glShaderSource(finalRenderGs,1,&finalRenderGSrc,nullptr);
   glCompileShader(finalRenderGs);
   chkcomp(finalRenderGs,"finalRenderGs");
   finalRenderFs = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(finalRenderFs,1,&finalRenderFSrc,nullptr);
   glCompileShader(finalRenderFs);
   chkcomp(finalRenderFs,"finalRenderFs");

   finalRenderProg = glCreateProgram();
   glAttachShader(finalRenderProg,finalRenderVs);
   glAttachShader(finalRenderProg,finalRenderGs);
   glAttachShader(finalRenderProg,finalRenderFs);
   glLinkProgram(finalRenderProg);
   chklink(finalRenderProg,"finalRenderProg");
   glUseProgram(finalRenderProg);
   glUniform4f(1,backColor,backColor,backColor,1.0);
   glUseProgram(0);

    // brain structures 
   structVShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(structVShader,1,&structVSrc,nullptr);
   glCompileShader(structVShader);
   chkcomp(structVShader,"structVShader");
   structGShader = glCreateShader(GL_GEOMETRY_SHADER);
   glShaderSource(structGShader,1,&structGSrc,nullptr);
   glCompileShader(structGShader);
   chkcomp(structGShader,"structGShader");
   structFShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(structFShader,1,&structFSrc,nullptr);
   glCompileShader(structFShader);
   chkcomp(structFShader,"structFShader");
   structProg = glCreateProgram();
   glAttachShader(structProg,structVShader);
   glAttachShader(structProg,structGShader);
   glAttachShader(structProg,structFShader);
   glLinkProgram(structProg);
   chklink(structProg,"structProg");
   glUseProgram(structProg);
   glUniform1f(1,regionTrans);
   glUniform3fv(2,1,glm::value_ptr(amb));
   glUniform3fv(3,1,glm::value_ptr(dif));
   glUniform3fv(4,1,glm::value_ptr(dist));

   extremes();
   outlines();
   axes();
   printBox();
   skin();
   sphere();
   stemStructs();
   oit();

   glDeleteShader(vertVShader);
   glDeleteShader(outlineFShader);
   glDeleteShader(axesFShader);
   glDeleteShader(printVShader);
   glDeleteShader(printFShader);
   glDeleteShader(cellVShader);
   glDeleteShader(cellGShader);
   glDeleteShader(cellFShader);
   glDeleteShader(sortVs);
   glDeleteShader(sortFs);
   glDeleteShader(sortGs);
   glDeleteShader(structVShader);
   glDeleteShader(structFShader);
   glDeleteShader(finalRenderVs);
   glDeleteShader(finalRenderGs);
   glDeleteShader(finalRenderFs);
}

// callback from driver with sometimes useful info
void BrainStemGL::glDebugMsgs(const QOpenGLDebugMessage &msg)
{
   QString info;
   QTextStream(&info) <<  msg.message() << endl;
   cout << qPrintable(info) << endl;
}

void BrainStemGL::mousePressEvent(QMouseEvent *event)
{
   lastx = event->x();
   lasty = event->y();
   event->accept();
}

void BrainStemGL::mouseMoveEvent(QMouseEvent *event)
{
   int x = event->x();
   int y = event->y();
   Qt::MouseButtons buttons = event->buttons(); 

   int diffx = x - lastx;
   int diffy = y - lasty;
   lastx = x;
   lasty = y;

   if((buttons & Qt::LeftButton) && (buttons & Qt::RightButton))
   {
      view_rotz += (float) 0.5f * diffx;
      if (view_rotz >= 360.0)
         view_rotz -= 360.0;
      else if (view_rotz <= -360.0)
         view_rotz += 360.0;
      update();
   }
   else if(buttons & Qt::LeftButton)
   {
      view_rotx += (float) 0.5f * diffy;
      if (view_rotx >= 360.0)
         view_rotx -= 360.0;
      else if (view_rotx <= -360.0)
         view_rotx += 360.0;
      view_roty += (float) 0.5f * diffx;
      if (view_roty >= 360.0)
         view_roty -= 360.0;
      else if (view_roty <= -360.0)
         view_roty += 360.0;
      update();
   }
   else if(buttons & Qt::RightButton)
   {
      view_trx += (float) 0.05f * diffx;
      view_try -= (float) 0.05f * diffy;
      update();
   }
//   QString msg;
//   QTextStream(&msg) << "X angle: " << view_rotx << "  Y angle: " << view_roty << " Z angle: " << view_rotz << endl;
//   emit(chatBox(msg));
   event->accept();
}

// Wheel zooms in and out
void BrainStemGL::wheelEvent(QWheelEvent *event)
{
   if (event->angleDelta().y() > 0)
   {
      radius *= 0.9;  
      if (radius < 2.0) // zoom in too close causes fp precison problems.
         radius = 2.0;  // many frags are at same Z coord, so OIT sorting fails.
   }
   else if (radius < 1000.0)  // at this point, a tiny dot, small enough
      radius *= 1.1;

//   QString msg;
//   QTextStream(&msg) << "angledelta:  " << event->angleDelta().y() << "  radius: " << radius << "  trz: " << view_trz << "  FOV: " << FOV << endl;
//   cout << msg.toLatin1().constData()  << endl;

   makeCurrent();  // normally done for us, but need to do it explicitly here
   int w = geometry().width();
   int h = geometry().height();
   resizeGL(w,h);
   doneCurrent();
   update();
   event->accept();
}

void BrainStemGL::extremes()
{
  int n;
  for (n = 1; n < plate_count; n++) {
    int p;
    for (p = 0; p < point_count[n]; p++) {

      if (plate[n][p][0] < xlo)
        xlo = plate[n][p][0];
      if (plate[n][p][0] > xhi)
        xhi = plate[n][p][0];

      if (plate[n][p][1] < ylo)
        ylo = plate[n][p][1];
      if (plate[n][p][1] > yhi)
        yhi = plate[n][p][1];

      if (plate[n][p][2] < zlo)
        zlo = plate[n][p][2];
      if (plate[n][p][2] > zhi)
        zhi = plate[n][p][2];
    }
  }
  xmidsave = xmid = (xlo + xhi) / 2;
  ymidsave = ymid = (ylo + yhi) / 2;
  zmidsave = zmid = (zlo + zhi) / 2;
  radius0 = sqrt(pow((xhi - xlo),2) + pow((yhi - ylo),2) + pow((zhi - zlo),2)) / 2;
  if (radius == -1.0)  // not using saved value
     radius = radius0;
}

void BrainStemGL::axes()
{
   float tick;
   vector<glm::vec3> axesLines;
   vector<glm::vec3>::iterator iter;
   GLenum err_chk = glGetError();

   axesLines.push_back(glm::vec3(-XLEN, 0.0, 0.0));   // X
   axesLines.push_back(glm::vec3(XLEN, 0.0, 0.0 ));

   for (tick=MINOR_TICK; tick <= XLEN; tick += 1.0)    // ticks
   {
      axesLines.push_back(glm::vec3(tick, MINOR_TICK_LEN, 0.));
      axesLines.push_back(glm::vec3(tick, -MINOR_TICK_LEN, 0.));
      axesLines.push_back(glm::vec3(-tick, MINOR_TICK_LEN, 0. ));
      axesLines.push_back(glm::vec3(-tick, -MINOR_TICK_LEN, 0. ));
      axesLines.push_back(glm::vec3(tick+0.5, MAJOR_TICK_LEN, 0. ));
      axesLines.push_back(glm::vec3(tick+0.5, -MAJOR_TICK_LEN, 0. ));
      axesLines.push_back(glm::vec3(-(tick+0.5), MAJOR_TICK_LEN, 0. ));
      axesLines.push_back(glm::vec3(-(tick+0.5), -MAJOR_TICK_LEN, 0. ));
   }

   axesLines.push_back(glm::vec3(0., -YLEN, 0. ));         // Y
   axesLines.push_back(glm::vec3(0., YLEN, 0. ));
   for (tick=MINOR_TICK; tick <= YLEN; tick += 1.0)
   {
      axesLines.push_back(glm::vec3(MINOR_TICK_LEN,  tick, 0. ));
      axesLines.push_back(glm::vec3(-MINOR_TICK_LEN, tick, 0. ));
      axesLines.push_back(glm::vec3(MINOR_TICK_LEN, -tick, 0. ));
      axesLines.push_back(glm::vec3(-MINOR_TICK_LEN, -tick, 0. ));
      axesLines.push_back(glm::vec3(MAJOR_TICK_LEN, tick+0.5, 0. ));
      axesLines.push_back(glm::vec3(-MAJOR_TICK_LEN, tick+0.5, 0. ));
      axesLines.push_back(glm::vec3(MAJOR_TICK_LEN, -(tick+0.5), 0. ));
      axesLines.push_back(glm::vec3(-MAJOR_TICK_LEN, -(tick+0.5), 0. ));
   }

   axesLines.push_back(glm::vec3(0., 0., ZLENP));   // Z
   axesLines.push_back(glm::vec3(0., 0., -ZLENN));
   for (tick=MINOR_TICK; tick <= ZLENN; tick += 1.0)
   {
      axesLines.push_back(glm::vec3(MINOR_TICK_LEN, 0., -tick));
      axesLines.push_back(glm::vec3(-MINOR_TICK_LEN, 0., -tick));
      axesLines.push_back(glm::vec3(MAJOR_TICK_LEN, 0., -(tick+0.5)));
      axesLines.push_back(glm::vec3(-MAJOR_TICK_LEN, 0., -(tick+0.5)));
   }
   for (tick=MINOR_TICK; tick <= ZLENP; tick += 1.0)
   {
      axesLines.push_back(glm::vec3(MINOR_TICK_LEN, 0., tick));
      axesLines.push_back(glm::vec3(-MINOR_TICK_LEN, 0., tick));
      axesLines.push_back(glm::vec3(MAJOR_TICK_LEN, 0., tick+0.5));
      axesLines.push_back(glm::vec3(-MAJOR_TICK_LEN, 0.,tick+0.5));
   }

     // stuff data
   int num_bytes = axesLines.size() * sizeof(glm::vec3);
   axesSize = axesLines.size();  // # of objects
   glGenVertexArrays(1,&axesVao);
   glBindVertexArray(axesVao);
   err_chk = glGetError();
   if (err_chk != 0)
      cout << "axis error 1 is: " << err_chk << endl;

   glGenBuffers(1,&axesVbo);
   glBindBuffer(GL_ARRAY_BUFFER,axesVbo);
   glBufferData(GL_ARRAY_BUFFER,num_bytes,axesLines.data(),GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(0);
   err_chk = glGetError();
   if (err_chk != 0)
      cout << "axis error 2 is: " << err_chk << endl;
   glBindVertexArray(0);
}

void BrainStemGL::outlines()
{
   vector<GLfloat> allpts;
   vector<GLuint> index;
   size_t num_plates = sizeof(point_count)/sizeof(int);
   int count;
   int pt;
   float (*curr_plate)[3];
   int idx = 0;

   for (count = 1; count < int(num_plates); ++count)
   {
      curr_plate = plate[count];
      for (pt = 0; pt < point_count[count]; ++idx, ++pt)  // , ++curr_plate)
      {
         allpts.push_back(curr_plate[pt][0]);
         allpts.push_back(curr_plate[pt][1]);
         allpts.push_back(curr_plate[pt][2]);
         index.push_back(idx);
      }
      if (count < int(num_plates)-1)
         index.push_back(RESTART_MARKER); // no trailing marker
   }
   outlinesIdxSize = index.size(); // need this later for drawing using indicies
   GLenum err_chk = glGetError();

   glGenVertexArrays(1,&outlinesVao);  // make VAO
   glBindVertexArray(outlinesVao);
   glGenBuffers(1,&outlinesVbo);      // stuff data into VBO
   glBindBuffer(GL_ARRAY_BUFFER,outlinesVbo);
   glBufferData(GL_ARRAY_BUFFER,allpts.size()*sizeof(GLfloat),allpts.data(),GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(0);

   err_chk = glGetError();
   if (err_chk != 0)
      cout << "outlines error 1 is: " << err_chk << endl;
   glGenBuffers(1,&outlinesIdx);   
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,outlinesIdx);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,index.size()*sizeof(GLuint),index.data(),GL_STATIC_DRAW);

   err_chk = glGetError();
   if (err_chk != 0)
      cout << "outlines error 2 is: " << err_chk << endl;
}

void BrainStemGL::printInfo(QString& info)
{
   makeCurrent();
   GLenum err_chk;
   err_chk = glGetError();

   QColor matchit;
   QPoint dest(PHRENIC_X,PHRENIC_Y);
   QFont font("Courier",8);
   QFontMetrics f_met(font);
   QImage timage(DRAWBOX_W,DRAWBOX_H,QImage::Format_ARGB32);
   QPainter qpaint(&timage);

   matchit.setRgbF(backColor,backColor,backColor,1.0);
   qpaint.fillRect(0,0,DRAWBOX_W,DRAWBOX_H,matchit);
   qpaint.setOpacity(0.9);
   if (backColor < 0.5)
      qpaint.setPen(Qt::white);
   else
      qpaint.setPen(Qt::black);

   font.setStyleStrategy(QFont::NoAntialias);
   qpaint.setFont(font);
   qpaint.drawText(1,f_met.height(),info);
   if (havePhrenic)
      qpaint.drawImage(dest,phrenicImageG);
   else
      qpaint.drawImage(dest,noPhrenicImage);
   qpaint.setPen(QColor(233,59,25));

   qpaint.drawLine(QPoint(phrenicLinePos,PHRENIC_Y+2),
                   QPoint(phrenicLinePos,PHRENIC_Y+PHRENIC_H-2));

   QImage cvt = QGLWidget::convertToGLFormat(timage);

   glUseProgram(printProg);
   glBindVertexArray(printVao);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,printText); // update texture buffer with new info
   err_chk = glGetError();
   if (err_chk != 0)
      cout << "texture error 1 is: " << err_chk << endl;
//timage.save("test.png");
//cvt.save("testcvt.png");

   glTexSubImage2D(GL_TEXTURE_2D, 
                  0, 
                  0,
                  0,
                DRAWBOX_W,
                DRAWBOX_H,
                GL_RGBA,
                GL_UNSIGNED_BYTE, 
                (void*)cvt.bits());
   err_chk = glGetError();
   if (err_chk != 0)
      cout << "texture error 2 is: " << err_chk << endl;
   glBindTexture(GL_TEXTURE_2D,0);
   glUseProgram(0);
   doneCurrent();
   update();
}


void BrainStemGL::clearInfo()
{
   QColor matchit;
   QImage timage(DRAWBOX_W,DRAWBOX_H,QImage::Format_ARGB32);
   QPainter qpaint(&timage);
   matchit.setRgbF(backColor,backColor,backColor,1.0);
   qpaint.fillRect(0,0,DRAWBOX_W,DRAWBOX_H,matchit);
   qpaint.setOpacity(1.0);
   QImage cvt = QGLWidget::convertToGLFormat(timage);

   makeCurrent();
   glUseProgram(printProg);
   glBindVertexArray(printVao);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,printText); // update texture buffer with new info
   glTexSubImage2D(GL_TEXTURE_2D, 
                  0, 
                  0,
                  0,
                DRAWBOX_W,
                DRAWBOX_H,
                GL_RGBA,
                GL_UNSIGNED_BYTE, 
                (void*)cvt.bits());
   glBindTexture(GL_TEXTURE_2D,0);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   glUseProgram(0);
   doneCurrent();
   printClear = true;
   update();
}

// A rectangle that we draw an image on as a texture.
// Image is text, maybe other stuff
void BrainStemGL::printBox()
{
   QColor matchit;
   QImage timage(DRAWBOX_W,DRAWBOX_H,QImage::Format_ARGB32);
   QPainter qpaint(&timage);
   matchit.setRgbF(backColor,backColor,backColor,1.0);
   qpaint.fillRect(0,0,DRAWBOX_W,DRAWBOX_H,matchit);
   qpaint.setOpacity(1.0);
   QImage cvt = QGLWidget::convertToGLFormat(timage);

   glGenBuffers(1,&printVbo); 
   glBindBuffer(GL_ARRAY_BUFFER,printVbo);
   glBufferData(GL_ARRAY_BUFFER,sizeof(drawBox),drawBox,GL_STATIC_DRAW);

   glGenBuffers(1,&texVbo);
   glBindBuffer(GL_ARRAY_BUFFER,texVbo);
   glBufferData(GL_ARRAY_BUFFER,sizeof(drawTextureBox),drawTextureBox,GL_STATIC_DRAW);

   sizeText = sizeof(drawTextureBox)/(sizeof(GLfloat)*2);

   glGenVertexArrays(1,&printVao);
   glBindVertexArray(printVao);
   glBindBuffer(GL_ARRAY_BUFFER,printVbo);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glBindBuffer(GL_ARRAY_BUFFER,texVbo);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);

   glUseProgram(printProg);
   glGenTextures(1,&printText);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,printText);
   glTexImage2D(GL_TEXTURE_2D, 
                  0,
                GL_RGBA,
                DRAWBOX_W,
                DRAWBOX_H,
                  0,
                GL_RGBA,
                GL_UNSIGNED_BYTE, 
                (void*)cvt.bits());
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D,0);
   glUseProgram(0);
}


// Set up the skin triangles and normals for skin
void BrainStemGL::skin()
{
   ptCoords triangles;
   ptCoords norms;
   int num_bytes;
   int pt;

   if (numbrainSkin != numskinNorms)
      cout << "Error:  number of skin pts and norms not the same, continuing anyway" << endl;

   for (pt = 0; pt < numbrainSkin; ++pt)
      triangles.push_back(glm::vec3(brainSkin[pt][0],brainSkin[pt][1],brainSkin[pt][2]));

   num_bytes = triangles.size() * sizeof(glm::vec3);
   skinSize = triangles.size();

   for (pt = 0; pt < numskinNorms; ++pt)
      norms.push_back(glm::vec3(skinNorms[pt][0],skinNorms[pt][1],skinNorms[pt][2]));
   GLenum err_chk = glGetError();

   glGenVertexArrays(1,&skinVao);
   glBindVertexArray(skinVao);
   glGenBuffers(1,&skinVbo);
   glBindBuffer(GL_ARRAY_BUFFER,skinVbo);
   glBufferData(GL_ARRAY_BUFFER,num_bytes,triangles.data(),GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(0);

   glGenBuffers(1,&normVbo);
   glBindBuffer(GL_ARRAY_BUFFER,normVbo);
   glBufferData(GL_ARRAY_BUFFER,num_bytes,norms.data(),GL_STATIC_DRAW);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(1);

   err_chk = glGetError();
   if (err_chk != 0)
      cout << "skin error 1 is: " << err_chk << endl;
}


// Set up the brainstem structures
void BrainStemGL::stemStructs()
{
   float (*curr_obj)[3];
   float (*curr_norm)[3];
   vector<GLfloat> triangles;
   vector<GLfloat> norms;
   int count, obj_count, num_bytes;
   int pt;
   oneStruct curr_offset;

   if (numStructs != numNormStructs)
      cout << "Warning Structure vertices and norms are different lengths" << endl;

   for (count = 0; count < numStructs; ++count)
   {
      curr_obj = allStructs[count];
      curr_norm = objNorms[count];
      curr_offset.first = objSizes[count][0];
      curr_offset.count = objSizes[count][1];
      selStructs.push_back(curr_offset);
      obj_count = curr_offset.count;

      for (pt = 0; pt < obj_count; ++pt)
      {
         triangles.push_back(curr_obj[pt][0]);
         triangles.push_back(curr_obj[pt][1]);
         triangles.push_back(curr_obj[pt][2]);

         norms.push_back(curr_norm[pt][0]);
         norms.push_back(curr_norm[pt][1]);
         norms.push_back(curr_norm[pt][2]);
      }
   }

   num_bytes = triangles.size() * sizeof(GLfloat);

   GLenum err_chk = glGetError();
   if (err_chk != 0)
      cout << "stemstruct 1 is: " << err_chk << endl;

   glGenVertexArrays(1,&structVao);
   glBindVertexArray(structVao);

   glGenBuffers(1,&structVbo);
   glBindBuffer(GL_ARRAY_BUFFER,structVbo);
   glBufferData(GL_ARRAY_BUFFER,num_bytes,nullptr,GL_STATIC_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER,0,num_bytes,triangles.data());
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(0);

   glGenBuffers(1,&structNormVbo);
   glBindBuffer(GL_ARRAY_BUFFER,structNormVbo);
   glBufferData(GL_ARRAY_BUFFER,num_bytes,nullptr,GL_STATIC_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER,0,num_bytes,norms.data());
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(1);

   err_chk = glGetError();
   if (err_chk != 0)
      cout << "stemstruct 2 is: " << err_chk << endl;
}

// Create array of sphere vertices and normals for vaos later
void BrainStemGL::sphere()
{
   int pt;

   if (numsphereVert != numsphereNorm)
      cout << "Error:  sphere pts and norms not the same, continuing anyway" << endl;

   for (pt = 0; pt < numsphereVert; ++pt)
   {
      sphereV.push_back(glm::vec3(sphereVert[pt][0],sphereVert[pt][1],sphereVert[pt][2]));
   }
   sphereSize = sphereV.size();

   for (pt = 0; pt < numsphereNorm; ++pt)
      sphereN.push_back(glm::vec3(sphereNorm[pt][0],sphereNorm[pt][1],sphereNorm[pt][2]));
}

// Set up fragment linked lists for Order Independent Transparency sorting
// on the second rendering pass
void BrainStemGL::oit()
{
   GLenum err_chk = glGetError();

    // Atomic counter
   glGenBuffers(1, &atomicCounterBuff);
   glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterBuff);
   glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

    // don't overflow linked list buff
   GLuint maxCap = MAX_OIT_NODES * OIT_NODE_SIZE;
   glGenBuffers(1,&nodeUbo);
   glBindBufferBase(GL_UNIFORM_BUFFER,nodeUboBlkId,nodeUbo);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(maxCap), &maxCap, GL_STATIC_DRAW);

      // for debugging, shaders keep track of max nodes
//   glGenBuffers(1,&nodeCount);
//   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,nodeId,nodeCount);
//   glBufferData(GL_UNIFORM_BUFFER, sizeof(GLuint), null, GL_DYNAMIC_DRAW);

     // Head pointer buffer, one item for screen w & h. This is the start of the OIT
     // linked lists. 
   glGenTextures(1, &headPointerTex);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, headPointerTex);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, MAX_FB_WIDTH, MAX_FB_HEIGHT);
   glBindImageTexture(0, headPointerTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    // Linked list storage buffer, head pointers above point to lists in this
   glGenBuffers(1, &linkedListBuff);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, linkedListBuff);
   glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_OIT_NODES * OIT_NODE_SIZE, nullptr, GL_DYNAMIC_DRAW);

    // Buffer for (re)initing the head pointer texture
   vector <GLuint> initValue(MAX_FB_WIDTH*MAX_FB_HEIGHT,0xFFFFFFFF);
   glGenBuffers(1, &headPointerInitVal);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, headPointerInitVal);
   glBufferData(GL_PIXEL_UNPACK_BUFFER, initValue.size()*sizeof(GLuint), &initValue[0], GL_STATIC_COPY);

     // Rect we use to get pixel (x,y) values from while rendering sorted vertices
   glGenVertexArrays(1, &showVao);
   glBindVertexArray(showVao);
   glGenBuffers(1,&showVbo);
   glBindBuffer(GL_ARRAY_BUFFER, showVbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(showVerts), showVerts, GL_STATIC_DRAW);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
   glEnableVertexAttribArray(0);
   glClearDepthf(1.0f);

   glBindVertexArray(0);            // nothing in scope
   glBindTexture(GL_TEXTURE_2D,0);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

   err_chk = glGetError();
   if (err_chk != 0)
      cout << "oit error is: " << err_chk << endl;
}

// Time to update the cells.
// Two cases
//   new file - clear all the old stuff out and build new gl stuff
//   on_off  -   only show selected on_off_vals
void BrainStemGL::updateCells(bool new_file, bool exp_chg, bool delta_cths, vector<int>& on_off_vals, vector<int>& exp_on_off, cellArray& dispCells, ClustRGB& rgbClustMap, bool have_phrenic)
{
   int curr_bin, curr_clust, coloridx;
   size_t rows;
   GLuint  newVao;
   size_t  c_coord_bytes, s_coord_bytes, d_coord_bytes;
   size_t  c_color_bytes, s_color_bytes, d_color_bytes;
   size_t set;
   size_t tot_vaos;
   double normval;
   double deltaval;
   ptCoords triangles;
   ptCoords norms;
   int num_sphere_bytes;
   array <ptCoords,NUM_PT_LISTS> cthCoords; 
   array <colorIdx,NUM_CELL_COLORS> cthColorIdx;
   CTHIter cthiter;
   CellIter citer, citerctl, citerstim, citerdelta;
   bool have_ctl = false;
   bool have_stim = false;
   ClusterIter cell;
   GLenum err_chk;
   GLuint sphere_pt_vbo, sphere_norm_vbo;
   GLuint ctl_pt_vbo, ctl_color_vbo;
   GLuint stim_pt_vbo, stim_color_vbo;
   GLuint delta_pt_vbo, delta_color_vbo;
   QString msg;
   int num_clusts = on_off_vals.size();

   haveDelta = delta_cths;
   onOff = on_off_vals;
   havePhrenic = have_phrenic;
   if (haveDelta)              // add fake checkbox at end for delta CTHs
      onOff.push_back(true);
   c_coord_bytes=s_coord_bytes=d_coord_bytes=0;
   c_color_bytes=s_color_bytes=d_color_bytes=0;

   if (new_file || exp_chg) // must (re) build vaos 
   {
      clearInfo();
      makeCurrent();         // operate in current GL context
      clearClusts();
      createShades(rgbClustMap);
      updateCellProg();

         // If we get this far, at least one list has cells. Get # bins.
         // We currently only support files with just control periods,
         // or ctl/stim periods for one type of stim. We assume the loader
         // has checked for this and will not call us if there are no 
         // or too many periods.
      int ctl_bins = 0;
      int stim_bins = 0;
      if (dispCells[CONTROL_COLORS].size())
         have_ctl = true;
      if (dispCells[STIM_COLORS].size())
         have_stim = true;

      if (have_ctl)
      {
         citer = dispCells[CONTROL_COLORS].begin();
         if (citer != dispCells[CONTROL_COLORS].end())
            ctl_bins = citer->second.begin()->normCth.size();
      }
      if (have_stim) 
      {
         citer = dispCells[STIM_COLORS].begin();
         if (citer != dispCells[STIM_COLORS].end())
            stim_bins = citer->second.begin()->normCth.size();
      }
   
      numBins = max(ctl_bins,stim_bins);  // really should be same if we have both
      tot_vaos=numBins+1;  // 1 vao for base color and bins (if any)
      currCycle = 0;
      if (numBins)
         phrenicStep = double(PHRENIC_E_END - PHRENIC_I_START)/numBins;
      else
         phrenicStep = 0;

        // sphere object global to all cell VAOs
      num_sphere_bytes = sphereSize * sizeof(glm::vec3);
      glGenBuffers(1,&sphere_pt_vbo);
      glBindBuffer(GL_ARRAY_BUFFER,sphere_pt_vbo);
      glBufferData(GL_ARRAY_BUFFER,num_sphere_bytes,sphereV.data(),GL_STATIC_DRAW);
       // normals for sphere
      glGenBuffers(1,&sphere_norm_vbo);
      glBindBuffer(GL_ARRAY_BUFFER,sphere_norm_vbo);
      glBufferData(GL_ARRAY_BUFFER,num_sphere_bytes,sphereN.data(),GL_STATIC_DRAW);

      for (curr_clust = 0; curr_clust < num_clusts; ++curr_clust)
      {
         for (int cells=CONTROL_PTS; cells < NUM_PT_LISTS; ++cells)
            cthCoords[cells].clear();
         for (int cells=CONTROL_COLORS; cells < NUM_CELL_COLORS; ++cells)
         {
            cthColorIdx[cells].clear();
            cthColorIdx[cells].resize(numBins+1);
         }
           // Collect cell info and build pts, color list and color cycling
           // lists if we have bins.
           // For all pts in current cluster make pt & primary color lists
         if (have_ctl)
         {
              // not all periods are in every cluster, so check for this
            citerctl = dispCells[CONTROL_COLORS].find(curr_clust);
            if (citerctl != dispCells[CONTROL_COLORS].end())
            {
               for (cell=citerctl->second.begin(); cell != citerctl->second.end(); ++cell)
               {
                      // some cells don't have stereotaxic coords.  If at origin,
                      // it is one of those cells. Don't draw it.
                  if ( cell->rl == 0.0 && cell->dp == 0.0 && cell->ap == 0.0)
                     continue;
                   // if we have experiment(s), is this cell in a selected experiment(s)
                  if ((exp_on_off.size() == 0) || exp_on_off[cell->expidx])
                  {
                     cthCoords[CONTROL_PTS].push_back(glm::vec3(cell->rl, -cell->dp, -cell->ap));
                       // index of brightest color for this cluster
                     cthColorIdx[CONTROL_COLORS][0].push_back(cell->coloridx*COLOR_STEPS);
                  }
               }
                  // optional bin colors
               for (curr_bin=0,rows = 1; rows <= numBins; ++curr_bin,++rows)
               {
                  for (ClusterIter normiter=citerctl->second.begin(); normiter != citerctl->second.end(); ++normiter)
                  {
                     normval = normiter->normCth[curr_bin];
                     coloridx = floor((COLOR_STEPS-1) - (normval*(COLOR_STEPS-1)));
                     if (coloridx >= COLOR_STEPS) // handle zero case
                         coloridx = COLOR_STEPS-1;
                     coloridx += COLOR_STEPS * normiter->coloridx; // offset into table
                     cthColorIdx[CONTROL_COLORS][rows].push_back(coloridx);
                  }
               }
            }
         }

         if (have_stim)
         {
            citerstim = dispCells[STIM_COLORS].find(curr_clust);
            if (citerstim != dispCells[STIM_COLORS].end())
            {
                // do STIM period coords & colors
               for (cell=citerstim->second.begin(); cell != citerstim->second.end(); ++cell)
               {
                  if (cell->rl == 0.0 && cell->dp == 0.0 && cell->ap == 0.0)
                     continue;
                  if ((exp_on_off.size() == 0) || exp_on_off[cell->expidx])
                  {
                     cthCoords[STIM_PTS].push_back(glm::vec3(cell->rl, -cell->dp, -cell->ap));
                     cthColorIdx[STIM_COLORS][0].push_back(cell->coloridx*COLOR_STEPS); 
                  }
               }
               for (curr_bin=0,rows = 1; rows <= numBins; ++curr_bin,++rows)
               {
                  for (ClusterIter deltaiter=citerstim->second.begin(); deltaiter != citerstim->second.end(); ++deltaiter)
                  {
                     deltaval = deltaiter->normCth[curr_bin];
                     coloridx = floor((COLOR_STEPS-1) - (deltaval*(COLOR_STEPS-1)));
                     if (coloridx >= COLOR_STEPS) // handle zero case
                         coloridx = COLOR_STEPS-1;
                     coloridx += COLOR_STEPS * deltaiter->coloridx; // offset into table
                     cthColorIdx[STIM_COLORS][rows].push_back(coloridx);
                  }
               }
            }
         }

            // generate a [pt list] [ color list] so we have a total of
            // at least one list if no CTH data,  OR 1 for each bin
            // total # of VAOs for this cluster is # of cths
         err_chk = glGetError(); // clear errors

         if (have_ctl)
         {
            c_coord_bytes = cthCoords[CONTROL_PTS].size() * sizeof(glm::vec3); 
            c_color_bytes = cthColorIdx[CONTROL_COLORS][0].size() * sizeof(coloridx);
         }

         if (have_stim)
         {
            s_coord_bytes = cthCoords[STIM_PTS].size() * sizeof(glm::vec3); 
            s_color_bytes = cthColorIdx[STIM_COLORS][0].size() * sizeof(coloridx);
         }

         for (set = 0; set < tot_vaos; ++set)
         {
            glGenVertexArrays(1,&newVao);
            glBindVertexArray(newVao);
            err_chk = glGetError();
            if (err_chk != 0)
               cout << "error update cells 1 is: " << err_chk << endl;

            glBindBuffer(GL_ARRAY_BUFFER,sphere_pt_vbo); // sphere vertex coords
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER,sphere_norm_vbo); // sphere normals
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);

            if (have_ctl && c_coord_bytes)
            {
               glGenBuffers(1,&ctl_pt_vbo);       // cell coords
               glBindBuffer(GL_ARRAY_BUFFER,ctl_pt_vbo);
               glBufferData(GL_ARRAY_BUFFER,c_coord_bytes,cthCoords[CONTROL_PTS].data(),GL_STATIC_DRAW);
               glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
               glEnableVertexAttribArray(2);
               glVertexAttribDivisor(2,1); // 1 position per sphere instance

               glGenBuffers(1,&ctl_color_vbo);       // cell color indexes
               glBindBuffer(GL_ARRAY_BUFFER,ctl_color_vbo);
               glBufferData(GL_ARRAY_BUFFER,c_color_bytes,cthColorIdx[CONTROL_COLORS][set].data(),GL_STATIC_DRAW);
               glVertexAttribIPointer(5, 1, GL_INT, 0, nullptr);
               glEnableVertexAttribArray(5);
               glVertexAttribDivisor(5,1); // 1 color index per sphere instance
            }

            if (have_stim && s_coord_bytes)
            {
               glGenBuffers(1,&stim_pt_vbo);      // stim cell coords
               glBindBuffer(GL_ARRAY_BUFFER,stim_pt_vbo);
               glBufferData(GL_ARRAY_BUFFER,s_coord_bytes,cthCoords[STIM_PTS].data(),GL_STATIC_DRAW);
               glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
               glEnableVertexAttribArray(3);
               glVertexAttribDivisor(3,1); // 1 position per sphere instance

               glGenBuffers(1,&stim_color_vbo);       // stim color indexes
               glBindBuffer(GL_ARRAY_BUFFER,stim_color_vbo);
               glBufferData(GL_ARRAY_BUFFER,s_color_bytes,cthColorIdx[STIM_COLORS][set].data(),GL_STATIC_DRAW);
               glVertexAttribIPointer(6, 1, GL_INT, 0, nullptr);
               glEnableVertexAttribArray(6);
               glVertexAttribDivisor(6,1); // 1 color index per sphere instance
            }

            err_chk = glGetError();
            if (err_chk != 0)
               cout << "error update cells 2 is: " << err_chk << endl;

            glCTH point = { newVao,{cthCoords[CONTROL_PTS].size(),
                                    cthCoords[STIM_PTS].size(),
                                    0}};  // painting list, no delta so far
            cellsVao[curr_clust].push_back(point);
            err_chk = glGetError();
            if (err_chk != 0)
               cout << "error update cells 3 is: " << err_chk << endl;
         }
      }

      if (haveDelta)
      {
         GLuint colsiz = deltaShadeTab.size() * sizeof(glm::vec4);
         glUseProgram(cellProg);
         glBindBuffer(GL_SHADER_STORAGE_BUFFER,deltaTabVbo);
         glBufferData(GL_SHADER_STORAGE_BUFFER,colsiz,nullptr,GL_STATIC_DRAW);
         glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, deltaTabVbo, 0, colsiz);
         glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, colsiz,deltaShadeTab.data());
         glUseProgram(0);
         glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);

         citerdelta = dispCells[DELTA_COLORS].find(0); // no clusters, just 1 set
         if (citerdelta != dispCells[DELTA_COLORS].end())
         {
              // do DELTA period coords & colors 
            for (cell=citerdelta->second.begin(); cell != citerdelta->second.end(); ++cell)
            {
               if (cell->rl == 0.0 && cell->dp == 0.0 && cell->ap == 0.0)
                  continue;
               if ((exp_on_off.size() == 0) || exp_on_off[cell->expidx])
               {
                  cthCoords[DELTA_PTS].push_back(glm::vec3(cell->rl, -cell->dp, -cell->ap));
                  cthColorIdx[DELTA_COLORS][0].push_back(D_COLOR_STEPS-1); // default
               }
            }
            for (curr_bin=0,rows = 1; rows <= numBins; ++curr_bin,++rows)
            {
               for (ClusterIter deltaiter=citerdelta->second.begin(); deltaiter != citerdelta->second.end(); ++deltaiter)
               {
                  deltaval = deltaiter->normCth[curr_bin];
                  if (deltaval >= 0)
                     coloridx = floor(D_COLOR_STEPS - (deltaval*D_COLOR_STEPS));
                  else
                     coloridx = ceil((D_COLOR_STEPS+1) - (deltaval*D_COLOR_STEPS));
                  cthColorIdx[DELTA_COLORS][rows].push_back(coloridx);
               }
            }
         }
         for (set = 0; set < tot_vaos; ++set)  // now send to opengl
         {
            glGenVertexArrays(1,&newVao);
            glBindVertexArray(newVao);

            glBindBuffer(GL_ARRAY_BUFFER,sphere_pt_vbo); // sphere vertex coords
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER,sphere_norm_vbo); // sphere normals
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);

            d_coord_bytes = cthCoords[DELTA_PTS].size() * sizeof(glm::vec3); 
            d_color_bytes = cthColorIdx[DELTA_COLORS][0].size() * sizeof(coloridx);
            glGenBuffers(1,&delta_pt_vbo);       // cell coords
            glBindBuffer(GL_ARRAY_BUFFER,delta_pt_vbo);
            glBufferData(GL_ARRAY_BUFFER,d_coord_bytes,cthCoords[DELTA_PTS].data(),GL_STATIC_DRAW);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4,1); // 1 position per sphere instance

            glGenBuffers(1,&delta_color_vbo);       // cell color indexes
            glBindBuffer(GL_ARRAY_BUFFER,delta_color_vbo);
            glBufferData(GL_ARRAY_BUFFER,d_color_bytes,cthColorIdx[DELTA_COLORS][set].data(),GL_STATIC_DRAW);
            glVertexAttribIPointer(7, 1, GL_INT, 0, nullptr);
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7,1); // 1 color index per sphere instance

             // we become the last "cluster"
            glCTH point = {newVao,0,0,cthCoords[DELTA_PTS].size()};
            cellsVao[curr_clust].push_back(point);
         }
      }

      doneCurrent();
      glBindVertexArray(0);               // nothing in scope
      glBindBuffer(GL_ARRAY_BUFFER,0);
      glUseProgram(0);
   }
   update();
}

void BrainStemGL::closeFile()
{
   makeCurrent();
   clearClusts();
   doneCurrent();
   clearInfo();
}



// Build the color brightness shades for color cycling.
// Also, for ctl/stim delta, create all the colors.
void BrainStemGL::createShades(ClustRGB& rgbClustMap)
{
   float step;
   int row, bright;
   int num_colors = rgbClustMap.size();
   ClustRGBIter rgbIter;
   glm::vec3 hsv;
   glm::vec3 newrgb;
   glm::vec3 one_rgb;
   glm::vec3 one_hsv;
   colorBright shade;
   glm::vec3 gamma(1.0/2.2);

   one_hsv = glm::hsvColor(glm::vec3(0.0,1.0,0.0)); // all green shades
   step = one_hsv.z / (COLOR_STEPS-1);
   for (bright = 0 ; bright < COLOR_STEPS; ++bright)
   {
      if (bright < COLOR_STEPS-1)
         newrgb = glm::rgbColor(one_hsv);
      else
         newrgb = glm::vec3(0.0);  // make black really black
      newrgb = glm::pow(newrgb,gamma);
      shade.push_back(glm::vec4(newrgb,1.0));
      one_hsv.z -= step;
      if (one_hsv.z < 0)
         one_hsv.z = 0;
   }

      // generate a set of COLOR_STEP shades for each cluster's primary color
   for (row = 0; row < num_colors ; ++row)
   {
      rgbIter = rgbClustMap.find(row);
      glm::vec3 rgb(glm::vec3(rgbIter->second.r, rgbIter->second.g,rgbIter->second.b));
      if ((rgb.r == rgb.g) && (rgb.r == rgb.b)) // bug in library if these are same
         hsv = glm::vec3(0.0f,0.0f,rgb.r);      // so work around it
      else
         hsv = glm::hsvColor(rgb);
      step = hsv.z / (COLOR_STEPS-1);
      for (bright=0; bright < COLOR_STEPS; ++bright)
      {
         if (bright < COLOR_STEPS-1)
            newrgb = glm::rgbColor(hsv);
         else
            newrgb = glm::vec3(0.0);  // make black really black
         newrgb = glm::pow(newrgb,gamma);
         glm::vec4 color4(newrgb,1.0);
         colorTab.push_back(color4);
         hsv.z -= step;
         if (hsv.z < 0)
            hsv.z = 0;
         oneShadeTab.push_back(shade[bright]);
      }
   }
   if (haveDelta)
   {
        // Build a list: [brightest col1 . . black . . brightest col2]
        // so there are 2*COLOR_STEPS colors + black in the middle.
        // The idea is to use the negative values in the delta cths values to index
        // into the col2 shades, where larger negatives means brigher color.
      deltaShadeTab.resize(D_COLOR_STEPS*2 + 1);
      glm::vec3 rgb(glm::vec3(1.0, 0.8, 0.0));  // yellow orange (gold)
      hsv = glm::hsvColor(rgb);
      step = hsv.z / D_COLOR_STEPS;
      for (bright=0; bright < D_COLOR_STEPS; ++bright)  // 0-11
      {
         newrgb = glm::rgbColor(hsv);
         newrgb = glm::pow(newrgb,gamma);
         glm::vec4 color4(newrgb,1.0);
         hsv.z -= step;
         if (hsv.z < 0)
            hsv.z = 0;
         deltaShadeTab[bright] = color4;
      }
          // 12
      deltaShadeTab[D_COLOR_STEPS] = glm::vec4(0.0,0.0,0.0,1.0); // black
      rgb = glm::vec3(0.40, 0.60, 0.80); // blue-gray (silver)
      hsv = glm::hsvColor(rgb);
      step = hsv.z / D_COLOR_STEPS;
      for (bright = 2*D_COLOR_STEPS; bright > D_COLOR_STEPS; --bright) // 24-13
      {
         newrgb = glm::rgbColor(hsv);
         newrgb = glm::pow(newrgb,gamma);
         glm::vec4 color4(newrgb,1.0);
         hsv.z -= step;
         deltaShadeTab[bright] = color4;
      }
   }
}


// Update the cell program with a new color table.
// Assumes caller has done a makeCurrent call
void BrainStemGL::updateCellProg()
{
   GLuint colsiz = colorTab.size() * sizeof(glm::vec4);
   glUseProgram(cellProg);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER,colorTabVbo);
   glBufferData(GL_SHADER_STORAGE_BUFFER,colsiz,nullptr,GL_STATIC_DRAW);
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, colorTabVbo, 0, colsiz);
   if (twinkleMode)
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, colsiz,oneShadeTab.data());
   else
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, colsiz,colorTab.data());
   glUseProgram(0);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);
}


// free up vaos and their vbos
void BrainStemGL::clearClusts()
{
   GLint numVbos;
   GLuint vboId = 0;
   GLuint curr_vao;

   if (cellsVao.size())
   {
      glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,&numVbos);
      for (vaoCellListIter iter = cellsVao.begin(); iter != cellsVao.end(); ++iter)
      {
         for (curr_vao = 0; curr_vao < iter->second.size(); ++curr_vao)
         {
            glBindVertexArray(iter->second[curr_vao].vao);
            for (int id = 0; id < numVbos; ++id)
            {
               vboId = 0;
               glGetVertexAttribIuiv(id,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&vboId);
               if (vboId)
                  glDeleteBuffers(1,&vboId);
            } 
            glDeleteVertexArrays(1,&(iter->second[curr_vao].vao));
         }
      }
      cellsVao.clear();
      colorTab.clear();
      oneShadeTab.clear();
      deltaShadeTab.clear();
      numBins = 0;
      phrenicLinePos = PHRENIC_I_START;
   }
}



// apply current rotations, translations, and redraw objects
void BrainStemGL::paintGL()
{
   int idx;
   GLuint *texdata;
   GLenum err_chk;
   glm::mat4 T1, T2, T2_3D, RX, RY, RY_3D, RZ;

// clock_gettime(CLOCK_MONOTONIC,&frameStart);

        // Various translation, rotation calcs
   float view_roty_3D = view_roty + 5.0;
   if (view_roty_3D >= 360.0)
      view_roty_3D -= 360.0;
      // model to world origin
   T1 = glm::translate(glm::mat4(1.0f),glm::vec3(-xmid,-ymid,-zmid));
      // spin it 
   RX  = glm::rotate(glm::mat4(1.0f),glm::radians(view_rotx),glm::vec3(1.0,0.0,0.0));
      // Y is fixed rotation + angle if animating
   RY  = glm::rotate(glm::mat4(1.0f),glm::radians(view_roty+angle),glm::vec3(0.0,1.0,0.0));
      // Y rotation for 3D viewing
   RY_3D = glm::rotate(glm::mat4(1.0f),glm::radians(view_roty_3D+angle),glm::vec3(0.0,1.0,0.0));
   RZ  = glm::rotate(glm::mat4(1.0f),glm::radians(view_rotz),glm::vec3(0.0,0.0,1.0));
     // back to model position
   T2 = glm::translate(glm::mat4(1.0f),glm::vec3(view_trx,view_try,view_trz));
   T2_3D = glm::translate(glm::mat4(1.0f),glm::vec3(-view_trx,view_try,view_trz));

   switch(stereoMode)
   {
      case CONTROL_STEREO:  // do Y rotation offset
      case STIM_STEREO:
      case DELTA_STEREO:
         RY_3D = glm::rotate(glm::mat4(1.0f),glm::radians(view_roty_3D+angle),glm::vec3(0.0,1.0,0.0));
         break;

      case CTL_STIM_PAIR:   // no Y offset
      case CONTROL_ONLY:
      case STIM_ONLY:
      case DELTA_ONLY:
      default:
          RY_3D  = glm::rotate(glm::mat4(1.0f),glm::radians(view_roty+angle),glm::vec3(0.0,1.0,0.0));
   }

   modelMat[0] = T2 * RX * RY * RZ * T1;            // std model
   mvpMat[0] = projMat * viewMat * modelMat[0]; 
   modelMat[1] = T2_3D * RX * RY_3D * RZ * T1;      // 3D model
   mvpMat[1] = projMat * viewMat * modelMat[1];
   mvMat[0] = viewMat * modelMat[0];   // normals
   mvMat[1] = viewMat * modelMat[1];

   if (stereoViewports == 1)   // 1 or 2 viewports
      glViewport(0,0,geometry().width(),geometry().height()); 
   else
   {
      glViewportIndexedf(0, 0.0, 0.0, viewPortW, viewPortH);
      glViewportIndexedf(1, geometry().width() - viewPortW, 0.0, viewPortW, viewPortH);
   }

     // send all this info to shaders
   glBindBufferBase(GL_UNIFORM_BUFFER,vUboBlkId,vUboBuff);
   unsigned char* mvp_ptr = (unsigned char*) glMapBufferRange(GL_UNIFORM_BUFFER,0,mvpSize,
                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
   memcpy(mvp_ptr,&mvpMat,mvpSize);
   glUnmapBuffer(GL_UNIFORM_BUFFER);

     // update transform for normals
   glBindBufferBase(GL_UNIFORM_BUFFER,nUboBlkId,nUboBuff);
   void* mv_ptr = glMapBufferRange(GL_UNIFORM_BUFFER,0,mvSize,
                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
   memcpy(mv_ptr,&mvMat,mvSize);
   glUnmapBuffer(GL_UNIFORM_BUFFER);
   err_chk = glGetError();
   if (err_chk != 0)
       cout << "paint error 1 is: " << err_chk << endl;

    // Do first render pass
    // Init vars to create fragment/pixel lists by the shaders for OIT sorting later
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   glDisable(GL_BLEND);

     // Reset atomic counter
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicCounterBuff);
    texdata = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
#if 0
// for debugging, the oit shader will return some info
    texdata = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_WRITE);
    cout << "nodes: " << texdata[0] << "  max: " << MAX_OIT_NODES << "  " << double(texdata[0])/MAX_OIT_NODES << "%" << " OIT buffer in use" << endl;
#endif
    texdata[0] = 0;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
#if 0
// more debugging
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,nodeId,nodeCount);
    GLuint* ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER,GL_READ_WRITE);
    if (ptr)
    {
       cout << "max nodes: in use " << ptr[0] << endl;
       ptr[0] = 0;
       glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
#endif
       // Re-init head-pointer image
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, headPointerInitVal);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, headPointerTex);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_FB_WIDTH,MAX_FB_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

   if (showOutlines)
   {
      glEnable(GL_PRIMITIVE_RESTART); // <- pre 4.5 opengl used this for non-indexed
      glUseProgram(outlineProg);      // draws, which it should not because it clobbers 
      glBindVertexArray(outlinesVao); // at least one structure object, so leave it off 
                                      // until we use it, then turn it back off.
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,outlinesIdx); //  until we need it
      glDrawElements(GL_LINE_LOOP,outlinesIdxSize,GL_UNSIGNED_INT,nullptr);
      glDisable(GL_PRIMITIVE_RESTART);
   }

   if (showAxes)
   {
      glUseProgram(axesProg);
      glBindVertexArray(axesVao);
      glDrawArrays(GL_LINES,0,axesSize);
   }

   if (twinkleOn || spinOn || printClear)  // print info
   {
      glUseProgram(printProg);
      glBindVertexArray(printVao);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D,printText);
      glDrawArrays(GL_TRIANGLES,0,sizeText);
      glBindTexture(GL_TEXTURE_2D,0);
      printClear = false;
   }

   if (skinOn)
   {
      glUseProgram(sort_skinProg); // create/add to transparency list
      glBindVertexArray(skinVao);
      glDrawArrays(GL_TRIANGLES,0,skinSize);
   }

   if (structsFirst.size() > 0)
   {
      glUseProgram(structProg);    // create/add to transparency list
      glBindVertexArray(structVao);
      glMultiDrawArrays(GL_TRIANGLES, structsFirst.data(), structsCount.data(),structsFirst.size()); 
   }
   
   if (cellsVao.size())
   {
      vaoCellListIter iter;
      glUseProgram(cellProg);
      void* stereo_ptr;
      GLuint mode;

      for (idx = 0, iter = cellsVao.begin(); iter != cellsVao.end(); ++idx,++iter)
      {
         if (onOff[idx])
         {
            glBindVertexArray(iter->second[currCycle].vao);
            switch (stereoMode)
            {
                // for this case, draw 1st coord/color list in 1st viewport, 
                // then switch mode and draw again using 2nd coord/color 
                // list in 2nd viewport
               case CTL_STIM_PAIR:
                  if (iter->second[currCycle].cellSize[CONTROL_PTS])
                  {
                     mode=CTL_PART;
                     glBindBufferBase(GL_UNIFORM_BUFFER,sUboBlkId,sUboBuff);
                     stereo_ptr = glMapBufferRange(GL_UNIFORM_BUFFER,0,sizeof(mode),
                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                     memcpy(stereo_ptr,&mode,sizeof(mode));
                     glUnmapBuffer(GL_UNIFORM_BUFFER);
                     glDrawArraysInstanced(GL_TRIANGLES,0,sphereSize, iter->second[currCycle].cellSize[CONTROL_PTS]);
                  }

                  if (iter->second[currCycle].cellSize[STIM_PTS])
                  {
                     mode=STIM_PART;
                     glBindBufferBase(GL_UNIFORM_BUFFER,sUboBlkId,sUboBuff);
                     stereo_ptr = glMapBufferRange(GL_UNIFORM_BUFFER,0,sizeof(mode),
                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                     memcpy(stereo_ptr,&mode,sizeof(mode));
                     glUnmapBuffer(GL_UNIFORM_BUFFER);

                     glDrawArraysInstanced(GL_TRIANGLES,0,sphereSize, iter->second[currCycle].cellSize[STIM_PTS]);
                  }
                  glBindBufferBase(GL_UNIFORM_BUFFER,sUboBlkId,sUboBuff);
                  stereo_ptr = glMapBufferRange(GL_UNIFORM_BUFFER,0,sizeof(stereoMode),
                                            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                  memcpy(stereo_ptr,&stereoMode,sizeof(stereoMode));
                  glUnmapBuffer(GL_UNIFORM_BUFFER);
                  break;
               case CONTROL_ONLY:
               case CONTROL_STEREO:
                  if (iter->second[currCycle].cellSize[CONTROL_PTS])
                     glDrawArraysInstanced(GL_TRIANGLES,0,sphereSize, iter->second[currCycle].cellSize[CONTROL_PTS]);
                  break;

               case STIM_ONLY:
               case STIM_STEREO:
                  if (iter->second[currCycle].cellSize[STIM_PTS])
                     glDrawArraysInstanced(GL_TRIANGLES,0,sphereSize, iter->second[currCycle].cellSize[STIM_PTS]);
                  break;

               case DELTA_ONLY:
               case DELTA_STEREO:
                  if (iter->second[currCycle].cellSize[DELTA_PTS])
                     glDrawArraysInstanced(GL_TRIANGLES,0,sphereSize, iter->second[currCycle].cellSize[DELTA_PTS]);
                  break;

            }
         }
      }
   }
   err_chk = glGetError();
   if (err_chk != 0)
      cout << "paint error 2 is: " << err_chk << endl;

       // second pass, sort the lists and render
   glFlush();                                      // tell the above to complete
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // then wait for it
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   glUseProgram(finalRenderProg);
   glBindVertexArray(showVao);
   glDrawArrays(GL_TRIANGLE_STRIP,0,4);

   err_chk = glGetError();
   if (err_chk != 0)
      cout << "paint error 3 is: " << err_chk << endl;

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0); // nothing in scope
   glBindVertexArray(0);
   glUseProgram(0);
   glBindTexture(GL_TEXTURE_2D,0);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

//clock_gettime(CLOCK_MONOTONIC,&frameEnd);
//interval = (frameEnd.tv_sec*1000*1000*1000 + frameEnd.tv_nsec) - (frameStart.tv_sec*1000*1000*1000 + frameStart.tv_nsec);
//cout << "Repaint time: " << (unsigned long)(interval/(1000.0 * 1000.0)) << endl;
}

// new window size or exposure
void BrainStemGL::resizeGL(int width, int height)
{
   float zd = 2 * radius;
   float neardist = (zd - radius) - 10; // clips sometimes without a fudge factor
   float fardist = (zd + radius) + 10;

   float left, right, top, bottom;
//cout << "radius: " << radius << " zd: " << zd << " near: " << neardist << "  far: " << fardist << endl;
   if (height < width/stereoViewports)    // 1 or 2
   {
      bottom = -radius;
      top = radius;
      right = radius * (width/stereoViewports) / height;
      left = -right;
   }
   else 
   {
      left = -radius;
      right = radius;
      top = radius * height / (width/stereoViewports);
      bottom = -top;
   }

   viewPortW = width/2.0;  // for stereo view
   viewPortH = height;
   viewMat = glm::translate(glm::mat4(1.0f),glm::vec3(0, 0, -zd)); // default
   if (showOrtho)
      projMat = glm::ortho(left,right,bottom,top,neardist,fardist); 
   else
      projMat = glm::perspective(glm::radians(FOV),double(width)/height,0.1,(double)zd+10);
}


/* We get here when the animation timer fires
*/
void BrainStemGL::spinAgain()
{
   double step = 1;
   double spinFPS = min((1000.0/timerSpin),60.0);
   double twinkleFPS = min((1000.0/timerTwinkle),60.0);

   if (timerSpin <= timerTwinkle)
   {
      if (recordingMovie)
      {
         if (spinFPS < 24)
            step = spinFPS/movieFPS;
         else
            step = 1;   // spin drives the ticks
      }
      else // spin drives the ticks
      {
         step = 1;
      }
   }
   else // twink drives ticks
   {
      if (recordingMovie)
      {
         if (twinkleFPS < 24)   // then spin is <= to 24
            step = twinkleFPS/movieFPS;
         else
            step = double(timerTwinkle)/timerSpin;  // simulate n twinks for 1 spin
      }
      else 
         step = double(timerTwinkle)/timerSpin; 
   }
   angle += angleDir * step;
   if (angle >= 360.0 || angle <= -360.0)
      angle = 0.0;
   update();

   if (Debug && (!twinkleOn || numBins == 0))
   {
      QString update;
      QTextStream updateInfo(&update);
      if (!recordingMovie)
         updateInfo << qSetFieldWidth(3) << "fps: " << movieFPS;
      printInfo(update);
   }
}

void BrainStemGL::rotateY()
{
   view_roty += 90.0;
   if (view_roty >= 360.0)
      view_roty -= 360.0;
   makeCurrent();
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}

void BrainStemGL::rotateX()
{
   view_rotx += 90.0;
   if (view_rotx >= 360.0)
      view_rotx -= 360.0;
   makeCurrent();
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}

void BrainStemGL::rotateZ()
{
   view_rotz += 90.0;
   if (view_rotz >= 360.0)
      view_rotz -= 360.0;
   makeCurrent();
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}

void BrainStemGL::updateRegions(BrainSel& sel)
{
   structsFirst.clear();
   structsCount.clear();

   for (BrainSelIter iter = sel.begin(); iter != sel.end(); ++iter)
   {
      structsFirst.push_back(selStructs[*iter].first);
      structsCount.push_back(selStructs[*iter].count);
   }
   update();
}



void BrainStemGL::toggleStereo(STEREO_MODE mode)
{
   stereoMode = mode;
   if (mode == CONTROL_STEREO || mode == CTL_STIM_PAIR || mode == STIM_STEREO || mode == DELTA_STEREO)
      stereoViewports = 2;
   else
      stereoViewports = 1;

   GLfloat box_aspect = (1.0-(1.0/((GLfloat)DRAWBOX_W/DRAWBOX_H)))/stereoViewports;
   drawBox[1] = drawBox[4] = drawBox[16] = box_aspect;

   makeCurrent();
   glBindBufferBase(GL_UNIFORM_BUFFER,sUboBlkId,sUboBuff);
   void* stereo_ptr = glMapBufferRange(GL_UNIFORM_BUFFER,0,sizeof(stereoMode),
                             GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
   memcpy(stereo_ptr,&stereoMode,sizeof(stereoMode));
   glUnmapBuffer(GL_UNIFORM_BUFFER);
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}

void BrainStemGL::showCtlStim(STEREO_MODE mode)
{
   toggleStereo(mode);
}


void BrainStemGL::reset()
{
   view_rotx = 0.0;
   view_roty = 0.0;
   view_rotz = 0.0;
   view_trx = 0.0;
   view_try = 0.0;
   view_trz = 0.0;
   xmid = xmidsave;
   ymid = ymidsave;
   zmid = zmidsave;
   angle = 0;
   radius = radius0;
   projMat = glm::mat4(1.0f);
   modelMat[0] = glm::mat4(1.0f);
   modelMat[1] = glm::mat4(1.0f);
   viewMat = glm::mat4(1.0f);
   phrenicLinePos = PHRENIC_I_START;

   makeCurrent();  // done for us on callbacks, but need to do it explicitly here
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();    // request a repaint
}

void BrainStemGL::doForeGround(int value)
{
   outlineColorVal[0] = outlineColorVal[1] = outlineColorVal[2] = value / 255.0;
   makeCurrent();
   glUseProgram(outlineProg);
   glUniform4fv(outlineColor,1,glm::value_ptr(outlineColorVal));
   glUseProgram(0);
   doneCurrent();
   update();
}

void BrainStemGL::doBackGround(int value)
{
   backColor = value/255.0;
   if (finalRenderProg)
   {
      makeCurrent();  // done for us on callbacks, but need to do it explicitly here
      glClearColor(backColor,backColor,backColor,1.0);
      glUseProgram(finalRenderProg);
      glUniform4f(1,backColor,backColor,backColor,1.0);
      glUseProgram(0);
      doneCurrent();
      clearInfo();
   }
}

void BrainStemGL::doDelayChanged(int value)
{
   if (value >= 0)
      angleDir = -1;
   else
      angleDir = 1;
   timerSpin = abs(value);
}

void BrainStemGL::doToggleAxes(bool onoff)
{
   showAxes = onoff;
   update();
}

void BrainStemGL::doToggleOutlines(bool onoff)
{
   showOutlines = onoff;
   update();
}

void BrainStemGL::spinToggle(bool onoff)
{
   if (!onoff)
      clearInfo();
   spinOn = onoff;
}

void BrainStemGL::doToggleColorCycling(bool val)
{
   twinkleOn = val;
   currCycle=0;
   clearInfo();
}

void BrainStemGL::doTwinkleChanged(int val)
{
   timerTwinkle = val;
}

// do next step in color cycling, with wrap-around
void BrainStemGL::twinkleAgain()
{
   QString update;
   QTextStream updateInfo(&update);

   if (numBins)
   {
      if (++currCycle > numBins) // for next time
         currCycle = 1;  // skip 1st color, its the default color
        // round off error at boundry preventers:
      if (currCycle == 1)
         phrenicLinePos = PHRENIC_I_START;
      else if (currCycle == numBins/2+1)
         phrenicLinePos = PHRENIC_E_START;
      else if (currCycle == numBins)
         phrenicLinePos = PHRENIC_E_END;
      else
         phrenicLinePos = PHRENIC_I_START + currCycle*phrenicStep;

      if (Debug && recordingMovie)
         updateInfo << qSetFieldWidth(3) << "fps: " << movieFPS;

      if (!recordingMovie)
         updateInfo << qSetFieldWidth(0) << "  bin: " << qSetFieldWidth(3) << currCycle;
      if (Debug)
         updateInfo << qSetFieldWidth(0) << "  phrenic:";
      printInfo(update);
   }
}

// On transition to/from single shade color mode,
// load the appropriate color table to the fragment shader
void BrainStemGL::singleTwinkle(bool val)
{
   twinkleMode = val;
   makeCurrent();
   updateCellProg();
   doneCurrent();
   update();
}

void BrainStemGL::singleStep(bool val)
{
   singleMode = val;
   if (singleMode)
   {
      twinkleOn = true;
      update();
   }
}

void BrainStemGL::doHideCells(bool onoff)
{
   hideCells = onoff;
   if (cellProg)
   {
      makeCurrent();
      glUseProgram(cellProg);
      glUniform1i(5,onoff);
      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doPtSizeChanged(int size)
{
   ptSize = abs(size);
   if (cellProg)
   {
// cout << "sphere size: " << ptSize << endl;
      makeCurrent();
      glUseProgram(cellProg);
      glUniform1i(4,ptSize);
      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doSkinToggle(bool state)
{
   skinOn = state;
   update();
}


void BrainStemGL::doSkinTransparencyChanged(int trans)
{
   skinTrans = trans / 100.0;
   if (sort_skinProg)
   {
      makeCurrent();
      glUseProgram(sort_skinProg);
      glUniform1f(1,skinTrans);
      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doCellTransparencyChanged(int trans)
{
   cellTrans = trans / 100.0;
   if (sort_skinProg)
   {
      makeCurrent();
      glUseProgram(cellProg);
      glUniform1f(14,cellTrans);
      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doRegionTransChanged(int trans)
{
   regionTrans = trans / 100.0;
   if (structProg)
   {
      makeCurrent();
      glUseProgram(structProg);
      glUniform1f(1,regionTrans);
      glUseProgram(0);
      doneCurrent();
      update();
   }
}


void BrainStemGL::movieNew(const QString& avi, const QString& tmpdir)
{
   aviName = avi;
   tmpPath = tmpdir;
}

void BrainStemGL::movieStart(double fps)
{
   currFrame = 0;
   recordingMovie = true;
   movieFPS = fps;
}

// Have a new frame, save it to tmp dir as png file
int BrainStemGL::movieFrame()
{
   QString pngFile;
   QTextStream formatName(&pngFile);
   
   formatName << tmpPath << qSetFieldWidth(0) << "/frame" << right << qSetPadChar('0') << qSetFieldWidth(6) << currFrame << qSetFieldWidth(0) << ".png";
   ++currFrame;
   QFile file(pngFile);
   file.open(QIODevice::WriteOnly);

   makeCurrent();
   QImage  aFrame = grabFramebuffer();
   aFrame.save(&file, "PNG");
   doneCurrent();
   return currFrame;
}

void BrainStemGL::moviePause()
{
   recordingMovie = !recordingMovie;
}

// create the movie files.
// Return true if okay, 0 on error
bool BrainStemGL::movieStop()
{
   QString cmd;
   QTextStream makeAvi(&cmd);
   int stat;
   QString movname;
   QString msg;
   int output_fps = 24;   // save at least this many fps
   
   recordingMovie = false;
   if (movieFPS > 24)
      output_fps = movieFPS;

   QTextStream movieinfo(&msg);
   movieinfo << "effective fps: " << movieFPS << endl
             << "output fps: " << output_fps << endl;
    // make avi
   cmd.clear();
   movname = aviName + ".avi";
   makeAvi << "ffmpeg -y -nostdin -loglevel error"
           << " -framerate " << movieFPS
           << " -i " << tmpPath << "/frame%06d.png"
           << " -r " << output_fps 
           << " -pix_fmt bgr24 -vcodec zlib " 
           << movname 
           << " > movie.log";
    stat = system(cmd.toLatin1().constData());

     // make wmv
   cmd.clear();
   movname = aviName + ".wmv";
   makeAvi << "ffmpeg -y -nostdin -loglevel error" 
           << " -framerate " << movieFPS 
           << " -i " << tmpPath << "/frame%06d.png"
           << " -r " << output_fps 
           << " -pix_fmt bgr24 -b:v 8192K"
           << " -c wmv2 " << movname 
           << " > movie.log";
   stat = stat + system(cmd.toLatin1().constData());

   emit(chatBox(msg));
   cmd.clear();

   if (stat == 0 || Debug)  // leave files around for debugging on failure
   {
#ifdef Q_OS_LINUX
      makeAvi << "rm -rf " << tmpPath;
#else
      makeAvi << "RMDIR /S /Q " << tmpPath;
#endif
      stat = system(cmd.toLatin1().constData());
   }
   return !stat;
}

// Save the current frame to a .png file
// Assumes all checking done by caller, we expect to succeed.
void BrainStemGL::saveFrame(QString& pngName, QString& pdfName)
{
   makeCurrent();
   QImage aFrame = grabFramebuffer();

   if (pngName.length())
   {
      QFile file(pngName);
      file.open(QIODevice::WriteOnly);
      aFrame.save(&file, "PNG");
      file.close();
   }

   if (pdfName.length())
   {
      QPrinter printer(QPrinter::HighResolution);
      printer.setOutputFormat(QPrinter::PdfFormat);
      printer.setOutputFileName(pdfName);
         // fit to page width (assumes square pixels)
      double Scale = (printer.width() / aFrame.width());
      QPainter painter;
      painter.begin(&printer);
      painter.scale(Scale,Scale);
      painter.drawImage(QPoint(0,0),aFrame);
      painter.end();
   }
   doneCurrent();
}

void BrainStemGL::doAmbient(int val)
{
   ambient = val;

   if (sort_skinProg)
   {
      makeCurrent();
      glm::vec3 amb(ambient/100.0,ambient/100.0,ambient/100.0);
      glUseProgram(sort_skinProg);
      glUniform3fv(2,1,glm::value_ptr(amb));

      glUseProgram(cellProg);
      glUniform3fv(11,1,glm::value_ptr(amb));

      glUseProgram(structProg);
      glUniform3fv(2,1,glm::value_ptr(amb));

      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doDiffuse(int val)
{
   diffuse = val;

   if (sort_skinProg)
   {
      makeCurrent();
      glm::vec3 dif(diffuse/100.0,diffuse/100.0,diffuse/100.0);
      glUseProgram(sort_skinProg);
      glUniform3fv(3,1,glm::value_ptr(dif));

      glUseProgram(cellProg);
      glUniform3fv(12,1,glm::value_ptr(dif));

      glUseProgram(structProg);
      glUniform3fv(3,1,glm::value_ptr(dif));

      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doLightDistX(int val)
{
   dX = val/10.0;
   if (sort_skinProg)
   {
      glm::vec3 dist(dX,dY,dZ);

      makeCurrent();
      glUseProgram(sort_skinProg);
      glUniform3fv(4,1,glm::value_ptr(dist));

      glUseProgram(cellProg);
      glUniform3fv(13,1,glm::value_ptr(dist));

      glUseProgram(structProg);
      glUniform3fv(4,1,glm::value_ptr(dist));

      glUseProgram(0);
      doneCurrent();
      update();
   }
}

void BrainStemGL::doLightDistY(int val)
{
   dY = val/10.0;
   if (sort_skinProg)
   {
      glm::vec3 dist(dX,dY,dZ);

      makeCurrent();
      glUseProgram(sort_skinProg);
      glUniform3fv(4,1,glm::value_ptr(dist));

      glUseProgram(cellProg);
      glUniform3fv(13,1,glm::value_ptr(dist));

      glUseProgram(structProg);
      glUniform3fv(4,1,glm::value_ptr(dist));

      glUseProgram(0);
      doneCurrent();
      update();
   }
}


void BrainStemGL::doLightDistZ(int val)
{
   dZ = val / 10.0;

   if (sort_skinProg)
   {
      glm::vec3 dist(dX,dY,dZ);
      makeCurrent();
      glUseProgram(sort_skinProg);
      glUniform3fv(4,1,glm::value_ptr(dist)); // see glsl code for uniform magic nums

      glUseProgram(cellProg);
      glUniform3fv(13,1,glm::value_ptr(dist));

      glUseProgram(structProg);
      glUniform3fv(4,1,glm::value_ptr(dist));

      glUseProgram(0);
      doneCurrent();
      update();
   }
}


// tan color
void BrainStemGL::doSurfaceT()
{
   makeCurrent();
   glUseProgram(sort_skinProg);
   glUniform1i(5,0);
   glUseProgram(0);
   doneCurrent();
   update();
}

// white color
void BrainStemGL::doSurfaceW()
{
   makeCurrent();
   glUseProgram(sort_skinProg);
   glUniform1i(5,1);
   glUseProgram(0);
   doneCurrent();
   update();
}


void BrainStemGL::doOrtho()
{
   showOrtho = true;
   makeCurrent();  // done for us on callbacks, but need to do it explicitly here
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}


void BrainStemGL::doPerspec()
{
   showOrtho = false;
   makeCurrent();  // done for us on callbacks, but need to do it explicitly here
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}

void BrainStemGL::doFov(int value)
{
   FOV = value;
   makeCurrent();
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
   update();
}

// if you add stuff, update version
void BrainStemGL::saveFig(QTextStream& out)
{
   out << "version: " << figSettingsVer << endl;
   out << "rotx: " << view_rotx << endl;
   out << "roty: " << view_roty << endl;
   out << "rotz: " << view_rotz << endl;
   out << "trx: " << view_trx << endl;
   out << "try: " << view_trz << endl;
   out << "trz: " << view_trz << endl;
   out << "angle: " << angle << endl;
   out << "radius: " << radius << endl;
}

// this expects the order from the saveFig function above
// input: stream. We read first bit, leave location at
// next part of stream for caller.
// values not in controls set here
// values in controls set in brain_impl
void BrainStemGL::loadFig(QTextStream& in)
{
   QString dummy;
   int version;

   in >> dummy >> version;
   if (version == 1)
   {
      in >> dummy >> view_rotx;
      in >> dummy >> view_roty;
      in >> dummy >> view_rotz;
      in >> dummy >> view_trx;
      in >> dummy >> view_try;
      in >> dummy >> view_trz;
      in >> dummy >> angle;
      in >> dummy >> radius;
   }
   makeCurrent();
   resizeGL(geometry().width(),geometry().height());
   doneCurrent();
}


void BrainStemGL::chkcomp(GLuint sh, const char* name)
{
   GLchar log[2048];
   GLsizei len;
   log[0] = 0;
   int params = -1;
   QString msg;

   glGetShaderiv(sh, GL_COMPILE_STATUS, &params);
   if (params != GL_TRUE) 
   {
      QTextStream(&msg) << "ERROR: GL shader " << name << " did not compile.";
      glGetShaderInfoLog(sh,2047,&len,log);
      QTextStream(&msg) << log << endl;
      emit(chatBox(msg));
   }
//   else
//   {
//      QTextStream(&msg) << name << " compiled ok.";
//      emit(chatBox(msg));
//   }
}

void BrainStemGL::chklink(GLuint ln, const char*name)
{
  int params = -1;
  GLchar log[2048];
  GLsizei len;
  log[0] = 0;
  QString msg;

   glGetProgramiv(ln, GL_LINK_STATUS, &params);
   if (params != GL_TRUE) 
   {
      QTextStream(&msg) << "ERROR: could not link shader program GL "  << name;
      glGetProgramInfoLog(ln,2047,&len,log);
      QTextStream(&msg) << log << endl;
      emit(chatBox(msg));
   }
//   else
//   {
//      QTextStream(&msg) << name << " linked ok.";
//      emit(chatBox(msg));
//   }
}


void BrainStemGL::dumpmat4(glm::mat4 m)
{
   for (int i=0; i < 4 ; ++i)
   {
      for (int j=0; j < 4; ++j)
         cout << m[j][i] << "        ";
      cout << endl;
   }
   cout << endl;
}


