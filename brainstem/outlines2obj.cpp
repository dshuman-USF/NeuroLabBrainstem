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



// read atlas dx files and create info for the brainstem program.

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <limits>
#include <memory>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <map>
#include <vector>
#include <tuple>

#include <QFile>
#include <QStringList>
#include <QTextStream>

//#define GLM_FORCE_CXX1Y
//#define GLM_FORCE_RADIANS
//#include <GL/gl.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/constants.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/ext.hpp>

using namespace std;

int main(int argc, char** argv)
{
   int cmd, fld_index, pos_index, conn_index;
   int conn_item, pos_item, field_items, pos_items, conn_items;
   int shape_item, shape_val;
   int p0, p1;
   float x0,y0,z0;
   int pt_offset = 0;
   QStringList comp_pos, comp_color;
   QStringList shape, field, position, connection;
   QStringList::const_iterator f_iter, p_iter, cn_iter, co_iter;
   QStringList rec;
   QString p_row, c_row;
   QStringList p_fields, cn_fields, d_fields, co_fields;
   QStringList fld_ids, pos_ids, conn_ids;
   QString in_name("brain_outlines.dx");
   QString out_name("brain_outlines.obj");
   QString struct_ref;
   QTextStream ref_stream(&struct_ref);
   QString struct_size;
   QTextStream size_stream(&struct_size);
   QString struct_norms;
   QTextStream norm_stream(&struct_norms);
   QString argstr;
   int section_num = 0;

   const struct option opts[] = {
                                   {"in", required_argument, nullptr, 'i'},
                                   {"out", required_argument, nullptr, 'o'},
                                   { 0,0,0,0} };

   while ((cmd = getopt_long(argc, argv, "", opts, nullptr )) != -1)
   {
      switch (cmd)
      {
         case 'i':
               argstr=optarg;
               if (argstr.size() == 0)
               {
                  cout << "Input file is missing, aborting. . .\n";
                  exit(1);
               }
               in_name = argstr;
               break;
         case 'o':
               argstr=optarg;
               if (argstr.size() == 0)
               {
                  cout << "Output file is missing, aborting. . .\n";
                  exit(1);
               }
               out_name = argstr;
               break;
        default:
              cout << "usage: stem2gl [--in inname] [--out outname]" << endl;
              cout << "default input name is brains_outlines.dx, default output name is  brains_outlines.obj" << endl;
              exit(1);
              break;
      }
   }


   QFile in_file(in_name);
   if (!in_file.open(QIODevice::ReadOnly))
   {
      cout << in_name.toLatin1().constData() << " not found" << endl;
      exit(1);
   }
   QFile out_file(out_name);
   if (!out_file.open(QIODevice::WriteOnly))
   {
      cout << "Cannot open output file "<< out_name.toLatin1().constData() << endl;
      exit(1);
   }
   QString out_string;
   QTextStream out_stream(&out_file);
   out_stream << "#Blender v2.78 (sub 0) OBJ File: ''" << endl << "# www.blender.org" << endl;
   QString all = in_file.readAll();

    // create v rows which are pts
    // and then l rows, which is order to draw lines betweeen the v's
   QStringList rows = all.split('\n',QString::SkipEmptyParts);
   QStringList::const_iterator h_iter, iter;

    // find IDs for position and draw sequence
   QStringList fld   = rows.filter(QRegExp("^object.+field$"));
   QStringList pos   = rows.filter(QRegExp("^component \"positions\" value.+"));
   QStringList conn  = rows.filter(QRegExp("^component \"connections\" value.+"));

   QStringList::const_iterator fld_iter, pos_iter, conn_iter;
   int num_fld   = fld.size();
   int num_pos  = pos.size();
   int num_conn = conn.size();

   if (fld.size() != num_pos || num_pos != num_conn)
   {
      cout << "Blocks are mismatched, aborting" << endl;
      exit(1);
   }

   fld_iter = fld.begin();
   pos_iter = pos.begin();
   conn_iter = conn.begin();

   for (int rows = 0; rows < num_fld; ++rows, ++fld_iter, ++pos_iter, ++conn_iter)
   {
      fld_ids   += fld_iter->section(' ',1,1);
      pos_ids   += pos_iter->section(' ',3,3);
      conn_ids  += conn_iter->section(' ',3,3);
   }

   QStringList::const_iterator f_id_iter = fld_ids.begin();
   QStringList::const_iterator p_id_iter = pos_ids.begin();
   QStringList::const_iterator cn_id_iter = conn_ids.begin();

    // each object "field number" class field line starts a new structure
    // not interested in sections that are not fields or not lines
   bool skip_first = true;
   while (f_id_iter != fld_ids.end())
   {
      // make lookup strings
      QString fld_obj("^object " + *f_id_iter + " .+");
      QString pt_obj("^object " + *p_id_iter + " .+");
      QString cn_obj("^object " + *cn_id_iter + " .+");
      ++f_id_iter;
      ++p_id_iter;
      ++cn_id_iter;

        // find field, position, and connection headers
      fld_index = rows.indexOf(QRegExp(fld_obj));
      pos_index = rows.indexOf(QRegExp(pt_obj));
      conn_index = rows.indexOf(QRegExp(cn_obj));
      if (pos_index == -1 || conn_index == -1)
      {
         cout << in_name.toLatin1().constData() << " not found" << endl;
         exit(1);
      }
         // find how many items in each section the connection section selects
         // pts in the pts section, and will be >= # of pts.  Sections where
         // shape != 3 are not triangles, so ignore them.
      shape = rows.at(conn_index).split(' ');
      shape_item = shape.indexOf(QRegExp("^shape$"));
      ++shape_item;
      shape_val = shape.at(shape_item).toInt();
      if (shape_val != 2 || skip_first)
      {
         skip_first = false;
         continue;
      }
        // parse lines like: 
        // object 2001 class array type int rank 1 shape 3 items 8447 data follows
      field = rows.at(fld_index).split(' ');
      position = rows.at(pos_index).split(' ');
      connection = rows.at(conn_index).split(' ');
      field_items =  field.indexOf(QRegExp("^object$"));
      pos_items = position.indexOf(QRegExp("^items$"));
      conn_items = connection.indexOf(QRegExp("^items$"));
      if (pos_items == -1 || conn_items == -1)
      {
         cout << "Something wrong with file "<< out_name.toLatin1().constData() << endl;
         exit(1);
      }
      ++field_items;
      ++pos_items;
      ++conn_items;
      QString fld_num =field.at(field_items);
      fld_num.remove('"');

      conn_item = connection.at(conn_items).toInt();
cout << "Got " << conn_item << " connections" << endl;
      pos_item = position.at(pos_items).toInt();
      ++pos_index;
      ++conn_index;
      p_iter = rows.begin()+pos_index;
      cn_iter = rows.begin()+conn_index;

       // start/continue  building output file
      out_stream << "o section_" << section_num << endl;
      ++section_num;
      for (int item = 0; item < pos_item ; ++item, ++p_iter)
      {
         p_row = p_iter->toLatin1().constData();
         p_fields = p_row.split(' ',QString::SkipEmptyParts);
         if (p_fields.length() == 3)
         {
            x0 = p_fields[0].toFloat();
            y0 = p_fields[1].toFloat();
            z0 = p_fields[2].toFloat();
            out_stream << "v " << x0 << " " << y0 << " " << z0 << endl;
         }
      }

      for (int item = 0; item < conn_item ; ++item, ++cn_iter)
      {
         c_row = cn_iter->toLatin1().constData();
         cn_fields = c_row.split(' ',QString::SkipEmptyParts);
         if (cn_fields.length() == 2)
         {
            p0 = cn_fields[0].toInt();
            p1 = cn_fields[1].toInt();
            ++p0;  // obj files start at 1, dx at 0
            ++p1;
            p0 += pt_offset;
            p1 += pt_offset;
            out_stream << "l " << p0 << " " << p1 << endl;
         }
      }
      pt_offset += conn_item; // obj files don't restart lines at 0
   }
   out_stream.flush();
   out_file.close();

   exit(0);
}

