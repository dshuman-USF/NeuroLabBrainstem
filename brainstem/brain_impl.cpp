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

/* Implementation of main window UI controls
   QTcreator modifies brainstem.cpp, so we let it own that file, more or less,
   and put all of our own code here.  

   This controls the BrainStemGL object, which does the openGL stuff.  It is
   difficult for it to do gui stuff, so these methods do GUI stuff, but also
   manage the state of the drawing machinery, such as if we are animating or
   not. The BrainStemGL object may also maintain its own state vars, some of
   which are duplicates.
*/


#include <random>
#include "brainstem.h"
#include "brainstemgl.h"
#include "ui_brainstem.h"
#include "helpbox.h"

// what text color contrasts best with this background?
rgbLookUp TextContrast(rgbLookUp color)
{
   rgbLookUp contrast;
     // luminance 
   double a = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
   if (a > 0.5)
      contrast.r = contrast.g = contrast.b = 0.0; // bright colors - black font
   else
      contrast.r = contrast.g = contrast.b = 1.0; // dark colors - white font
   return contrast;
}

// This prints messages via a signal from the openGL code.
void BrainStem::glMsg(QString msg)
{
   printMsg(msg);
}
// Print to text window in brainstem gui.
void BrainStem::printMsg(QString msg)
{
   ui->infoOut->appendPlainText(msg);
}

// Note that setting some of these values in controls such as sliders fire
// signal events, which initializes the brainstemGL side of things as a
// side-effect. Sometimes those things do not exist, so we mask some of the
// signals.
void BrainStem::loadSettings()
{
   QSettings settings("cthgui","brainstem");
   if (!settings.status() && settings.contains("geometry")) // does it exist at all?
   {
      bool onoff;
      int pos;
      restoreGeometry(settings.value("geometry").toByteArray());
      restoreState(settings.value("windowState").toByteArray());
      ui->ctrlPlotSplitter->restoreState(settings.value("splitterSizes").toByteArray());
      ui->brainStemGL->loadSettings(settings.value("modelTransforms").toByteArray());
      onoff =  settings.value("pngChecked",true).toBool();
      ui->actionPNG->setChecked(onoff);
      onoff =  settings.value("pdfChecked",false).toBool();
      ui->actionPDF->setChecked(onoff);
      axesOn = settings.value("axesOn",true).toBool();
      outlinesOn = settings.value("outlinesOn",false).toBool();
      pos = settings.value("foreCol",255).toInt();
      ui->foreGround->blockSignals(true);
      ui->foreGround->setSliderPosition(pos);
      ui->foreGround->blockSignals(false);
      pos = settings.value("backCol",70).toInt();
      ui->backGround->setSliderPosition(pos);
      pos = settings.value("animatePos",0).toInt();
      doSpinChanged(pos);
      ui->delaySlider->setSliderPosition(pos);
      pos = settings.value("twinklePos",100).toInt();
      ui->twinkleSlider->setSliderPosition(pos);
      doTwinkleChanged(pos);
      onoff= settings.value("hideCell",0).toBool(); 
      ui->actionHide_Inactive_Cells->setChecked(onoff);
      pos = settings.value("ptSize",-20).toInt();
      ui->ptSizeSlider->setSliderPosition(pos);
      pos = settings.value("skinTrans",70).toInt();
      ui->skinTransparencySlider->setSliderPosition(pos);
      pos = settings.value("cellTrans",100).toInt();
      ui->cellSlider->setSliderPosition(pos);
      pos = settings.value("regionTrans",50).toInt();
      ui->regionTransSlider->setSliderPosition(pos);
      pos = settings.value("fieldOfView",20).toInt();
      ui->fovSlider->setSliderPosition(pos);
      pos = settings.value("lightX",0).toInt();
      ui->xLightSlider->setSliderPosition(pos);
      pos = settings.value("lightY",0).toInt();
      ui->yLightSlider->setSliderPosition(pos);
      pos = settings.value("lightZ",10).toInt();
      ui->zLightSlider->setSliderPosition(pos);
      pos = settings.value("diffuse",100).toInt();
      ui->diffuseSlider->setSliderPosition(pos);
      pos = settings.value("ambient",0).toInt();
      ui->ambientSlider->setSliderPosition(pos);
   }
}

bool BrainStem::doQuit()
{
   if (movieState != OFF)
   {
      QMessageBox mBox;
      mBox.setStandardButtons(QMessageBox::Ok);
      mBox.setText("You are currently recording a movie.\nPlease stop the movie before shutting down the program.");
      mBox.exec();
      return false;
   }

   QSettings settings("cthgui","brainstem");
   if (!settings.status())
   {
      settings.setValue("geometry",saveGeometry());
      settings.setValue("windowState",saveState());
      settings.setValue("splitterSizes",ui->ctrlPlotSplitter->saveState());
      settings.setValue("modelTransforms",ui->brainStemGL->saveSettings());
      settings.setValue("pngChecked",ui->actionPNG->isChecked());
      settings.setValue("pdfChecked",ui->actionPDF->isChecked());
      settings.setValue("foreCol",ui->foreGround->sliderPosition());
      settings.setValue("backCol",ui->backGround->sliderPosition());
      settings.setValue("twinklePos",ui->twinkleSlider->sliderPosition());
      settings.setValue("animatePos",ui->delaySlider->sliderPosition());
      settings.setValue("hideCell",ui->actionHide_Inactive_Cells->isChecked());
      settings.setValue("ptSize",ui->ptSizeSlider->sliderPosition());
      settings.setValue("skinTrans",ui->skinTransparencySlider->sliderPosition());
      settings.setValue("cellTrans",ui->cellSlider->sliderPosition());
      settings.setValue("regionTrans",ui->regionTransSlider->sliderPosition());
      settings.setValue("axesOn",axesOn);
      settings.setValue("outlinesOn",outlinesOn);
      settings.setValue("fieldOfView",ui->fovSlider->sliderPosition());
      settings.setValue("lightX",ui->xLightSlider->sliderPosition());
      settings.setValue("lightY",ui->yLightSlider->sliderPosition());
      settings.setValue("lightZ",ui->zLightSlider->sliderPosition());
      settings.setValue("diffuse",ui->diffuseSlider->sliderPosition());
      settings.setValue("ambient",ui->ambientSlider->sliderPosition());
   }
   close();
   return true;
}

void BrainStem::doMenuOpen()
{
   bool ok = true;
   pauseTimers();

   QString fName = QFileDialog::getOpenFileName(this,
                      tr("Select CSV file to load"), "./", "CSV and DX Files (*.csv *.dx)");
   if (fName.length())
   {
      QFileInfo readInfo(fName);
      QString justName =readInfo.fileName();
      ui->currFile->setText(justName);
      QString ext = readInfo.suffix();
      QFile file(fName); 
      if (!file.open(QIODevice::ReadOnly))
      {
         QString msg;
         QTextStream(&msg) << tr("Error opening file ") << fName << endl << tr("Error is: ") << file.errorString();
         printMsg(msg);
         ok = false;
      }
      if (ok)
      {
         QDir::setCurrent(readInfo.canonicalPath()); // make src the cwd
         if (ext.compare(csvFile,Qt::CaseInsensitive) == 0)
            ok = readCSV(file,justName);
         else if (ext.compare(dxFile,Qt::CaseInsensitive) == 0)
            ok = readDX(file,justName);
         if (ok)
         {
            checksPlease();
            updateCells(true,false);
         }
         file.close();
      }
   }
   restartTimers();
}

void BrainStem::doMenuClose()
{
   ui->brainStemGL->closeFile();
   ui->selectClusts->clear();
   stereoMode=CONTROL_ONLY;
   ui->brainStemGL->toggleStereo(stereoMode);
   ui->boxCtlStim->clear();
   ui->toggleStereo->setEnabled(false);
}

void BrainStem::doAllOn()
{
   doChecks(true);
}

void BrainStem::doAllOff()
{
   doChecks(false);
}

