#ifndef BRAINSTEM_H
#define BRAINSTEM_H

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

#include <QMainWindow>
#include <QFileDialog>
#include <QTimer>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QStringList>
#include <QStringListModel>
#include <QFile>
#include <QSettings>
#include <QSignalMapper>
#include <QSlider>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTemporaryDir>
#include <QWhatsThis>

#include <map>
#include <set>
#include <vector>
#include <tuple>
#include <unistd.h>
#include <ios>
#include <iostream>

using namespace std;

const QString csvFile("CSV");
const QString dxFile("DX");

const int DELTA_FLAG = -1;  // several places use this to detect delta CTHs.

// This is the output of period2str.m.  If it changes, this
// will need to be changed.
// It is not impossible, but kind of a pain to share a src file
// between two packages and two languages.
enum CTL_STIMS {CONTROL_PERIOD=0, CTL_CCO2, STIM_CCO2, CTL_VCO2, STIM_VCO2, CTL_TBCGH, STIM_TBCGH, CTL_LARCGH, STIM_LARCGH, CS_DELTA,CTL_SWALLOW1,STIM_SWALLOW1,CTL_LAREFLEX,STIM_LAREFLEX,MAX_PERIODS};
const QString PeriodNames[]={"CONTROL","CCO2CTL","CCO2STIM","VCO2CTL","VCO2STIM","TBCGHCTL","TBCGHSTIM","LARCGHCTL","LARCGHSTIM","CS-DELTA","CONTROL","SWALLOW1STIM","CONTROL","LAREFLEXSTIM","UNKN"};
enum CELL_COLORS {CONTROL_COLORS,STIM_COLORS,DELTA_COLORS,NUM_CELL_COLORS};
enum CELL_PTS {CONTROL_PTS,STIM_PTS,DELTA_PTS,NUM_PT_LISTS};

// strings used in combo box
const QString selCONTROL("Control");
const QString selSTIM("Stim");
const QString selBOTH("Ctl/Stim");
const QString selDELTA("CS-Delta");
const QString selCTLSWALLOW1("Ctl Swallow1");
const QString selSTIMSWALLOW1("Stim Swallow1");
const QString selLAREFLEX("Ctl Lareflex");
const QString selLAREFLEXSTIM("Stim Lareflex");
enum SHOW_CELLS {NO_CELLS=0,CONTROL_CELLS,STIM_CELLS,BOTH_CELLS,DELTA_CELLS};
enum STEREO_MODE {CONTROL_ONLY=0,CONTROL_STEREO,CTL_STIM_PAIR,CTL_PART,STIM_PART,STIM_ONLY,STIM_STEREO,DELTA_ONLY,DELTA_STEREO};
/*
CONTROL_ONLY     0
CONTROL_STEREO   1
CTL_STIM_PAIR    2
CTL_PART         3
STIM_PART        4
STIM_ONLY        5
STIM_STEREO      6
DELTA_ONLY       7
DELTA_STEREO     8
*/

// given rgb color, find cluster #
class rgbLookUp
{
   public:
      rgbLookUp(): r(0.0),g(0.0),b(0.0) {}
      rgbLookUp(const rgbLookUp& rgb0):r(rgb0.r),g(rgb0.g),b(rgb0.b) {}
      virtual ~rgbLookUp() {}

      int rAsInt() { return r * 255; }
      int gAsInt() { return g * 255; }
      int bAsInt() { return b * 255; }

      double r;
      double g;
      double b;
};

// for sorting/comparing rgb tuples
class CompRGB
{
   public:
      bool operator() (rgbLookUp const& lhs, rgbLookUp const& rhs) const
      { return tie(lhs.r,lhs.g,lhs.b) < tie(rhs.r,rhs.g,rhs.b); }
};


using RGBClust = map <rgbLookUp,int,CompRGB>;
using RGBClustIter = RGBClust::iterator;
using RGBInsert  = pair<RGBClustIter,bool>;

// Given cluster #, find rgb color, the reverse of the above
using ClustRGB = map <int,rgbLookUp>;
using ClustRGBIter = ClustRGB::iterator;



