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

/* The main window managed here.
   Most GUI stuff is passed on down to child openGL window.
   The qtcreator gui designer modifies this file. Sometimes it can corrupt the
   file, so I keep the implementation separate from what the designer does.
   Most of the functions here are just pass-throughs. The implementation code
   is in brain_impl.cpp.
*/


#include "brainstem.h"
#include "ui_brainstem.h"
#include <QSurfaceFormat>
#include <QPushButton>

extern "C" {
extern char* structNames[];
extern int totNames;
}


BrainStem::BrainStem(QWidget *parent) : QMainWindow(parent), ui(new Ui::BrainStem)
{
   ui->setupUi(this);

   setWindowIcon(QIcon(":/brainstem.png"));
   QString title("BRAINSTEM Version: ");
   title = title.append(VERSION);
   setWindowTitle(title);

   loadSettings();

   masterTimer = new QTimer(this);
   masterTimer->setTimerType(Qt::PreciseTimer);
   connect(masterTimer,SIGNAL(timeout()),this,SLOT(timerFired()));

      // let GL window talk to us
   connect(ui->brainStemGL,SIGNAL(chatBox(QString)),this,SLOT(glMsg(QString)));

     // setup click event for dynamic checkboxes for brain regions
   checksMapper = new QSignalMapper(this);
   connect(checksMapper,SIGNAL(mapped(int)),this,SLOT(checksClicked(int)));
   chkmodel = new QStandardItemModel(this);
   for (int name=0; name < totNames; ++name)
   {
      QStandardItem* chkstr = new QStandardItem(structNames[name]); // todo: who frees these?
      chkstr->setCheckable(true);
      chkmodel->setItem(name,chkstr);
   }
   ui->brainRegions->setModel(chkmodel);

   connect(chkmodel,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(on_ModelChanged(QStandardItem*)));

   connect(ui->brainRegions->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,SLOT(on_SelChanged(const QItemSelection&,const QItemSelection&)));

   expNameMapper = new QSignalMapper(this);
   connect(expNameMapper,SIGNAL(mapped(int)),this,SLOT(expNamesClicked(int)));
   expNameModel = new QStandardItemModel(this);
   ui->expList->setModel(expNameModel);
   connect(expNameModel,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(on_NameModelChanged(QStandardItem*)));
   connect(ui->expList->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,SLOT(on_NameSelChanged(const QItemSelection&,const QItemSelection&)));

}


BrainStem::~BrainStem()
{
   delete ui;
}

void BrainStem::on_fileOpen_triggered()
{
   doMenuOpen();
}

void BrainStem::on_Quit_clicked()
{
   doQuit();
}

void BrainStem::on_actionQuit_triggered()
{
    doQuit();
}

void BrainStem::closeEvent(QCloseEvent *evt)
{
   if (!doQuit())
      evt->ignore();
   else
      evt->accept();

}

void BrainStem::on_figureReset_clicked()
{
   ui->brainStemGL->reset();
}

void BrainStem::on_spinMe_clicked()
{
   doSpinMe();
}

void BrainStem::on_movieNew_clicked()
{
   doMovieNew();
}

void BrainStem::on_movieStart_clicked()
{
   doMovieStart();
}

void BrainStem::on_moviePause_clicked()
{
   doMoviePause();
}

void BrainStem::on_movieStop_clicked()
{
   doMovieStop();
}

void BrainStem::on_selectClusts_clicked(const QModelIndex &index)
{
  doSelectClustsClicked(index.row());
}

void BrainStem::on_rotateX_clicked()
{
   doRotateX();
}

void BrainStem::on_rotateY_clicked()
{
   doRotateY();
}

void BrainStem::on_rotateZ_clicked()
{
   doRotateZ();
}

void BrainStem::on_toggleStereo_clicked()
{
    doToggleStereo();
}

void BrainStem::on_allOn_clicked()
{
   doAllOn();
}

void BrainStem::on_allOff_clicked()
{
   doAllOff();
}

void BrainStem::on_foreGround_valueChanged(int value)
{
   doForeGround(value);
}

void BrainStem::on_backGround_valueChanged(int value)
{
   doBackGround(value);
}

void BrainStem::on_delaySlider_valueChanged(int value)
{
   doSpinChanged(value);
}