void BrainStem::doChecks(bool state)
{
   QCheckBox *current;
   QListWidgetItem *who;
   int num = ui->selectClusts->count();

   for (int row = 0; row < num ; ++row)
   {
      who = ui->selectClusts->item(row);
      current = dynamic_cast <QCheckBox *> (ui->selectClusts->itemWidget(who));
      if (current != nullptr)
         current->setChecked(state);
   }
   updateCells(false,false);
}

void BrainStem::doRotateX()
{
   ui->brainStemGL->rotateX();
}

void BrainStem::doRotateY()
{
   ui->brainStemGL->rotateY();
}

void BrainStem::doRotateZ()
{
   ui->brainStemGL->rotateZ();
}

// toggle stereo button
void BrainStem::doToggleStereo()
{
   switch (stereoMode)
   {
      case CONTROL_ONLY:
         stereoMode = CONTROL_STEREO;
         break;
      case CONTROL_STEREO:
         stereoMode = CONTROL_ONLY;
         break;
      case STIM_ONLY:
         stereoMode = STIM_STEREO;
         break;
      case STIM_STEREO:
         stereoMode = STIM_ONLY;
         break;
      case DELTA_ONLY:
         stereoMode = DELTA_STEREO;
         break;
      case DELTA_STEREO:
         stereoMode = DELTA_ONLY;
         break;
      default:
      case CTL_STIM_PAIR:  // shouldn't be here, can't do stereo when showing pairs
         cout << "doToggleStereo unexpected state" << endl;
         break;
   }
   ui->brainStemGL->toggleStereo(stereoMode);
}


// This reads a csv file that is generated by the cth_cluster program.  It
// expects a specfic format and will not work with just any .csv file.
bool BrainStem::readCSV(QFile& file, QString& name)
{
   RGBClustIter rgbIter;
   int  totpts = 0;
   QString row, dist, link, period, rowname;
   QString expname;
   QStringList fields, rec;
   rgbLookUp  rgbLook;
   expNameIter nameIter;
   archTypeIter archIter;
   int numbins = 0;
   int clust_num = 0;
   int exp_num = 0;
   int idx, bin, item = 0;
   cellMapIter cm_iter;
   int curr_chan;
   int archetype = 0;
   cellPtMap cellCoords;
   cellPtMapIter cmpt_iter;
   double ap, rl, dp;
   double jitx, jity, jitz;
   int  color_idx;
   bool haveArch = false;
   int cthStart;

   jitx = jity = jitz = 0.0;
   havePhrenic = true;

   QString all = file.readAll();

     // many ways to fail, lambda function to report & exit
   auto bailout = [this](QFile& file) { 
         QString msg;
         QTextStream(&msg) << tr("file ") << file.fileName() << tr(" is not a .csv file that this program can use, not loaded.");
         printMsg(msg);
         return false;
   };

   default_random_engine gen(33620);
   uniform_real_distribution<double> jitter_me(-0.2,0.2);

   expNames.clear();
   expNameModel->clear();
   archTypeNames.clear();

   QStringList rows = all.split('\n',QString::SkipEmptyParts);
   QStringList::const_iterator h_iter, iter;

   QString header_row;
   iter = rows.begin();
   if (iter != rows.end())
   {
      header_row = *iter;
      ++iter;
   }
   else
      return bailout(file);

     // The text is set by export_clust.m.  Changes there may require changes here.
      // newer version
   if (header_row.startsWith("name,mchan,ap,rl,dp,dchan,ref,r,g,b,ap_atlas,rl_atlas,dp_atlas,expname,period,archetype"))
      cthStart=OneRec::ARCH+1;
         // backwards compatible with older version
   else if (header_row.startsWith("name,mchan,ap,rl,dp,dchan,ref,r,g,b,ap_atlas,rl_atlas,dp_atlas,expname,period"))
      cthStart=OneRec::PERIOD+1;
   else
      return bailout(file);

      // 1st data row has info that is common to all data rows
   row = iter->toLatin1().constData();
   fields = row.split(',',QString::SkipEmptyParts);
   if (fields.length() >= OneRec::NUMCTH)
   {
      rowname = fields[OneRec::ROWNAME];
      if (rowname.compare("GLOBALS:") != 0)
         return bailout(file);
      dist = fields[OneRec::DIST_ALGO];
      link = fields[OneRec::LINK_ALGO];
      if (link.compare("archetype") == 0)
         haveArch = true;
      numbins = fields[OneRec::NUMCTH].toInt();
      ++iter; // first real data row
   }
   else
      return bailout(file);

   rgbClustMap.clear();   // build cluster number by color values lookup
   clustRGBMap.clear();
   for (int cells=CELL_COLORS::CONTROL_COLORS; cells < CELL_COLORS::NUM_CELL_COLORS; ++cells)
      dispCells[cells].clear();

   for ( ; iter != rows.end(); ++iter)
   {
      row = iter->toLatin1().constData();
      fields = row.split(',',QString::SkipEmptyParts);
      if (fields.length() >= OneRec::DP_ATLAS+1)
      {
            ap = fields[OneRec::AP_ATLAS].toDouble();
            rl = fields[OneRec::RL_ATLAS].toDouble();
            dp = fields[OneRec::DP_ATLAS].toDouble();
               // no stereotaxis coords for
         if (ap == 0.0 && rl == 0.0 && dp == 0.0) // no stereotaxis coords for
            continue;                            // this one, so skip it
         rgbLook.r = fields[OneRec::R].toDouble(); // extract color, aka, cluster
         rgbLook.g = fields[OneRec::G].toDouble(); // assumes no duplicate colors
         rgbLook.b = fields[OneRec::B].toDouble();
      }
      else
      {
         QString msg;
         QTextStream(&msg) << tr("file ") << file.fileName() << tr(" has line that is too short, skipped.");
         printMsg(msg);
         continue;
      }

      if (rgbLook.r == DELTA_FLAG)  // This is delta CTH, not in a cluster,
      {                             // colors handled differently.
         color_idx = DELTA_FLAG;
      }
      else
      {
         rgbIter = rgbClustMap.find(rgbLook);   // is this a new color/cluster?
         if (rgbIter == rgbClustMap.end())
         {
            RGBInsert add_to = rgbClustMap.insert(make_pair(rgbLook,clust_num));
            rgbIter = add_to.first;
            ++clust_num;
         }
         color_idx = rgbIter->second;
      }
      expname = fields[OneRec::EXPNAME];
      nameIter = expNames.find(expname);
      if (nameIter == expNames.end())
      {
         expNameInsert add_exp = expNames.insert(make_pair(expname,exp_num));
         nameIter = add_exp.first;
         ++exp_num;
      }
      curr_chan = fields[OneRec::MCHAN].toInt();
      period = fields[OneRec::PERIOD];
      if (haveArch)
         archetype = fields[OneRec::ARCH].toInt();

      OneRec aRec { fields[OneRec::NAME].toStdString(),
                    curr_chan,
                    -dp, // fields[OneRec::DP_ATLAS].toDouble(),
                    ap,  // fields[OneRec::AP_ATLAS].toDouble(),
                    -rl, // fields[OneRec::RL_ATLAS].toDouble(),
                    fields[OneRec::DCHAN].toStdString(),
                    fields[OneRec::REF].toStdString(),
                    rgbLook.r, // fields[OneRec::R].toDouble(),
                    rgbLook.b, // fields[OneRec::B].toDouble(),
                    rgbLook.g, // fields[OneRec::G].toDouble(),
                    color_idx,
                    archetype,
                    nameIter->second,
                    CTH() 
                  };

        // We want to jitter cells at same xyz so we can see them.
        // Must apply same jitter to same cell in each period/view.
      xyzCoords xyz(aRec.rl,aRec.ap,aRec.dp);
      aCell newCell(expname,curr_chan);
      cmpt_iter = cellCoords.find(xyz);
      if (cmpt_iter == cellCoords.end())
      {
         jitx = jity = jitz = 0.0;
         jitter jit(jitx, jity, jitz);
         cellCoords[xyz][newCell] = jit;
      }
      else
      {
         cm_iter = cmpt_iter->second.begin();
         if (cm_iter != cmpt_iter->second.end()) 
         {
            cm_iter = cmpt_iter->second.find(newCell);
            if (cm_iter != cmpt_iter->second.end()) 
            {
               jitx = cm_iter->second.jit_x;
               jity = cm_iter->second.jit_y;
               jitz = cm_iter->second.jit_z;
            }
            else
            {
               jitx = jitter_me(gen);
               jity = jitter_me(gen);
               jitz = jitter_me(gen);
               jitter jit(jitx, jity, jitz);
               cellCoords[xyz][newCell]= jit;
            }
         }
      }
      aRec.rl += jitx;
      aRec.ap += jity;
      aRec.dp += jitz;

      if (haveArch && archetype != 0 && archetype != 400)   // skip deltas and flats, not a cluster type
         archTypeNames.insert(archetype);

      if (fields.length() > cthStart) // do we have CTHs?
      {
         for (bin = 0, idx=cthStart; bin < numbins; ++idx, ++bin) // raw cth
           aRec.cth.push_back(fields[idx].toDouble());
         for (bin = 0 ; bin < numbins; ++idx,++bin)              // normalized cth
           aRec.normCth.push_back(fields[idx].toDouble());
      }
      if (color_idx != DELTA_FLAG)  // no clusters/colors lookup for deltas
         clustRGBMap.insert(make_pair(color_idx,rgbLook));
      if (period == PeriodNames[CONTROL_PERIOD] ||
          period == PeriodNames[CTL_CCO2] || period == PeriodNames[CTL_VCO2] ||
          period == PeriodNames[CTL_TBCGH] || period == PeriodNames[CTL_LARCGH] ||
          period == PeriodNames[CTL_SWALLOW1] || period == PeriodNames[CTL_LAREFLEX])
      {
         dispCells[CONTROL_COLORS][color_idx].push_back(aRec);
      }
      else if (period ==PeriodNames[STIM_CCO2] || period == PeriodNames[STIM_VCO2] || 
               period == PeriodNames[STIM_TBCGH] || period == PeriodNames[STIM_LARCGH] ||
               period == PeriodNames[STIM_SWALLOW1] || period == PeriodNames[STIM_LAREFLEX]) 
      {
         dispCells[STIM_COLORS][color_idx].push_back(aRec);
      }
      else if (period == PeriodNames[CS_DELTA])
      {
         dispCells[DELTA_COLORS][0].push_back(aRec); // only one set of colors for this
      }
      else
      {
         cout << "Unsupported period type, case not handled" << period.toLatin1().constData()  << endl;
         continue;
      }
      if (period == PeriodNames[CTL_SWALLOW1] || period == PeriodNames[STIM_SWALLOW1] ||
          period == PeriodNames[CTL_LAREFLEX] || period == PeriodNames[STIM_LAREFLEX])
         havePhrenic = false;
      ++totpts;
   }

    // what can you choose from in combo box?
   ui->toggleStereo->setEnabled(true);
   ui->boxCtlStim->clear();
   QStringList items;
   stereoMode = CONTROL_ONLY;
   if (dispCells[CONTROL_COLORS].size() && dispCells[STIM_COLORS].size())
   {
      stereoMode = CTL_STIM_PAIR;      // default to this if have ctl/stim
      items << selBOTH;
      ui->toggleStereo->setEnabled(false);
   }
   if (dispCells[CONTROL_COLORS].size())
   {
      items << selCONTROL;
   }
   if (dispCells[STIM_COLORS].size())
   {
      items << selSTIM;
   }
   if (dispCells[DELTA_COLORS].size())
   {
      haveDelta = true;
      items << selDELTA;
   }
   ui->boxCtlStim->insertItems(0,items);
   ui->boxCtlStim->setCurrentIndex(0);

   expNameModel->blockSignals(true);
   for (nameIter = expNames.begin(); nameIter != expNames.end(); ++nameIter, ++item)
   {
      QStandardItem* nmstr = new QStandardItem(nameIter->first);
      nmstr->setCheckable(true);
      nmstr->setData(nameIter->second);  // exp #
      expNameModel->setItem(item,nmstr);
      expNameModel->item(item)->setCheckState(Qt::Checked);
   }
   ui->expList->clearSelection();
   expNameModel->blockSignals(false);

   printMsg(tr("Loaded: ") + name);
   QString msg;
   QTextStream(&msg) << tr("Found ") << totpts << " CTHs." << endl << tr("Using ") << numbins << tr(" bins.") << endl << tr("Using ") << clust_num <<  tr(" clusters") << endl << tr("Distance algorithm: ") << dist.toLatin1().constData() << endl << tr("Linkage algorithm: ") << link.toLatin1().constData();
   printMsg(msg);
   return true;
}