// Cells in different views should appear in same location. Complicated
// by the fact we jitter cells on same (x,y,z) so we can see them.
class expCell
{
   public:
      expCell():expidx(0),mchan(0) {}
      expCell(const expCell& cell0):expidx(cell0.expidx),mchan(cell0.mchan) {}

      int expidx;
      int mchan;
};


class xyzCoords
{
   public:
      xyzCoords():x(0.0),y(0.0),z(0.0) {}
      xyzCoords(double x0,double y0,double z0):x(x0),y(y0),z(z0) {}
      xyzCoords(const xyzCoords& xyz0):x(xyz0.x),y(xyz0.y),z(xyz0.z) {}
      virtual ~xyzCoords() {}

      double x;
      double y;
      double z;
};

// for sorting/comparing xyz tuples
class CompXYZ
{
   public:
      bool operator() (xyzCoords const& lhs, xyzCoords const& rhs) const
      {
         return tie(lhs.x,lhs.y,lhs.z) < tie(rhs.x,rhs.y,rhs.z); 
      }
};

using coordKey = tuple<double,double,double>;

class aCell
{
   public:
      aCell():expname(""),chan(0) {}
      aCell(QString name, int ch):expname(name),chan(ch) {}
      aCell(const aCell& p0):expname(p0.expname),chan(p0.chan) {}
      QString expname;
      int chan;
};

class compCell {
   public:
      bool operator() (aCell const& lhs, aCell const& rhs) const
      {
         return tie(lhs.expname,lhs.chan) < tie(rhs.expname,rhs.chan); 
      }
};

using aCellKey = pair<QString,int>;

class jitter
{
   public:
      jitter():jit_x(0),jit_y(0),jit_z(0) {}
      jitter(double x,double y,double z):jit_x(x),jit_y(y),jit_z(z) {}
      jitter(const jitter& jm):jit_x(jm.jit_x),jit_y(jm.jit_y),jit_z(jm.jit_z) {}
      double jit_x;
      double jit_y;
      double jit_z;
};

using cellMap = map <aCell,jitter,compCell>;
using cellPtMap = map <xyzCoords,cellMap,CompXYZ>;
using cellMapIter = cellMap::iterator;
using cellPtMapIter = cellPtMap::iterator;

// DX files have no experiments or periods, just colors and locations
using dxCellPtMap = set <xyzCoords,CompXYZ>;
using dxCellPtMapIter = dxCellPtMap::iterator;

// set of experiment names
using expNameSet = map <QString,int>;
using expNameIter = expNameSet::iterator;
using expNameInsert = pair<expNameIter,bool>;

using CTH = vector <double>;
using CTHIter = CTH::iterator;

// If archetype clustering, the archtype # that corresponds to the
// color order, that is, color 1 is archetype 4.
using archType = set <int>;
using archTypeIter = archType::iterator;


class OneRec
{
   public:
      OneRec() { };
      OneRec(string n0,int mc0, double ap0, double rl0, double dp0,string dc0, string rf0,
             double r0,double g0,double b0,int cidx, int arch, int eidx, CTH c0) :
             name(n0),mchan(mc0),ap(ap0),rl(rl0),dp(dp0),dchan(dc0),ref(rf0),
             r(r0),g(g0),b(b0),coloridx(cidx),archetype(arch),expidx(eidx),cth(c0) {} ;
      virtual ~OneRec() { };

      string name;
      int mchan;
      double ap;
      double rl;
      double dp;
      string dchan;
      string ref;
      double r;
      double g;
      double b;
      int coloridx;   // base color index, 0, COLOR_STEPS, 2*COLOR_STEPS, etc
      int archetype;
      int expidx;
      CTH  cth;
      CTH  normCth;
      enum CSV_GLOB {ROWNAME=0,DIST_ALGO,LINK_ALGO,NUMCTH};
      enum CSV_NUM {NAME=0,MCHAN,AP,RL,DP,DCHAN,REF,R,G,B,AP_ATLAS,RL_ATLAS,DP_ATLAS,EXPNAME,PERIOD,ARCH};
      enum DX_POS { DX_AP=0,DX_RL,DX_DP };
      enum DX_COLOR { DX_R=0,DX_G,DX_B };
};

