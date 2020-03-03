#ifndef BRAINSTEMGL_H
#define BRAINSTEMGL_H
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


#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTextStream>
#include <QTextEdit>
#include <QCheckBox>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QScreen>
#include <QOpenGLFunctions_4_3_Core>
#include <QtPrintSupport/QPrinter>
#include <QOpenGLDebugMessage>
#include <QOpenGLDebugLogger>

#define GLM_FORCE_CXX1Y
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <glm/exponential.hpp>
#include <glm/gtx/color_space.hpp>

#include <ios>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <limits>
#include <memory>
#include <stdlib.h>
#ifdef Q_OS_LINUX
#include <time.h>
#endif

using namespace std;

#include "brainstem.h"

const GLenum PrintTexture = GL_TEXTURE0;
const GLenum ListTexture = GL_TEXTURE1;

using glCTH = struct glCTHStruct {GLuint vao; size_t cellSize[NUM_PT_LISTS];}; 
using cellList = vector<GLint>;
using cellListIter = cellList::iterator;

using vaoCellList = map<int, std::vector<glCTH>>;
using vaoCellListIter = vaoCellList::iterator;
using vaoCellInsert = pair<int, glCTH>;

using colorBright = vector<glm::vec4>;
using colorBrightIter = colorBright::iterator;

// for create lists of data in a format we can
// send to openGL
using ptCoords = vector<glm::vec3>;
using ptCoordsIter = ptCoords::iterator;
using colorIdx = vector<vector<GLint> >; 
using colorIdxIter = colorIdx::iterator; 

class oneStruct
{
   public:
      unsigned int first;
      unsigned int count;
};


const int MAX_FB_WIDTH=2048;  // 4K displays or many monitors overflow
const int MAX_FB_HEIGHT=2048; // buffers, clip physical screen to this.

// shaders use this for building OIT linked lists
struct oitNode {
   glm::vec4 color;
   GLfloat depth;
   GLuint next;
};
const GLint OIT_NODE_SIZE=5*sizeof(GLfloat)+sizeof(GLuint); // glsl thinks above struct 
                                                            // is this big
const GLuint MAX_OIT_NODES = 27 * MAX_FB_WIDTH*MAX_FB_HEIGHT;  // max nodes

using brainStructs = vector<oneStruct>; 
using structuresFirst = vector<GLint>;
using structuresCount = vector<GLsizei>;

class BrainStem;

class BrainStemGL : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core
{
   Q_OBJECT

   friend BrainStem;

   public:
      explicit BrainStemGL(QWidget *parent = nullptr);
      virtual ~BrainStemGL();

      QByteArray saveSettings();
      void loadSettings(QByteArray);

   signals:
      void chatBox(QString);
      void movieUpd(QString);

   protected:
      void initializeGL();
      void paintGL();
      void resizeGL(int width, int height);
      void mousePressEvent(QMouseEvent*);
      void mouseMoveEvent(QMouseEvent*);
      void wheelEvent(QWheelEvent*);

   public slots:

   protected slots:

   private:
      void extremes();
      void cells();
      void outlines();
      void axes();
      void printBox();
      void skin();
      void sphere();
      void stemStructs();
      void oit();
      void printInfo(QString&);
      void clearInfo();
      void reset();
      void doToggleAxes(bool);
      void doForeGround(int);
      void doBackGround(int);
      void doDelayChanged(int);
      void doToggleOutlines(bool);
      void spinToggle(bool);
      void spinAgain();
      void saveFrame(QString&,QString&);
      void doAmbient(int);
      void doDiffuse(int);
      void movieNew(const QString&, const QString&);
      void movieStart(double);
      int movieFrame();
      void moviePause();
      bool movieStop();
      void rotateX();
      void rotateY();
      void rotateZ();
      void toggleStereo(STEREO_MODE);
      void updateCells(bool,bool,bool,vector<int>&,vector<int>&,cellArray&,ClustRGB&,bool);
      void createShades(ClustRGB&);
      void updateCellProg();
      void doToggleColorCycling(bool);
      void doTwinkleChanged(int);
      void twinkleAgain();
      int  newFrame();
      void singleTwinkle(bool);
      void singleStep(bool);
      void doHideCells(bool);
      void doCellTransparencyChanged(int);
      void doPtSizeChanged(int);
      void doSkinToggle(bool);
      void doSkinTransparencyChanged(int);
      void closeFile();
      void updateRegions(BrainSel&);
      void doRegionTransChanged(int);
      void doLightDistX(int);
      void doLightDistY(int);
      void doLightDistZ(int);
      void doSurfaceT();
      void doSurfaceW();
      void clearClusts();
      void applyPrefs();
      void doOrtho();
      void doPerspec();
      void doFov(int);
      void showCtlStim(STEREO_MODE);
      void saveFig(QTextStream &);
      void loadFig(QTextStream &);

        // debugging
      void chkcomp(GLuint, const char*);
      void chklink(GLuint, const char*);
      void dumpmat4(glm::mat4);
      void static glDebugMsgs(const QOpenGLDebugMessage &);

      float rotx = 0.0;
      float roty = 0.0;
      float tx = 0.0;
      float ty = 0.0;
      int lastx = 0.0;
      int lasty = 0.0;
      GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;
      GLfloat view_trx = 0.0, view_try = 0.0, view_trz = 0.0;
      GLfloat angle = 0.0;
      bool spin = false;
      bool showOutlines = false;
      bool showAxes = true;
      double backColor=0.8;
      bool showOrtho = true;
      double FOV = 20.0;

        // lighting
      GLfloat ambient= 0;
      GLfloat diffuse= 100;
      GLfloat dX= 0.0;
      GLfloat dY= 0.0;
      GLfloat dZ= 1.0;