/* Read in a DX file.  There are a variety of subsections.  In each one, we want the 
   first which is position data and the third which is color data.  These are
   ordered so that the 1st position line has the color at the 1st color line.
*/
bool BrainStem::readDX(QFile& file,QString& name)
{
   RGBClustIter rgbIter;
   int pos_index, color_index;
   int totpts = 0;

   int items1, items2, pos_items, color_items;
   QStringList comp_pos, comp_color;
   QStringList head1, head2;
   QStringList::const_iterator p_iter, c_iter;
   QString msg;
   QStringList rec;
   rgbLookUp  rgbLook;
   QString p_row, c_row;
   QStringList p_fields, c_fields;
   int clust_num = 0; 
   QStringList pos_ids, color_ids;
   cellMapIter cm_iter;
   dxCellPtMap cellCoords;
   dxCellPtMapIter cmpt_iter;
   double jitx, jity, jitz;

   jitx = jity = jitz = 0.0;
   default_random_engine gen(33620);
   uniform_real_distribution<double> jitter_me(-0.2,0.2);

   QString all = file.readAll();

     // turn file into list of rows
   QStringList rows = all.split('\n',QString::SkipEmptyParts);
   QStringList::const_iterator h_iter, iter;

    // find IDs for position and color components
   QStringList pos_components = rows.filter(QRegExp("^component \"positions\" value.+"));
   QStringList color_components = rows.filter(QRegExp("^component \"colors\" value.+"));
   QStringList::const_iterator pos_iter, color_iter;
   int num_blocks = min(pos_components.size(),color_components.size());
   pos_iter = pos_components.begin();
   color_iter = color_components.begin();
   for (int rows = 0; rows < num_blocks; ++rows, ++pos_iter, ++color_iter)
   {
      pos_ids += pos_iter->section(' ',3,3);
      color_ids += color_iter->section(' ',3,3);
   }

   QStringList::const_iterator p_id_iter = pos_ids.begin();
   QStringList::const_iterator c_id_iter = color_ids.begin();

   rgbClustMap.clear();   // build cluster number by color values lookup
   clustRGBMap.clear();
   for (int cells=CELL_COLORS::CONTROL_COLORS; cells < CELL_COLORS::NUM_CELL_COLORS; ++cells)
      dispCells[cells].clear();

   while (p_id_iter != pos_ids.end())
   {
      // make lookup strings
      QString row_obj("^object " + *p_id_iter + " .+");
      QString color_obj("^object " + *c_id_iter + " .+");
      ++p_id_iter;
      ++c_id_iter;

        // find position and color headers
      pos_index = rows.indexOf(QRegExp(row_obj));
      color_index = rows.indexOf(QRegExp(color_obj));
      if (pos_index == -1 || color_index == -1)
      {
         QTextStream(&msg) << tr("file ") << file.fileName() << tr(" does not appear to be a dx file, not loaded.");
         printMsg(msg);
         return false;
      }
         // find how many items in each section (should be same)
      head1 = rows.at(pos_index).split(' ');
      head2 = rows.at(color_index).split(' ');
      pos_items = head1.indexOf(QRegExp("^items$"));
      color_items = head2.indexOf(QRegExp("^items$"));
      if (pos_items == -1 || color_items == -1)
      {
         QTextStream(&msg) << tr("file ") << file.fileName() << tr(" does not appear to be a dx file, not loaded.");
         printMsg(msg);
         return false;
      }
      
      ++pos_items;
      ++color_items;
      items1 = head1.at(pos_items).toInt();
      items2 = head2.at(color_items).toInt();
      if (items1 == -1 || items1 != items2)
      {
         QTextStream(&msg) << tr("file ") << file.fileName() << tr(" has a mismatch between the number of positions and colors, not loaded.");
         printMsg(msg);
         return false;
      }

      ++pos_index;
      ++color_index;
      p_iter = rows.begin()+pos_index;
      c_iter = rows.begin()+color_index;
      for ( int item = 0; item < items1 ; ++item, ++p_iter, ++c_iter)
      {
         if (p_iter >= rows.end() || c_iter >= rows.end())
         {
            QTextStream(&msg) << tr("There seems to be something wrong with the file ") << file.fileName() << tr(" ,not loaded.") << endl;
            printMsg(msg);
            return false;
         }
         p_row = p_iter->toLatin1().constData();
         c_row = c_iter->toLatin1().constData();
         p_fields = p_row.split(' ',QString::SkipEmptyParts);
         c_fields = c_row.split(' ',QString::SkipEmptyParts);
         if (c_fields.length() >= 3)
         {
            rgbLook.r = c_fields[OneRec::DX_R].toDouble();
            rgbLook.g = c_fields[OneRec::DX_G].toDouble();
            rgbLook.b = c_fields[OneRec::DX_B].toDouble();
         }
         else
         {
            QString msg;
            QTextStream(&msg) << tr("file ") << file.fileName() << tr(" has line that is too short, skipped.");
            printMsg(msg);
            continue;
         }

         rgbIter = rgbClustMap.find(rgbLook);
         if (rgbIter == rgbClustMap.end())
         {
            RGBInsert add_to = rgbClustMap.insert(make_pair(rgbLook,clust_num));
            rgbIter = add_to.first;
            ++clust_num;
         }
            // A dx file does not have most of these values
         OneRec aRec {   "noname",
                         0, 
                         -p_fields[OneRec::DX_DP].toDouble(),
                         p_fields[OneRec::DX_AP].toDouble(),
                         -p_fields[OneRec::DX_RL].toDouble(),
                         "noDchan",
                         "noRef",
                         c_fields[OneRec::DX_R].toDouble(),
                         c_fields[OneRec::DX_G].toDouble(),
                         c_fields[OneRec::DX_B].toDouble(),rgbIter->second,-1,-1,CTH() };
           // We want to jitter cells at same xyz so we can see them.
           // Must apply same jitter to same cell in each period/view.
         xyzCoords xyz(aRec.rl,aRec.ap,aRec.dp);
         cmpt_iter = cellCoords.find(xyz);
         if (cmpt_iter == cellCoords.end())
         {
            jitx = jity = jitz = 0.0;
            jitter jit(jitx, jity, jitz);
            cellCoords.insert(xyz);
         }
         else
         {
            jitx = jitter_me(gen);
            jity = jitter_me(gen);
            jitz = jitter_me(gen);
         }
         aRec.rl += jitx;
         aRec.ap += jity;
         aRec.dp += jitz;

          // all of these go to control list only, there is no period or cth info in these
         dispCells[CONTROL_COLORS][rgbIter->second].push_back(aRec);
         clustRGBMap.insert(make_pair(rgbIter->second,rgbLook));
         ++totpts;
      }
   }

     // some dx files are the structures or skins, but they are hard to tell
     // from just cell files. If there are too many clusters, this is a clue, so bail.
   if (clust_num > 256)
   {
      QString msg;
      QTextStream(&msg) << tr("Found ") << clust_num << " clusters." << tr(" which is too many. This file is probably not a dx cell file, not loaded");
      printMsg(msg);
      return false;
   }

   printMsg(tr("Loaded: ") + name);
   msg.clear();
   QTextStream(&msg) << tr("Found ") << clust_num << " clusters." << tr("  Found ") << totpts << " cells.";
   printMsg(msg);
   return true;
}


