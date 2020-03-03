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


#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <getopt.h>
#include <iostream>

#include <glm/glm.hpp>
#include "objloader.hpp"

using namespace std;

int main(int argc,char *argv[])
{
	vector<glm::vec3> out_vertices; 
	vector<glm::vec3> out_normals;
   int cmd;
   char *tmpstr;
   string in_name("skins.obj");
   string out_name("skins.fromblend.c");
   string vname("brainSkin");
   string nname("skinNorms");

   const struct option opts[] = {
                                   {"in", required_argument, NULL, 'i'},
                                   {"out", required_argument, NULL, 'o'},
                                   {"vname", required_argument, NULL, 'v'},
                                   {"nname", required_argument, NULL, 'n'},
                                   { 0,0,0,0} };

   while ((cmd = getopt_long(argc, argv, "", opts, NULL )) != -1)
   {
      switch (cmd)
      {
         case 'i':
               tmpstr=optarg;
               if (!tmpstr)
               {
                  cout << "Input file is missing, aborting. . .\n";
                  exit(1);
               }
               in_name = tmpstr;
               break;
         case 'o':
               tmpstr=optarg;
               if (!tmpstr)
               {
                  cout << "Output file is missing, aborting. . .\n";
                  exit(1);
               }
               out_name = tmpstr;
               break;
         case 'v':
               tmpstr=optarg;
               if (!tmpstr)
               {
                  cout << "Vertex array name is missing, aborting. . .\n";
                  exit(1);
               }
               vname = tmpstr;
               break;
         case 'n':
               tmpstr=optarg;
               if (!tmpstr)
               {
                  cout << "Normal array name is missing, aborting. . .\n";
                  exit(1);
               }
               nname = tmpstr;
               break;
        default:
              cout << "usage: objloader [--in inname] [--out outname] [--vname vertexarrayname] [--nname normalarrayname]" << endl;
              cout << "default input name is skins.obj, default output name is  skins.fromblend.c" << endl << "default vertex array name is brainSkin, default normal array name is skinNorms" << endl;
              exit(1);
              break;
      }
   }


	vector<unsigned int> vertexIndices, normalIndices;
	vector<glm::vec3> temp_vertices; 
	vector<glm::vec3> temp_normals;

	FILE * file = fopen(in_name.c_str(), "r");
	if( file == NULL )
   {
		printf("Can't open file\n");
		return false;
	}

	while(1)
   {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.
      else if ( strcmp( lineHeader, "v" ) == 0 ) // else parse lineHeader
      {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			temp_vertices.push_back(vertex);
		}
      else if ( strcmp( lineHeader, "vn" ) == 0 )
      {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}
      else if ( strcmp( lineHeader, "f" ) == 0 )
      {
			unsigned int vertexIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2] );
			if (matches != 6)
         {
				printf("File has cases not handledl\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
      else
      {
	  		// comment or something we don't know
			char skipit[1000];
			fgets(skipit, 1000, file);
		}
	}

	// For each vertex of each triangle
	for( unsigned int i=0; i<vertexIndices.size(); i++ )
   {
		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int normalIndex = normalIndices[i];
		
		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
		glm::vec3 normal = temp_normals[ normalIndex-1 ];
		
		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_normals .push_back(normal);
	
	}
   fclose(file);

	FILE * out = fopen(out_name.c_str(), "w");
   fprintf(out, "#include <GL/gl.h>\n");

   fprintf(out, "const GLfloat %s[][3]={\n", vname.c_str());

   vector<glm::vec3>::iterator verts;
   vector<glm::vec3>::iterator norms;

   for (verts = out_vertices.begin(); verts != out_vertices.end(); ++verts)
      fprintf(out,"{%2.1f,%2.1f,%2.1f},\n", verts->x, verts->y, verts->z);

    fprintf(out,"};\n\nconst int num%s = sizeof(%s) / (sizeof(GLfloat)*3);\n\nconst GLfloat %s[][3] = {\n",vname.c_str(),vname.c_str(),nname.c_str());

   for (norms = out_normals.begin(); norms != out_normals.end(); ++norms)
      fprintf(out,"{%2.1f,%2.1f,%2.1f},\n", norms->x, norms->y, norms->z);

   fprintf(out,"};\n\nconst int num%s = sizeof(%s)/(sizeof(GLfloat)*3);\n\n",nname.c_str(),nname.c_str()); 
   fclose(out);

	exit(0);
}