void BrainStem::on_delaySlider_actionTriggered(int action)
{
   doSpinAction(action);
}

void BrainStem::on_takeSnap_clicked()
{
   doTakeSnap();
}

void BrainStem::on_actionPNG_triggered(bool checked)
{
   doPNGClick(checked);
}

void BrainStem::on_actionPDF_triggered(bool checked)
{
    doPDFClick(checked);
}

void BrainStem::on_toggleColorCycling_clicked()
{
   doToggleColorCycling();
}

void BrainStem::on_twinkleSlider_valueChanged(int value)
{
   doTwinkleChanged(value);
}

void BrainStem::on_singleTwinkle_clicked()
{
   doSingleTwinkle();
}

void BrainStem::on_nextStep_clicked()
{
    doNextStep();
}

void BrainStem::on_actionHide_Inactive_Cells_toggled(bool on)
{
   doHideCells(on);
}

void BrainStem::on_ptSizeSlider_valueChanged(int value)
{
   doPtSizeChanged(value);
}

void BrainStem::on_skinToggle_clicked()
{
   doSkinToggle();
}

void BrainStem::on_skinTransparencySlider_valueChanged(int value)
{
   doSkinTransparencyChanged(value);
}

void BrainStem::on_actionClose_triggered()
{
   doMenuClose();
}

void BrainStem::on_ModelChanged(QStandardItem* item)
{
   doBrainRegionsCheck(item);
}

void BrainStem::on_SelChanged(const QItemSelection& sel,const QItemSelection& desel)
{
   doBrainRegionsSel(sel,desel);
}

void BrainStem::on_NameModelChanged(QStandardItem* item)
{
   doExpNameCheck(item);
}

void BrainStem::on_NameSelChanged(const QItemSelection& sel,const QItemSelection& desel)
{
   doExpNameSel(sel,desel);
}

void BrainStem::on_Quit_2_clicked()
{
   doQuit();
}

void BrainStem::on_brainRegionsDeSel_clicked()
{
   doBrainRegionsDeSel();
}

void BrainStem::on_regionTransSlider_valueChanged(int value)
{
   doRegionTransChanged(value);
}

void BrainStem::on_ambientSlider_valueChanged(int value)
{
   doAmbientChanged(value);
}

void BrainStem::on_diffuseSlider_valueChanged(int value)
{
   doDiffuseChanged(value);
}


void BrainStem::on_xLightSlider_valueChanged(int value)
{
   doXLightChanged(value);
}

void BrainStem::on_yLightSlider_valueChanged(int value)
{
   doYLightChanged(value);
}

void BrainStem::on_actionSurface_Tan_triggered(bool checked)
{
   doSurfaceTan(checked);
}

void BrainStem::on_actionSurface_White_triggered(bool checked)
{
   doSurfaceWhite(checked);
}

void BrainStem::on_toggleAxes_clicked()
{
   doToggleAxes();
}

void BrainStem::on_toggleOutlines_clicked()
{
   doToggleOutlines();
}

void BrainStem::on_Quit_3_clicked()
{
   doQuit();
}

void BrainStem::on_expDeSel_clicked()
{
   doExpDeSel();
}

void BrainStem::on_expSelAll_clicked()
{
   doExpSelAll();
}

void BrainStem::on_actionOrthoProj_triggered(bool checked)
{
    doOrthoProj(checked);
}

void BrainStem::on_actionPerspecProj_triggered(bool checked)
{
   doPerspecProj(checked);
}

void BrainStem::on_fovSlider_valueChanged(int value)
{
   doFovChanged(value);
}

void BrainStem::on_actionHelp_triggered()
{
   doHelp();
}

void BrainStem::on_zLightSlider_valueChanged(int value)
{
    doZLightChanged(value);
}

void BrainStem::on_cellSlider_valueChanged(int value)
{
   doCellTransChanged(value);
}

void BrainStem::on_boxCtlStim_currentIndexChanged(const QString &arg1)
{
   doSelectCtlStim(arg1);
}

void BrainStem::on_actionLoad_Figure_Settings_triggered()
{
   loadFigureSettings();
}

void BrainStem::on_actionSave_Figure_Settings_triggered()
{
   saveFigureSettings();
}

void BrainStem::on_brainStemGL_resized()
{
   forceEven();
}