// dynamically create a list of checkboxes so we can turn 
// cluster members on and off
void BrainStem::checksPlease()
{
   QString cnum;
   ui->selectClusts->clear();
   ClustRGBIter cRGBIter;
   rgbLookUp clus_bcolor, clus_tcolor;
   archTypeIter archIter;
   bool haveArch = false;

   int num_checks = rgbClustMap.size(); // # of clusts plus maybe flats
   QString style;
   QTextStream style_stream(&style);
   archIter = archTypeNames.begin();
   if (archIter != archTypeNames.end())
      haveArch = true;

   for (int i = 0; i < num_checks ; ++i)
   {
      if (!haveArch)
      {
         if (stereoMode == CTL_STIM_PAIR && i == num_checks-1)
            cnum = tr("Flats ");
         else
            cnum = tr("Cluster ") + QString::number(i+1);
      }
      else
      {
         if (stereoMode == CTL_STIM_PAIR && i == num_checks-1)
            cnum = tr("Flats ");
         else
         {
            cnum = "Archetype " + QString::number(*archIter);
            ++archIter;
         }
      }

      cRGBIter = clustRGBMap.find(i);
      if (cRGBIter == clustRGBMap.end())
      {
         cout << "Warning: You have run out of colors." << endl;
         continue;
      }
      clus_bcolor = cRGBIter->second;
      clus_bcolor.r = pow(clus_bcolor.r,1.0/2.2);  // gamma correction
      clus_bcolor.g = pow(clus_bcolor.g,1.0/2.2); 
      clus_bcolor.b = pow(clus_bcolor.b,1.0/2.2); 

      clus_tcolor = TextContrast(clus_bcolor);
      style_stream.reset();
      style.clear();
       // make a stylesheet string
      style_stream << "background-color: rgb(" << 
                      clus_bcolor.rAsInt()  << 
                      ","  <<
                      clus_bcolor.gAsInt() << 
                      "," << 
                      clus_bcolor.bAsInt() <<
                      "); color: rgb(" <<
                      clus_tcolor.rAsInt()  <<
                      "," << 
                      clus_tcolor.gAsInt() << 
                      "," << 
                      clus_tcolor.bAsInt()  <<
                      ");"; 
      QListWidgetItem *item = new QListWidgetItem("",ui->selectClusts);
      QCheckBox *cb = new QCheckBox(cnum);
      cb->setChecked(true);
      ui->selectClusts->setItemWidget(item,cb);
      cb->setStyleSheet(style);
      checksMapper->setMapping(cb,i);
      connect(cb,SIGNAL(clicked()),checksMapper,SLOT(map()));
   }
}

// Tell the GL stuff to (re)create and display the dots
// It is hard to let the GL class access the UI stuff, so 
// we prepare an array for it that tells it what cluster numbers
// are on and off.
void BrainStem::updateCells(bool new_file, bool exp_chg)
{
   int num = ui->selectClusts->count();
   int expnum = expNameModel->rowCount();
   int row;
   QCheckBox *current;
   QListWidgetItem *who;
   vector<int> on_off(num);
   vector<int> name_on_off(expnum,0);

   for (row = 0; row < num ; ++row)
   {
      who = ui->selectClusts->item(row);
      current = dynamic_cast <QCheckBox *> (ui->selectClusts->itemWidget(who));
      if (current != nullptr)
         on_off[row] = current->isChecked();
   }
//   if (haveDelta) // these are always selected
//      on_off.push_back(true);

   for (row = 0; row < expnum ; ++row)
   {
      if (expNameModel->item(row)->checkState() == Qt::Checked)
         name_on_off[expNameModel->item(row)->data().toInt()] = 1;
   }
   ui->brainStemGL->updateCells(new_file,exp_chg,haveDelta,on_off,name_on_off,dispCells,clustRGBMap,havePhrenic);
}