using Cluster = vector <OneRec>;
using ClusterIter = Cluster::iterator;

using Cells = map <int,Cluster>;
using CellIter = Cells::iterator;
using cellArray = array <Cells,CELL_COLORS::NUM_CELL_COLORS>;

using BrainSel = vector<int>;
using BrainSelIter = BrainSel::iterator;

using NameSel = vector<int>;
using NameSelIter = BrainSel::iterator;

namespace Ui {
class BrainStem;
}

class BrainStemGL;

class BrainStem : public QMainWindow
{
   Q_OBJECT

   friend BrainStemGL;

   enum MOVIE_STATE {OFF,PAUSED,ON};

   public:
     explicit BrainStem(QWidget *parent = 0);
     ~BrainStem();
    void printMsg(QString);

   signals:

   private slots:
      void on_Quit_clicked();
      void on_fileOpen_triggered();
      void on_actionQuit_triggered();
      void on_toggleAxes_clicked();
      void on_toggleOutlines_clicked();
      void on_figureReset_clicked();
      void on_spinMe_clicked();
      void glMsg(QString);
      void on_movieNew_clicked();
      void on_movieStart_clicked();
      void on_moviePause_clicked();
      void on_movieStop_clicked();
      void checksClicked(int);
      void on_selectClusts_clicked(const QModelIndex &index);
      void expNamesClicked(int);
      void on_rotateX_clicked();
      void on_rotateY_clicked();
      void on_rotateZ_clicked();
      void on_toggleStereo_clicked();
      void on_allOn_clicked();
      void on_allOff_clicked();
      void on_foreGround_valueChanged(int value);
      void on_backGround_valueChanged(int value);
      void on_delaySlider_valueChanged(int value);
      void on_delaySlider_actionTriggered(int action);
      void timerFired();
      void doSpinMore();
      void newFrame();
      void doTwinkle();
      void on_takeSnap_clicked();
      void on_actionPNG_triggered(bool checked);
      void on_actionPDF_triggered(bool checked);
      void on_toggleColorCycling_clicked();
      void on_twinkleSlider_valueChanged(int value);
      void pauseTimers();
      void restartTimers();
      void on_singleTwinkle_clicked();
      void on_nextStep_clicked();
      void on_actionHide_Inactive_Cells_toggled(bool arg1);
      void on_ptSizeSlider_valueChanged(int value);
      void on_skinToggle_clicked();
      void on_skinTransparencySlider_valueChanged(int value);
      void setupTimers();
      void on_actionClose_triggered();
      void on_Quit_2_clicked();
      void on_ModelChanged(QStandardItem *);
      void on_SelChanged(const QItemSelection&,const QItemSelection&);
      void on_NameModelChanged(QStandardItem *);
      void on_NameSelChanged(const QItemSelection&,const QItemSelection&);
      void on_brainRegionsDeSel_clicked();
      void on_regionTransSlider_valueChanged(int value);
      void on_ambientSlider_valueChanged(int value);
      void on_diffuseSlider_valueChanged(int value);
      void on_xLightSlider_valueChanged(int value);
      void on_yLightSlider_valueChanged(int value);
      void on_actionSurface_Tan_triggered(bool checked);
      void on_actionSurface_White_triggered(bool checked);
      void on_Quit_3_clicked();
      void on_expDeSel_clicked();
      void on_expSelAll_clicked();
      void on_actionOrthoProj_triggered(bool checked);
      void on_actionPerspecProj_triggered(bool checked);
      void on_fovSlider_valueChanged(int value);
      void on_actionHelp_triggered();
      void on_zLightSlider_valueChanged(int value);
      void on_cellSlider_valueChanged(int value);
      void on_boxCtlStim_currentIndexChanged(const QString &arg1);
      void on_actionLoad_Figure_Settings_triggered();
      void on_actionSave_Figure_Settings_triggered();
      void on_brainStemGL_resized();

protected:
      void closeEvent(QCloseEvent *evt);