        // viewport
      float xlo= 0.0, xhi= 0.0, ylo= 0.0, yhi= 0.0, zlo= 0.0, zhi= 0.0;
      float xmid= 0.0, ymid= 0.0, zmid= 0.0, radius = -1.0, radius0= 0.0;
      float xmidsave= 0.0, ymidsave= 0.0, zmidsave= 0.0;

       // animation
      int angleDir = -1;
      bool spinOn=false;
      bool twinkleOn=false;

       // movie 
      int currFrame = 0;
      bool recordingMovie = 0;
      QFile movie;
      QString aviName;
      QString tmpPath;
      struct timespec frameStart;
      struct timespec frameEnd;
      double movieFPS;
      unsigned int timerSpin;
      unsigned int timerTwinkle;
      size_t currCycle=0;

      size_t numBins=0;
      bool   twinkleMode=false;
      bool   singleMode=false;
      bool   hideCells=false; 
      int    ptSize=15;

      int stereoViewports=1;
      float viewPortW;
      float viewPortH;

      // for vertex sorting so transparency works
      GLuint headPointerTex;
      GLuint headPointerInitVal;
      GLuint atomicCounterBuff;
      GLuint linkedListBuff, linkedListTex;
      GLuint sortVs, sortGs, sortFs, sort_skinProg = 0;
      GLuint finalRenderVs, finalRenderGs, finalRenderFs, finalRenderProg = 0;
      GLuint showVao, showVbo;

         // outlines VAO, VBO 
      GLuint outlinesVao = 0;
      GLuint outlinesVbo = 0;
      GLuint outlinesIdx = 0;
      GLuint outlinesIdxBuf = 0;
      GLuint outlinesIdxSize;
      GLint  outlineColor = 1.0;  // uniforms
      GLuint outlineFShader;
      glm::vec4 outlineColorVal = glm::vec4(1.0, 1.0, 1.0, 1.0);

       // UBO for common transform matrix
      GLuint vUbo;
      GLuint vUboBlkId = 1;
      GLuint vUboBuff;

       // UBO for max OIT buffer size
      GLuint nodeUbo;
      GLuint nodeUboBlkId = 2;
      GLuint nodeBuff;

       // UBO for common transform matrix for lighting normals
      GLuint nUbo;
      GLuint nUboBlkId = 3;
      GLuint nUboBuff;
      GLuint stereoMode=0;

        // UBO for stereo mode
      GLuint sUbo;
      GLuint sUboBlkId = 4;
      GLuint sUboBuff;

        // testing, how many frags stack up?
    GLuint nodeId = 9;
    GLuint nodeCount;

          // some progs use the vertex shader
      GLuint vertVShader;
      GLuint vertGShader;
      GLuint vertFShader;
      GLuint outlineProg=0;

         // axes
      GLuint axesVao = 0;
      GLuint axesVbo = 0;
      GLuint axesVShader;
      GLuint axesFShader;
      GLuint axesSize = 0;
      GLuint axesProg=0;
      GLuint axesColor;
      glm::vec4 axesColorVec = glm::vec4(0.0, 1.0, 1.0, 1.0);

        // transforms, etc.
      glm::mat4 mvpMat[2];
      glm::mat4 mvMat[2];
      glm::mat4 projMat, viewMat, modelMat[2];
      size_t mvpSize;
      size_t mvSize;

          // cells are complicated due to color cycling
      vaoCellList cellsVao;
      colorBright colorTab;
      colorBright oneShadeTab;
      colorBright deltaShadeTab;
      GLuint *vboPointList=nullptr; 
      GLuint *vboColorList=nullptr; 
      vector<int>onOff;
      GLuint cellVShader=0;
      GLuint cellGShader=0;
      GLuint cellFShader=0;
      GLuint cellProg=0;
      GLuint colorTabVbo;
      GLuint deltaTabVbo;
      GLfloat cellTrans=1.0;
      bool haveDelta=false;

        // ui box with text & etc as textures
      GLuint printVao;
      GLuint printVbo;
      GLuint texVbo; 
      GLuint printProg=0; 
      GLuint printVShader; 
      GLuint printFShader;
      GLuint printTShader;
      GLuint printText;
      GLuint sizeText;
      QImage phrenicImageG;
      QImage noPhrenicImage;
      int phrenicLinePos;
      double phrenicStep;
      bool printClear = false;

        // skin
      GLuint skinVao;
      GLuint skinVbo;
      GLuint normVbo;
      GLuint skinProg=0;
      GLuint skinVShader; 
      GLuint skinGShader;
      GLuint skinFShader;
      GLuint skinSize = 0;
      bool skinOn = true;
      GLfloat skinTrans=0.7;

        // sphere
      GLuint sphereVao;
      GLuint sphereVertVbo;
      GLuint sphereNormVbo;
      GLuint sphereProg=0;
      GLuint sphereVShader; 
      GLuint sphereFShader;
      GLuint sphereSize = 0;
      ptCoords sphereV, sphereN;

        // brain structures
      GLuint structVao;
      GLuint structVbo;
      GLuint structNormVbo;
      GLuint structProg=0;
      GLuint structVShader; 
      GLuint structGShader;
      GLuint structFShader;
      GLfloat regionTrans=0.4;
      brainStructs selStructs; 
      structuresFirst structsFirst;
      structuresCount structsCount;

      GLuint structSize = 0;
      bool structOn = true;
      GLfloat structTrans=1.0;
      bool havePhrenic = true;

      QByteArray userPrefs;
};

#endif // BRAINSTEMGL_H