void BrainStem::checksClicked(int)
{
   updateCells(false,false);
}

// The checkboxes we put into the cluster list do not span the entire
// widget. You can click on the area to the right of the checkbox and
// we will get this event. Use it to toggle to checkbox.
void BrainStem::doSelectClustsClicked(int row)
{
   QListWidgetItem *who = ui->selectClusts->item(row);
   QCheckBox *current = dynamic_cast <QCheckBox *> (ui->selectClusts->itemWidget(who));
   if (current != nullptr)
      current->setChecked(!current->isChecked());
   checksClicked(row);
}
     

void BrainStem::doForeGround(int value)
{
   ui->brainStemGL->doForeGround(value);
}

void BrainStem::doBackGround(int value)
{
   ui->brainStemGL->doBackGround(value);
}


// When timer usage has changed, (re)setup countdown ticks
// assumes this is called with state vars in current state, not previous one
void BrainStem::setupTimers()
{
   int ticks;

   masterOn = true;
   movieTime = min(spinTime,twinkleTime);
   if (movieTime >= 1000.0/24)          // we save assuming at least 24 fps
   {
      movieTime = floor(1000.0/24);  // so make movie timer fire at least that often
      framesPerSec = floor(1000.0/movieTime);
   }
   else
      framesPerSec = 1000.0 / movieTime;
   if (framesPerSec > 60.0)
      framesPerSec = 60.0;

   if (spinOn && !twinkleOn)  // just one timer
   {
      masterTimer->stop();
      spinTick = spinMovieRefresh = 1;
      spinTickRefresh = 1;
      twinkleTick = -1;
      activeTime = spinTime;
   }
   else if (!spinOn && twinkleOn)
   {
      masterTimer->stop();
      twinkleTick = twinkleTickRefresh = 1;
      if (framesPerSec <= 24)
         twinkleMovieRefresh = twinkleTime/movieTime;
      else
         twinkleMovieRefresh = 1;
      spinTick = -1;
      activeTime = twinkleTime;
   }
   else if (!spinOn && !twinkleOn)
   {
      if (movieState != ON)  // let timer run unless we pause
      {                      // so we can have static movie segment
         masterTimer->stop();
         masterOn = false;
      }
      twinkleTick = -1;
      spinTick = -1;
   }
   else  // both on
   {
      masterTimer->stop(); 
      if (spinTime >= twinkleTime) // twinkle drives the timer
      {
         ticks = floor(float(spinTime)/twinkleTime);
         if (ticks < 1)
            ticks = 1;
         spinTickRefresh = spinTick = ticks;
         twinkleTick = 1; 
         twinkleTickRefresh = 1;
         spinMovieRefresh = spinMovieTick = ticks;
         twinkleMovieTick = 1; 
         twinkleMovieRefresh = 1;
         activeTime = twinkleTime;
      }
      else  // spin drives timer
      {
         ticks = floor(float(twinkleTime)/spinTime);
         if ( ticks < 1)
            ticks = 1;
         twinkleTickRefresh = twinkleTick = ticks;
         spinTick = 1; 
         spinTickRefresh = 1;
         if (framesPerSec <= 24)
            twinkleMovieRefresh = twinkleMovieTick = twinkleTime/movieTime;
         else
            twinkleMovieRefresh = twinkleMovieTick = ticks;
         spinMovieTick = 1; 
         spinMovieRefresh = 1;
         activeTime = spinTime;
      }
   }

   if (masterOn)
   {
      if (movieState == ON) 
         masterTimer->start(movieTime);
      else
         masterTimer->start(activeTime);
   }

//streamsize savep = cout.precision();
// cout << "Frames Per Second: " << setprecision(3) << framesPerSec << endl;
//cout.precision(savep);
//cout << endl << "spin delay: " << spinTime << " twinkle delay: " << twinkleTime << endl;
//cout << "spin tick: " << spinTickRefresh << " twinkle tick: " << twinkleTickRefresh << endl;
}

static struct timespec now;
static struct timespec before;
static unsigned long interval;
static bool speedup = false;
// To keep things more or less in sync, use one timer to drive our two
// time-driven events.  If, e.g., we get 1 tick for spin and 8 for twinkle,
// we don't miss timer events if we're making a movie (it can take longer than
// a tick or even 3 or 4 to save the images).
void BrainStem::timerFired()
{
//   monitor actual tick timing
   clock_gettime(CLOCK_MONOTONIC,&now);
   interval = (now.tv_sec*1000*1000*1000 + now.tv_nsec) - (before.tv_sec*1000*1000*1000 + before.tv_nsec);
//   cout << "Time since last timeout: " << (unsigned long)(interval/(1000.0 * 1000.0)) << endl;
   before = now;

   if (movieState != ON)  // only important for real time, movie timer doesn't care
   {
      unsigned long target = (unsigned long)(interval/(1000.0 * 1000.0));
      if (target > activeTime)  // missed 
      {
//   cout << "Missed tick, time since last timeout: " << target << " wanted: " << activeTime << endl;
         if (activeTime < 33)    // times of more than 33 ms seems to never miss ticks
         {
            masterTimer->start(0); // run as fast as we can
            speedup = true;
         }
      }
      else
      {
         if (speedup) // okay to slow down
         {
            speedup = false;
            masterTimer->start(activeTime);
         }
      }
   }
   if (spinTick != -1)
   {
      --spinTick;
      doSpinMore();         // call on every tick
      if (movieState == ON) 
         spinTick = spinMovieRefresh;
      else
         spinTick = spinTickRefresh;
   }
   if (twinkleTick != -1)
   {
      --twinkleTick;
      if (!twinkleTick)
      {
         doTwinkle();
         if (movieState == ON)
            twinkleTick = twinkleMovieRefresh;
         else
            twinkleTick = twinkleTickRefresh;
      }
   }
     // since at least one of the above timed out, 
     // always save the current frame
   if (movieState == ON)
   {
      newFrame();
      ++movieElapsed;
      QString msg;
      QTextStream info(&msg);
      info.setRealNumberNotation(QTextStream::FixedNotation);
      info.setRealNumberPrecision(1);
      info.setFieldWidth(5);
      info.setPadChar(' ');
      info << "Movie Time: " << movieElapsed/framesPerSec << " seconds" << endl;
      ui->movieTime->setText(msg);
   }
}

// start the timer that will drive the animation
void BrainStem::doSpinMe()
{
  spinOn = !spinOn;
  ui->brainStemGL->spinToggle(spinOn);
  setupTimers();
}

void BrainStem::doSpinChanged(int value)
{
   double min, max, mid;
   int fps;

   min = ui->delaySlider->minimum();
   max = ui->delaySlider->maximum();
   mid = (max+min)/2;

   if (value < mid)
      fps = 60 - (60.0 * ((mid-value)/(mid-min)));
   else
      fps = 60.0 * ((max-value)/(max-mid));
   if (fps < 3)
      fps = 3;
   spinTime = floor(1000.0/fps);
   QString msg;
   QTextStream info(&msg);
   info.setRealNumberPrecision(2);
   info.setFieldWidth(5);
   info << "Spin FPS: " << fps << "   ";
   ui->spinFPS->setText(msg);
   if (spinOn)
      setupTimers();
   if (value < mid)
      ui->brainStemGL->doDelayChanged(-spinTime);
   else
      ui->brainStemGL->doDelayChanged(spinTime);
}



// spin timer fired, tell GL window about it
void BrainStem::doSpinMore()
{
   ui->brainStemGL->spinAgain();
}

// HOME key processing for animation speed slider, move it to center, 
// not minimum value, unless making a movie.
void BrainStem::doSpinAction(int action)
{
   int mid = (ui->delaySlider->maximum() + ui->delaySlider->minimum())/2;
   if (action == QAbstractSlider::SliderToMinimum && (movieState == OFF))
      ui->delaySlider->setSliderPosition(mid);
}


