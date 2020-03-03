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
#include "brainstem.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QGLFormat>
#include <glm/glm.hpp>

bool Debug = false;

int main(int argc, char *argv[])
{
//   QApplication app(argc, argv);

   QSurfaceFormat format;
   format = QSurfaceFormat::defaultFormat();
   if (argc > 1)
   {
      if (strcmp(argv[1],"-d") == 0)
      {
         cout << "Debug output turned on" << endl;
         format.setOption(QSurfaceFormat::DebugContext);
         Debug = true;
      }
   }

   //cout << "format opt is: " << format.options() << endl;
   //cout << "format profile 1 is: " << format.profile() << endl;
   format.setProfile(QSurfaceFormat::CoreProfile);
   format.setVersion(4,3);
   QSurfaceFormat::setDefaultFormat(format);

   QApplication app(argc, argv);

   if (!QGLFormat::hasOpenGL())
   {
      QMessageBox msgBox;
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setText("This application cannot run on this system because it does not support openGL");
      msgBox.exec();
      exit(1);
   }

   QGLFormat::OpenGLVersionFlags vers;
//   cout << "Created vers" << endl;
//   fflush(stdout);

   vers = QGLFormat::openGLVersionFlags();
//   cout << "Got flags " << vers << endl;
//   fflush(stdout);

   QString verSupp;
   QTextStream(&verSupp) << "\nVersions supported: ";

   if (vers & QGLFormat::OpenGL_Version_1_5)
      QTextStream(&verSupp) << "1.5  "; 
   if (vers & QGLFormat::OpenGL_Version_2_0)
      QTextStream(&verSupp) << "2.0  "; 
   if (vers & QGLFormat::OpenGL_Version_2_1)
      QTextStream(&verSupp) << "2.1  "; 
   if (vers & QGLFormat::OpenGL_Version_3_0)
      QTextStream(&verSupp) << " 3.0  "; 
   if (vers & QGLFormat::OpenGL_Version_3_1)
      QTextStream(&verSupp) << "3.1  "; 
   if (vers & QGLFormat::OpenGL_Version_3_2)
      QTextStream(&verSupp) << "3.2  "; 
   if (vers & QGLFormat::OpenGL_Version_3_3)
      QTextStream(&verSupp) << "3.3  "; 
   if (vers & QGLFormat::OpenGL_Version_4_0)
      QTextStream(&verSupp) << "4.0  "; 
   if (vers & QGLFormat::OpenGL_Version_4_1)
      QTextStream(&verSupp) << "4.1  "; 
   if (vers & QGLFormat::OpenGL_Version_4_2)
      QTextStream(&verSupp) << "4.2  "; 
   if (vers & QGLFormat::OpenGL_Version_4_3)
      QTextStream(&verSupp) << "4.3, or later ";

   if (!(vers & QGLFormat::OpenGL_Version_4_3))
   {
      QMessageBox msgBox;
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setText("This application cannot run on this system because it does not support openGL version 4.3 or later.\nYou may need to update your graphics card device driver.\nNote that this application will not run in a VM or in an x2go session."+verSupp);
      msgBox.exec();
      exit(1);
   }

   QGLFormat form;
   if (!form.directRendering())
   {
      QMessageBox msgBox;
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setText("This application cannot run on this system because it does not support direct rendering");
      msgBox.exec();
      exit(1);
   }

   if (Debug)
   {
      QMessageBox msgBox;
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setText("This following openGl versions are supported on this system:\n"+verSupp);
      msgBox.exec();
   }

    BrainStem win;
    win.show();
    return app.exec();
}