   private:
     Ui::BrainStem *ui;
     RGBClust rgbClustMap;
     ClustRGB clustRGBMap;
     cellArray dispCells;
     archType archTypeNames;

       // brain regions
     QSignalMapper *checksMapper;
     QStandardItemModel *chkmodel;
       // exp names
     QSignalMapper *expNameMapper;
     QStandardItemModel *expNameModel;

     expNameSet expNames;
     QTimer *masterTimer;
     bool masterOn=false;
     int spinTick=-1;
     int twinkleTick=-1;
     int spinTickRefresh=0;
     int twinkleTickRefresh=0;

     bool spinOn = false;
     int spinTime = 0;

     bool twinkleOn = false;
     int twinkleTime = 0;
     int spinMovieTick;
     int twinkleMovieTick;
     int spinMovieRefresh;
     int twinkleMovieRefresh;
     unsigned int activeTime;
     unsigned int movieTime;

     MOVIE_STATE movieState = OFF;
     int movieElapsed = 0;
     QString movieFile;
     double framesPerSec;
     QSize minMovie;
     QSize maxMovie;

     bool spinSave = false;
     bool twinkleSave = false;
     bool singleTwinkleColor = false;
     bool singleStep = false;
     bool skinOn = true;
     bool axesOn = true;
     bool outlinesOn = false;

     bool readData(QString);
     bool readCSV(QFile&, QString &);
     bool readDX(QFile&, QString &);
     void checksPlease();
     void updateCells(bool,bool);
     void createCycles();

     BrainSel brainShow;   // current selected rows
     BrainSel brainCheck;  // which ones are checked

     NameSel expNameShow;
     NameSel expNameCheck;

     bool haveDelta = false;
     STEREO_MODE stereoMode;
     bool havePhrenic = true;

       // handle signals from UI
     bool doQuit();
     void loadSettings();
     void doMenuOpen();
     void doMenuClose();
     void doToggleAxes();
     void doToggleOutlines();
     void doFigureReset();
     void doSpinMe();
     void doMovieNew();
     void doMovieStart();
     void doMoviePause();
     void doMovieStop();
     void doRotateX();
     void doRotateY();
     void doRotateZ();
     void doToggleStereo();
     void doAllOn();
     void doAllOff();
     void doChecks(bool);
     void doSelectClustsClicked(int);
     void doForeGround(int);
     void doBackGround(int);
     void doSpinChanged(int);
     void doSpinAction(int);
     void doTakeSnap();
     void doToggleColorCycling();
     void doPNGClick(bool);
     void doPDFClick(bool);
     void doTwinkleChanged(int);
     void doSingleTwinkle();
     void doNextStep();
     void doHideCells(bool);
     void doPtSizeChanged(int);
     void doSkinToggle();
     void doSkinTransparencyChanged(int);
     void doBrainRegionsClicked(const QModelIndex &index);
     void doBrainRegionsSel(const QItemSelection&,const QItemSelection&);
     void doBrainRegionsCheck(QStandardItem*);
     void doBrainRegionsDeSel();
     void doRegionTransChanged(int);
     void doCellTransChanged(int);
     void doAmbientChanged(int);
     void doDiffuseChanged(int);
     void doXLightChanged(int);
     void doYLightChanged(int);
     void doZLightChanged(int);
     void doSurfaceTan(bool);
     void doSurfaceWhite(bool);
     void doExpDeSel();
     void doExpSelAll();
     void doExpNameClicked(const QModelIndex &index);
     void doExpNameSel(const QItemSelection&,const QItemSelection&);
     void doExpNameCheck(QStandardItem*);
     void doOrthoProj(bool);
     void doPerspecProj(bool);
     void doFovChanged(int);
     void doHelp();
     void doSelectCtlStim(const QString&);
     void doSelectCtlStim2(const QString&);
     void loadFigureSettings();
     void saveFigureSettings();
     void forceEven();
};


#include "brainstemgl.h"

#endif // BRAINSTEM_H