void BrainStem::doToggleColorCycling()
{
  twinkleOn = !twinkleOn;
  setupTimers();
  if (twinkleOn)
     ui->toggleColorCycling->setText("Color Cycling Off");
  else
     ui->toggleColorCycling->setText("Color Cycling On");
   ui->brainStemGL->doToggleColorCycling(twinkleOn);
}

// twinkle timer fired or single step button clicked, tell GL window about it
void BrainStem::doTwinkle()
{
   ui->brainStemGL->twinkleAgain();
}

// we have a new frame, if making a movie, save it
void BrainStem::newFrame()
{
//   movieElapsed = ui->brainStemGL->movieFrame();
   ui->brainStemGL->movieFrame();
}

// value is fps -60 to -3
void BrainStem::doTwinkleChanged(int value)
{
   value = abs(value);
   twinkleTime = floor(1000.0/value);

   if (twinkleOn)
      setupTimers();
   ui->brainStemGL->doTwinkleChanged(twinkleTime);
   QString msg;
   QTextStream info(&msg);
   info.setRealNumberPrecision(2);
   info.setFieldWidth(5);
   info << "Cell FPS: " << value  << "   ";
   ui->twinkleFPS->setText(msg);
}

void BrainStem::doSingleTwinkle()
{
   singleTwinkleColor = !singleTwinkleColor;
   if (singleTwinkleColor)
      ui->singleTwinkle->setText("Single Cycle Color Off");
   else
      ui->singleTwinkle->setText("Single Cycle Color On");

   ui->brainStemGL->singleTwinkle(singleTwinkleColor);
}

void BrainStem::doNextStep()
{
   if (twinkleOn)               // kill timer
      doToggleColorCycling();

   ui->delaySlider->setEnabled(true);
   ui->twinkleSlider->setEnabled(true);

   ui->brainStemGL->singleStep(true);
   doTwinkle();                 // button instead of timer
}


// fast animations use up all the cpu, so the dialog boxes
// never get drawn.  If timers are running, stop them, and
// remember their state so we can restart them later.
// This expects pause to be called, then restart.  Calling
// pause again without calling restart will clobber the states.
void BrainStem::pauseTimers()
{
   spinSave = spinOn;
   if (spinOn)
      doSpinMe();      // toggle to off
   twinkleSave = twinkleOn;
   if (twinkleOn)
      doToggleColorCycling();
}

void BrainStem::restartTimers()
{
   if (spinSave)
      doSpinMe();      // toggle to on 
   if (twinkleSave)
      doToggleColorCycling();
}

// Get a final file name for the avi file, then also set up
// a temp directory to accumulate .png files.
void BrainStem::doMovieNew()
{
   pauseTimers();

   QString saveName = QFileDialog::getSaveFileName(this,
                         tr("File to save .avi and .wmv movies to"), 
                         ".", 
                         tr("Movie Files (*.avi *.wmv)"));
   if (!saveName.isEmpty())
   {
      QFileInfo saveInfo(saveName);
      QString justName = saveInfo.fileName();
      QString justPath = saveInfo.canonicalPath();
      if (saveInfo.suffix() == "avi")
      {
         justName.chop(saveName.size() - saveName.lastIndexOf(".avi",-1,Qt::CaseInsensitive));
      }
      else if (saveInfo.suffix() == "wmv")
      {
         justName.chop(saveName.size() - saveName.lastIndexOf(".wmv",-1,Qt::CaseInsensitive));
      }

      QFile test(saveName); // make sure permissions, etc., are okay
      if (!test.open(QIODevice::WriteOnly))
      {
         QString msg;
         QTextStream(&msg) << tr("Error opening file ") << saveName << endl << tr("Error is: ") << test.errorString() << endl << "Movie cannot be started" << endl;
         printMsg(msg);
         return;
      }
      test.close();

      QTemporaryDir movieTmp(justPath + "/" + justName + "XXXXXX");
      if (!movieTmp.isValid())
      {
         QString msg;
         QTextStream(&msg) << tr("Error creating temporary directory ") << movieTmp.path() << endl << "Movie cannot be started" << endl;
         printMsg(msg);
         return;
      }
      movieFile = justPath + "/" + justName;
      movieTmp.setAutoRemove(false);  // keep dir
      ui->movieFile->setText("Creating " + justName + ".avi" + " and " + justName + ".wmv");
      ui->movieStart->setEnabled(true);
      ui->brainStemGL->movieNew(movieFile,movieTmp.path());
   }
   restartTimers();
}

// To start the movie, update controls, then also make sure the animation is on
// and that we set the delay to -1 or 0 to select rotation direction.  ffmpeg
// will create a movie of at least 24 frames per second. The time it takes to
// save the frames is not the running time of the movie, so run as fast as we
// can even if it makes the graphics speed up.
void BrainStem::doMovieStart()
{
   minMovie = ui->brainStemGL->minimumSize();
   maxMovie = ui->brainStemGL->maximumSize();
   auto fixed = ui->brainStemGL->size();
   ui->brainStemGL->setMinimumSize(fixed);
   ui->brainStemGL->setMaximumSize(fixed);

   ui->movieNew->setEnabled(false);
   ui->movieStart->setEnabled(false);
   ui->moviePause->setEnabled(true);
   ui->movieStop->setEnabled(true);
   movieElapsed = 0;
   movieState = ON;
   setupTimers();
   ui->brainStemGL->movieStart(framesPerSec);
}

// Toggle pause on/off.
void BrainStem::doMoviePause()
{
   if (movieState == PAUSED)
   {
      ui->moviePause->setText("Pause Filming");
      ui->movieTime->setText("Continuing");
      movieState = ON;
      ui->brainStemGL->moviePause();
      restartTimers();
   }
   else if (movieState == ON)
   {
      pauseTimers();
      movieState = PAUSED;
      ui->movieTime->setText("Paused");
      ui->moviePause->setText("Continue Filming");
      ui->brainStemGL->moviePause();
   }
}

void BrainStem::doMovieStop()
{
   bool stat;

   movieState = OFF;
   setupTimers();
   ui->delaySlider->setEnabled(true);
   ui->twinkleSlider->setEnabled(true);
   ui->nextStep->setEnabled(true);
   ui->brainStemGL->setMinimumSize(minMovie);
   ui->brainStemGL->setMaximumSize(maxMovie);

   ui->movieNew->setEnabled(true);
   ui->movieStart->setEnabled(false);
   ui->moviePause->setEnabled(false);
   ui->movieStop->setEnabled(false);
   stat = ui->brainStemGL->movieStop();
   QString msg;
   if (stat)
      QTextStream(&msg) << "Movie saved as: " << movieFile + ".avi" << " and " << movieFile+".wmv" << endl << "Movie frames: " << movieElapsed << endl << "Total movie time: " << qSetRealNumberPrecision(3) << movieElapsed/framesPerSec << " seconds." << endl;
   else
      QTextStream(&msg) << "There was a problem saving the movie." << endl << "View the movie.log file in the .avi save directory for clues." << endl;

   printMsg(msg);
   ui->movieTime->setText("");
}



// At least one of PNG and PDF menu checks must be set
void BrainStem::doPNGClick(bool on)
{
  if (on || ui->actionPDF->isChecked())
     return;
  else
     ui->actionPDF->setChecked(true);
}

void BrainStem::doPDFClick(bool on)
{
  if (on || ui->actionPNG->isChecked())
     return;
  else
     ui->actionPNG->setChecked(true);
}

