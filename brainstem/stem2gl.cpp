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

#define GLM_FORCE_CXX1Y
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace std;

class vtxLookUp
{
   public:
      vtxLookUp():vtx(glm::vec3(0.0)) {};
      vtxLookUp(glm::vec3 v0) {vtx = v0;};
      vtxLookUp(const vtxLookUp& v0):vtx(v0.vtx) {};
      virtual ~vtxLookUp() {}

      glm::vec3 vtx;
};

// for sorting/comparing rgb tuples
class CompVtx
{
   public:
      bool operator() (vtxLookUp const& lhs, vtxLookUp const& rhs) const
      { return tie(lhs.vtx.x,lhs.vtx.y,lhs.vtx.z) < tie(rhs.vtx.r,rhs.vtx.y,rhs.vtx.z); }
};


typedef vector <glm::vec3> triNorms;
typedef map <vtxLookUp,triNorms,CompVtx> smoothNorms;
typedef smoothNorms::iterator smoothNormsIter;
typedef pair<smoothNormsIter,bool> smoothInsert;

using namespace std;

int main(int argc, char** argv)
{
   int cmd, fld_index, pos_index, conn_index;
   int fld_item, conn_item, field_items, pos_items, conn_items;
   int shape_item, shape_val;
   int p0, p1, p2;
   GLfloat x0,y0,z0;
   GLfloat x1,y1,z1;
   GLfloat x2,y2,z2;
   int obj_start = 0;
   int obj_count = 0;
   QStringList comp_pos, comp_color;
   QStringList shape, field, position, connection;
   QStringList::const_iterator f_iter, p_iter, cn_iter, co_iter;
   QStringList rec;
   QString p_row, c_row;
   QStringList p_fields, cn_fields, d_fields, co_fields;
   QStringList fld_ids, pos_ids, conn_ids, data_ids, color_ids;
   QString in_name("all_structures.dx");
   QString out_name("all_structures.c");
   QString struct_ref;
   QTextStream ref_stream(&struct_ref);
   QString struct_size;
   QTextStream size_stream(&struct_size);
   QString struct_norms;
   QTextStream norm_stream(&struct_norms);
   smoothNorms skinNorms;
   smoothNormsIter skinIter;
   smoothNorms objNorms;
   smoothNormsIter objIter;
   QString skin_tri_list;
   QTextStream s_tri_list(&skin_tri_list);
   QString obj_tri_list;
   QTextStream o_tri_list(&obj_tri_list);
   char space;
   float f1,f2,f3;
   glm::vec3 key;
   glm::vec3 sum;
   glm::vec3 tri_norm;
   glm::vec3 avgnorm;
   unsigned int n_pts;
   int skiplines;
   QString argstr;

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
              cout << "default input name is all_structures.dx, default output name is  all_structures.c" << endl;
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
   out_stream << endl << "#include <GL/gl.h>" << endl << endl;

   // build a table of ptrs to each structure struct and the begining and
   // end offsets for openGL vbo indexing
   ref_stream << "const GLfloat (*allStructs [])[3] = { " << endl;
   size_stream << "const int objSizes[][2] = { " << endl;
   norm_stream << "const GLfloat (*objNorms [])[3] = { " << endl;

   QString all = in_file.readAll();

     // turn file into list of rows
   QStringList rows = all.split('\n',QString::SkipEmptyParts);
   QStringList::const_iterator h_iter, iter;

    // find IDs for position and color components
   QStringList fld   = rows.filter(QRegExp("^object.+field$"));
   QStringList pos   = rows.filter(QRegExp("^component \"positions\" value.+"));
   QStringList conn  = rows.filter(QRegExp("^component \"connections\" value.+"));
   QStringList data  = rows.filter(QRegExp("^component \"data\" value.+"));
   QStringList color = rows.filter(QRegExp("^component \"colors\" value.+"));

   QStringList::const_iterator fld_iter, pos_iter, conn_iter, data_iter, color_iter;
   int num_fld   = fld.size();
   int num_pos  = pos.size();
   int num_conn = conn.size();
   int num_data = data.size();
   int num_color = color.size();

   if (fld.size() != num_pos || num_pos != num_data || num_pos != num_conn || num_pos != num_color)
   {
      cout << "Blocks are mismatched, aborting" << endl;
      exit(1);
   }

   fld_iter = fld.begin();
   pos_iter = pos.begin();
   data_iter = data.begin();
   conn_iter = conn.begin();
   color_iter = color.begin();

   for (int rows = 0; rows < num_fld; ++rows, ++fld_iter, ++pos_iter, ++data_iter, ++conn_iter)
   {
      fld_ids   += fld_iter->section(' ',1,1);
      pos_ids   += pos_iter->section(' ',3,3);
      conn_ids  += conn_iter->section(' ',3,3);
      data_ids  += data_iter->section(' ',3,3);
      color_ids += color_iter->section(' ',3,3);
   }

   QStringList::const_iterator f_id_iter = fld_ids.begin();
   QStringList::const_iterator p_id_iter = pos_ids.begin();
   QStringList::const_iterator cn_id_iter = conn_ids.begin();
//   QStringList::const_iterator d_id_iter = data_ids.begin();
//   QStringList::const_iterator co_id_iter = color_ids.begin();

    // each object "field number" class field line starts a new structure
    // not interested in sections that are not fields or not triangles
    // (need 3 coords)
   bool doing_skin = true;
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
      if (shape_val != 3)
         continue;

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
         cout << "Cannot open output file "<< out_name.toLatin1().constData() << endl;
         exit(1);
      }
      ++field_items;
      ++pos_items;
      ++conn_items;
      QString fld_num =field.at(field_items);
      fld_num.remove('"');
      fld_item = fld_num.toInt();   // object number

      conn_item = connection.at(conn_items).toInt();
      ++pos_index;
      ++conn_index;
      p_iter = rows.begin()+pos_index;
      cn_iter = rows.begin()+conn_index;

       // start/continue  building output file
      if (doing_skin)
         out_stream << "const GLfloat brainSkin" << "[][3]={" << endl;
      else
         out_stream << "const GLfloat object" << fld_item << "[][3]={" << endl;

        // look up all the points in the connections list and generate
        // the list of points we will use to create triangles that 
        // we use to draw the structures
      skiplines = 0; 
      skinNorms.clear();
      skin_tri_list.clear();
      for (int item = 0; item < conn_item ; ++item, ++cn_iter)
      {
         c_row = cn_iter->toLatin1().constData();
         cn_fields = c_row.split(' ',QString::SkipEmptyParts);
         if (cn_fields.length() == 3)
         {
            p0 = cn_fields[0].toInt();
            p1 = cn_fields[1].toInt();
            p2 = cn_fields[2].toInt();
         }
         else
         {
            cout << "Unexpected field length " << cn_fields.length() << endl;
            p0 = p1 = p2 = 0;
         }
             // vertices of triangle
         p_row = (p_iter+p0)->toLatin1().constData();
         p_fields = p_row.split(' ',QString::SkipEmptyParts);
         x0 = p_fields[0].toFloat();
         y0 = p_fields[1].toFloat();
         z0 = p_fields[2].toFloat();
         p_row = (p_iter+p1)->toLatin1().constData();
         p_fields = p_row.split(' ',QString::SkipEmptyParts);
         x1 = p_fields[0].toFloat();
         y1 = p_fields[1].toFloat();
         z1 = p_fields[2].toFloat();
         p_row = (p_iter+p2)->toLatin1().constData();
         p_fields = p_row.split(' ',QString::SkipEmptyParts);
         x2 = p_fields[0].toFloat();
         y2 = p_fields[1].toFloat();
         z2 = p_fields[2].toFloat();

         glm::vec3 s0(x0,y0,z0);
         glm::vec3 s1(x1,y1,z1);
         glm::vec3 s2(x2,y2,z2);

         if (s0 == s1 || s0 == s2 || s1 == s2)
         {
            ++skiplines;
            continue;
         }

         out_stream << "{" << x0 << "," << y0 << "," << z0 << "}," << endl;
         s_tri_list << x0 << " " << y0 << " " << z0 << endl;
         ++obj_count;
         out_stream << "{" << x1 << "," << y1 << "," << z1 << "}," << endl;
         s_tri_list << x1 << " " << y1 << " " << z1 << endl;
         ++obj_count;
         out_stream << "{" << x2 << "," << y2 << "," << z2 << "}," << endl;
         s_tri_list << x2 << " " << y2 << " " << z2 << endl;
         ++obj_count;

           // make/add to map of the normals of all triangles 
           // sharing this triangle's vertices
         tri_norm = glm::triangleNormal(s0,s1,s2);
         skinIter = skinNorms.find(s0);
         if (skinIter == skinNorms.end())
            skinNorms.insert(pair<vtxLookUp, triNorms> (vtxLookUp(s0),{tri_norm}));
         else
            skinIter->second.push_back(tri_norm);

         skinIter = skinNorms.find(s1);
         if (skinIter == skinNorms.end())
            skinNorms.insert(pair<vtxLookUp, triNorms> (vtxLookUp(s1),{tri_norm}));
         else
            skinIter->second.push_back(tri_norm);

         skinIter = skinNorms.find(s2);
         if (skinIter == skinNorms.end())
            skinNorms.insert(pair<vtxLookUp, triNorms> (vtxLookUp(s2),{tri_norm}));
         else
            skinIter->second.push_back(tri_norm);
      }
      out_stream << "};" << endl << endl;
      if (skiplines)
         cout << "found " << skiplines << " lines, not triangles, in object " << fld_item << endl;

        // finished with skin/object
      if (doing_skin)
      {
         out_stream << endl << "const int numbrainSkin = sizeof(brainSkin) / (sizeof(GLfloat)*3);" << endl << endl << "const GLfloat skinNorms[][3] = { " << endl;
      }
      else
      {
         out_stream << endl << endl << "const GLfloat object" << fld_item << "Norm" << "[][3]={" << endl;
         ref_stream << "&object" << fld_item << "[0]," << endl;
         size_stream << "{" << obj_start << "," << obj_count << "}," << endl;
         norm_stream << "&object" << fld_item << "Norm[0]," << endl;
         obj_start += obj_count;
         obj_count = 0;
      }

           // generate average shared norms list
      s_tri_list.seek(0);
      QStringList vtxinfo = skin_tri_list.split('\n',QString::SkipEmptyParts);
      while (!s_tri_list.atEnd())
      {
         s_tri_list >> f1 >> space >> f2 >> space >> f3;
         key = glm::vec3(f1,f2,f3);
         skinIter = skinNorms.find(key);
         if (skinIter != skinNorms.end())
         {
             int num_shared = 0;
             sum = glm::vec3(0.0);
             for (n_pts = 0 ; n_pts < skinIter->second.size(); ++n_pts)
             {
                sum = sum + skinIter->second[n_pts];
                ++num_shared;
             }
             avgnorm = glm::normalize(sum); 
             if (glm::isnan(avgnorm[1]))
             {
                unsigned int t_pts;
                avgnorm = glm::vec3(0.0);
                cout << endl << "NAN problem" << endl;
                for (t_pts = 0 ; t_pts < skinIter->second.size(); ++t_pts)
                   cout << glm::to_string(skinIter->second[t_pts]) << endl;
                cout << endl;
             }
             out_stream << "{" << avgnorm[0] << "," << avgnorm[1] << "," << avgnorm[2] << "}," << endl;
         }
         s_tri_list.readLine(); // next line
      }
      out_stream << "};" << endl << endl;

      if (doing_skin)
      {
         out_stream << "const int numskinNorms = sizeof(skinNorms)/(sizeof(GLfloat)*3);" << endl << endl;
         obj_count = 0;
         doing_skin = false;
      }
   }


   ref_stream << "};" << endl;
   size_stream << "};" << endl;
   norm_stream << "};" << endl;
   out_stream << struct_ref << endl;
   out_stream << endl << "const int numStructs = sizeof(allStructs)/sizeof(GLfloat*);" << endl << endl;
   out_stream << struct_size << endl;
   out_stream << struct_norms << endl;
   out_stream << endl << "const int numNormStructs = sizeof(objNorms)/sizeof(GLfloat*);" << endl << endl;

   out_stream.flush();

   exit(0);
}