void BrainStem::doTakeSnap()
{
   pauseTimers();
   QString saveName = QFileDialog::getSaveFileName(this,
                         tr("File to save frame to"), ".", tr(".png .pdf Files (*.png *.pdf)"));
   if (!saveName.isEmpty())
   {
      QString pngName;
      QString pdfName;
      QFileInfo saveInfo(saveName);

      if (saveInfo.suffix() == "png")
         saveName.chop(saveName.size() - saveName.lastIndexOf(".png",-1,Qt::CaseInsensitive));
      else if (saveInfo.suffix() == "pdf")
         saveName.chop(saveName.size() - saveName.lastIndexOf(".pdf",-1,Qt::CaseInsensitive));
      QString justName = saveInfo.fileName();
      if (ui->actionPNG->isChecked())
      {
         pngName = saveName + ".png";
         QFile test(pngName); // make sure permissions, etc., are okay
         if (!test.open(QIODevice::WriteOnly))
         {
            QString msg;
            QTextStream(&msg) << tr("Error opening file ") << pngName << endl << tr("Error is: ") << test.errorString() << endl << "Frame not saved." << endl;
            printMsg(msg);
            pngName.clear();
         }
         test.close();
      }

      if (ui->actionPDF->isChecked())
      {
         pdfName = saveName + ".pdf";
         QFile test(pdfName); // make sure permissions, etc., are okay
         if (!test.open(QIODevice::WriteOnly))
         {
            QString msg;
            QTextStream(&msg) << tr("Error opening file ") << pdfName << endl << tr("Error is: ") << test.errorString() << endl << "Frame not saved." << endl;
            printMsg(msg);
            pdfName.clear();
         }
         test.close();
      }
      if (pngName.length() || pdfName.length())
      {
         printMsg("Saving frame.");
         ui->brainStemGL->saveFrame(pngName,pdfName);
      }
   }
   restartTimers();
}


void BrainStem::doHideCells(bool on)
{
   ui->brainStemGL->doHideCells(on);
}

void BrainStem::doPtSizeChanged(int size)
{
   ui->brainStemGL->doPtSizeChanged(size);
}

void BrainStem::doSkinToggle()
{
   skinOn = !skinOn;
   ui->brainStemGL->doSkinToggle(skinOn);
}

void BrainStem::doToggleOutlines()
{
   outlinesOn = !outlinesOn;
   ui->brainStemGL->doToggleOutlines(outlinesOn);
}

void BrainStem::doToggleAxes()
{
   axesOn = !axesOn;
   ui->brainStemGL->doToggleAxes(axesOn);
}

void BrainStem::doSkinTransparencyChanged(int value)
{
   ui->brainStemGL->doSkinTransparencyChanged(value);
}
void BrainStem::doCellTransChanged(int value)
{
   ui->brainStemGL->doCellTransparencyChanged(value);
}

// Make a list of all selected rows
void BrainStem::doBrainRegionsClicked(const QModelIndex & /* index */)
{
   QStandardItem *it;
   brainShow.clear();
   QModelIndexList curr = ui->brainRegions->selectionModel()->selectedIndexes();
   for (int row = 0; row < curr.size(); ++row)
   {
      it = chkmodel->itemFromIndex(curr[row]);
      if (it)
         brainShow.push_back(it->row());
   }
}

// region selection / deselection signal
void BrainStem::doBrainRegionsSel(const QItemSelection& /*sel*/,const QItemSelection& /*desel*/ )
{
   QModelIndex not_used;
   doBrainRegionsClicked(not_used);
}

// Handle checkbox click signal.
// If the click is outside of the selected row(s), clear the selections and
// move to the new selected row.
void BrainStem::doBrainRegionsCheck(QStandardItem* item)
{
   BrainSelIter iter;
   bool in_sel = false;
   int row;
   QStandardItem *it;
   Qt::CheckState on_off = item->checkState();

      // list of currently selected
   for (iter = brainShow.begin(); iter != brainShow.end(); ++iter)
   {
      if (item->row() == chkmodel->item(*iter)->row())
      {
         in_sel = true;  // clicked item must be in selection list
         break;
      } }

   if (in_sel)
   {
      chkmodel->blockSignals(true);
      for (iter = brainShow.begin(); iter != brainShow.end(); ++iter)
      {
         it = chkmodel->item(*iter);
         it->setCheckState(on_off);

      }
      chkmodel->blockSignals(false);
   }
   else
   {
       // move selection to clicked row
      brainShow.clear();
      const QModelIndex currentIndex = chkmodel->indexFromItem(item);
      QItemSelectionModel *selModel = ui->brainRegions->selectionModel();
      selModel->setCurrentIndex(currentIndex, 
                                QItemSelectionModel::Select | QItemSelectionModel::Current);
   }

   brainCheck.clear();
   for (row = 0; row < chkmodel->rowCount(); ++row) // update display
   {
      it = chkmodel->item(row);
      if (it->checkState() == Qt::Checked)
         brainCheck.push_back(it->row());
   }
   ui->brainStemGL->updateRegions(brainCheck);
}


void BrainStem::doBrainRegionsDeSel()
{
   brainCheck.clear();
   for (int row = 0; row < chkmodel->rowCount(); ++row)
      chkmodel->item(row)->setCheckState(Qt::Unchecked);
   ui->brainStemGL->updateRegions(brainCheck);
}

void BrainStem::doRegionTransChanged(int value)
{
   ui->brainStemGL->doRegionTransChanged(value);
}

void BrainStem::doAmbientChanged(int value)
{
   QString ambval;
   QTextStream(&ambval) << value;
   ui->ambientVal->setText(ambval);
   ui->brainStemGL->doAmbient(value);
}

void BrainStem::doDiffuseChanged(int value)
{
   QString difval;
//   QTextStream(&difval) << value/10.0;
   QTextStream(&difval) << value;
   ui->diffuseVal->setText(difval);
   ui->brainStemGL->doDiffuse(value);
}


void BrainStem::doXLightChanged(int value)
{
   QString xval;
   QTextStream(&xval) << value;
   ui->xLightVal->setText(xval);
   ui->brainStemGL->doLightDistX(value);
}

void BrainStem::doYLightChanged(int value)
{
   QString yval;
   QTextStream(&yval) << value;
   ui->yLightVal->setText(yval);
   ui->brainStemGL->doLightDistY(value);
}

void BrainStem::doZLightChanged(int value)
{
   QString zval;
   QTextStream(&zval) << value;
   ui->zLightVal->setText(zval);
   ui->brainStemGL->doLightDistZ(value);
}

void BrainStem::doSurfaceTan(bool checked)
{
  if (checked)
  {
     ui->actionSurface_White->setChecked(false);
     ui->brainStemGL->doSurfaceT();
  }
  else
  {
     ui->actionSurface_White->setChecked(true);
     ui->brainStemGL->doSurfaceW();
  }
}


void BrainStem::doSurfaceWhite(bool checked)
{
  if (checked)
  {
     ui->actionSurface_Tan->setChecked(false);
     ui->brainStemGL->doSurfaceW();
  }
  else
  {
     ui->actionSurface_Tan->setChecked(true);
     ui->brainStemGL->doSurfaceT();
  }
}


// show no experiments
void BrainStem::doExpDeSel()
{

   expNameModel->blockSignals(true);
   expNameCheck.clear();
   for (int row = 0; row < expNameModel->rowCount(); ++row)
      expNameModel->item(row)->setCheckState(Qt::Unchecked);
   expNameModel->blockSignals(false);
   ui->expList->reset();  // refresh view/display
   updateCells(false,true);
}

// show all experiments
void BrainStem::doExpSelAll()
{
   expNameModel->blockSignals(true);
   expNameCheck.clear();
   for (int row = 0; row < expNameModel->rowCount(); ++row)
   {
      expNameModel->item(row)->setCheckState(Qt::Checked);
      expNameCheck.push_back(row);
   }
   expNameModel->blockSignals(false);
   ui->expList->reset();
   updateCells(false,true);
}


void BrainStem::doExpNameSel(const QItemSelection& /*sel*/,const QItemSelection& /*desel*/)
{
   QModelIndex not_used;
   doExpNameClicked(not_used);
}

void BrainStem::expNamesClicked(int)
{
   QStandardItem *it;
   expNameShow.clear();
   QModelIndexList curr = ui->expList->selectionModel()->selectedIndexes();
   for (int row = 0; row < curr.size(); ++row)
   {
      it = expNameModel->itemFromIndex(curr[row]);
      if (it)
         expNameShow.push_back(it->row());
   }
}


void BrainStem::doExpNameCheck(QStandardItem* item)
{
   NameSelIter iter;
   bool in_sel = false;
   int row;
   QStandardItem *it;
   Qt::CheckState on_off = item->checkState();

      // list of currently selected
   for (iter = expNameShow.begin(); iter != expNameShow.end(); ++iter)
   {
      if (item->row() == expNameModel->item(*iter)->row())
      {
         in_sel = true;  // clicked item must be in selection list
         break;
      } }

   if (in_sel)
   {
      expNameModel->blockSignals(true);
      for (iter = expNameShow.begin(); iter != expNameShow.end(); ++iter)
      {
         it = expNameModel->item(*iter);
         it->setCheckState(on_off);

      }
      expNameModel->blockSignals(false);
   }
   else
   {
       // move selection to clicked row
      expNameShow.clear();
      const QModelIndex currentIndex = expNameModel->indexFromItem(item);
      QItemSelectionModel *selModel = ui->expList->selectionModel();
      selModel->setCurrentIndex(currentIndex, 
                                QItemSelectionModel::Select | QItemSelectionModel::Current);
   }

   expNameCheck.clear();
   for (row = 0; row < expNameModel->rowCount(); ++row) // update display
   {
      it = expNameModel->item(row);
      if (it->checkState() == Qt::Checked)
         expNameCheck.push_back(it->row());
   }
   updateCells(false,true);
}

void BrainStem::doExpNameClicked(const QModelIndex & /* index */)
{
   QStandardItem *it;
   expNameShow.clear();
   QModelIndexList curr = ui->expList->selectionModel()->selectedIndexes();
   for (int row = 0; row < curr.size(); ++row)
   {
      it = expNameModel->itemFromIndex(curr[row]);
      if (it)
         expNameShow.push_back(it->row());
   }
}

void BrainStem::doOrthoProj(bool checked)
{
  if (checked)
  {
     ui->actionPerspecProj->setChecked(false);
     ui->brainStemGL->doOrtho();
  }
  else
  {
     ui->actionPerspecProj->setChecked(true);
     ui->brainStemGL->doPerspec();
  }
}

void BrainStem::doPerspecProj(bool checked)
{
  if (checked)
  {
     ui->actionOrthoProj->setChecked(false);
     ui->brainStemGL->doPerspec();
  }
  else
  {
     ui->actionOrthoProj->setChecked(true);
     ui->brainStemGL->doOrtho();
  }
}

void BrainStem::doFovChanged(int value)
{
   ui->brainStemGL->doFov(value);
}

void BrainStem::doHelp()
{
   HelpBox *help = new HelpBox(this);
   help->exec();
}


// Selection made in control/ctl/stim combo
// This only fires if we have ctl/stim cths.
void BrainStem::doSelectCtlStim(const QString& sel)
{
   if (sel.compare(selBOTH) == 0)
   {
     ui->toggleStereo->setEnabled(false);
     stereoMode = CTL_STIM_PAIR;
   }
   else if (sel.compare(selCONTROL) == 0)
   {
      if (stereoMode == STIM_STEREO || stereoMode == DELTA_STEREO) // leave in stereo mode
         stereoMode = CONTROL_STEREO;
      else
         stereoMode = CONTROL_ONLY;
      ui->toggleStereo->setEnabled(true);
   }
   else if (sel.compare(selSTIM) == 0)
   {
      if (stereoMode == CONTROL_STEREO || stereoMode == DELTA_STEREO)
         stereoMode = STIM_STEREO;
      else
         stereoMode = STIM_ONLY;
      ui->toggleStereo->setEnabled(true);
   }
   else if (sel.compare(selCONTROL) == 0)
   {
      stereoMode = CONTROL_ONLY;
      ui->toggleStereo->setEnabled(true);
   }
   else if (sel.compare(selDELTA) == 0)
   {
      if (stereoMode == CONTROL_STEREO || stereoMode == STIM_STEREO)
         stereoMode = DELTA_STEREO;
      else
         stereoMode = DELTA_ONLY;
      ui->toggleStereo->setEnabled(true);
   }
   ui->brainStemGL->showCtlStim(stereoMode);
}


void BrainStem::loadFigureSettings()
{
   pauseTimers();

   QString fName = QFileDialog::getOpenFileName(this,
                      tr("Select figure setting to use."), "./", "SET files (*.set)");
   if (fName.length())
   {
      QFile file(fName); 
      if (!file.open(QIODevice::ReadOnly))
      {
         QString msg;
         QTextStream(&msg) << tr("Error opening file ") << fName << endl << tr("Error is: ") << file.errorString();
         printMsg(msg);

      }
      else
      {
         QTextStream in(&file);
         QString dummy;
         int ver, val;
         in >> dummy >> ver;  // version
         in.seek(0);
         ui->brainStemGL->loadFig(in);
         if (ver == 1)
         {
            in >> dummy >> val; 
            ui->backGround->setSliderPosition(val);
            in >> dummy >> val; 
            ui->foreGround->setSliderPosition(val);
            in >> dummy >> val; 
            ui->ptSizeSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->skinTransparencySlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->cellSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->regionTransSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->fovSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->xLightSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->yLightSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->zLightSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->diffuseSlider->setSliderPosition(val);
            in >> dummy >> val; 
            ui->ambientSlider->setSliderPosition(val);
            in >> dummy >> val;
            axesOn = !val; // setup for toggle
            in >> dummy >> val;
            outlinesOn = !val;
         }
         doToggleAxes();
         doToggleOutlines();
      }
   }
   restartTimers();
}

void BrainStem::saveFigureSettings()
{
   pauseTimers();

   QString saveName = QFileDialog::getSaveFileName(this,
                         tr("Save figure settings."), 
                         ".", 
                         tr("Setting Files (*.set)"));
   if (saveName.length())
   {
      QFileInfo saveInfo(saveName);
      if (saveInfo.suffix() != "set")
         saveName += ".set";
      QFile file(saveName); 
      if (!file.open(QIODevice::WriteOnly))
      {
         QString msg;
         QTextStream(&msg) << tr("Error opening file ") << saveName << endl << tr("Error is: ") << file.errorString() << endl << "Movie cannot be started" << endl;
         printMsg(msg);
         return;
      }
      else
      {
         QTextStream out(&file);
         ui->brainStemGL->saveFig(out); // this will save it's non-control parts
           // now add in our controls
         out << "back_color: " << ui->backGround->sliderPosition() << endl;
         out << "section_color: " <<  ui->foreGround->sliderPosition() << endl;
         out << "point_size: " << ui->ptSizeSlider->sliderPosition() << endl;
         out << "skin_transparency: " << ui->skinTransparencySlider->sliderPosition() << endl;
         out << "cell_size: " << ui->cellSlider->sliderPosition() << endl;
         out << "region_transparency: "<< ui->regionTransSlider->sliderPosition() << endl;
         out << "FOV: " << ui->fovSlider->sliderPosition() << endl;
         out << "light_x: " << ui->xLightSlider->sliderPosition() << endl;
         out << "light_y: " << ui->yLightSlider->sliderPosition() << endl;
         out << "light_z: " << ui->zLightSlider->sliderPosition() << endl;
         out << "diffuse: " << ui->diffuseSlider->sliderPosition() << endl;
         out << "ambient: " << ui->ambientSlider->sliderPosition() << endl;
         out << "axes_on: " << axesOn << endl;
         out << "sections_on: " << outlinesOn << endl;
      }
      file.close();
   }
   restartTimers();
}


void BrainStem::forceEven()
{
   bool adjust = false;
   QRect rect = ui->brainStemGL->geometry();
   int w = rect.width();
   int h = rect.height();

   if (w % 2) // force framebuffer to be even w & h
   {
      rect.setWidth(--w);
      adjust = true;
   }
   if (rect.height() % 2)
   {
      rect.setHeight(--h);
      adjust = true;
   }
   if (adjust)
      ui->brainStemGL->setGeometry(rect);
}
