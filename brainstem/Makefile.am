
MSWIN_DIR=mswin

#DEBUG_OR_NOT= -ggdb3
DEBUG_OR_NOT= -O2
AUTOMAKE_OPTIONS= -Wno-portability

AM_CXXFLAGS=
AM_CPPFLAGS = $(DEBUG_OR_NOT) -DVERSION=\"$(VERSION)\"
AM_CFLAGS = $(DEBUG_OR_NOT) -Wall -std=c99 

bin_PROGRAMS = brainstem brainstem.exe
noinst_PROGRAMS = stem2gl obj2c outlines2obj
dist_bin_SCRIPTS = 
dist_pkgdata_DATA=brainstem.desktop brainstem.png allexp-100_ctl.csv allexp-100_cco2.csv  allexp-100_vco2.csv
dist_doc_DATA = brainstem.odt brainstem.pdf ChangeLog HOWTO_BUILD_FOR_WIN COPYING LICENSE COPYRIGHTS README.md

BUILT_SOURCES = ui_brainstem.h qrc_brainstem.cpp moc_brainstem.cpp moc_brainstemgl.cpp moc_helpbox.cpp ui_helpbox.h all_structures.c sphere.c

brainstem_LDADD = -lX11 -lGL -lm 

brainstem_code = main.cpp \
                    brainstemgl.cpp \
						  brainstemgl.glsl \
                    brainstemgl.h \
                    brain_impl.cpp \
                    brainstem.cpp \
						  helpbox.cpp \
                    outlines.c \
                    atlasnames.c \
                    brainstemgl.h \
                    brainstem.h \
                    helpbox.h \
                    brainstem.pro \
                    brainstem.qrc \
                    brainstem.png \
                    phrenic_scaled_norm_transp_b.png \
                    phrenic_scaled_norm_transp_w.png \
                    phrenic_scaled_norm_transp_green.png \
                    brainstem.ui \
                    helpbox.ui \
						  sphere.object \
						  all_structures.dx

brainstem_SOURCES = $(brainstem_code) $(BUILT_SOURCES)

stem2gl_SOURCES = stem2gl.cpp stem2gl.pro
obj2c_SOURCES = obj2c.cpp objloader.hpp
outlines2obj_SOURCES = outlines2obj.cpp outlines2obj.pro

brainstem_exe_SOURCES = $(brainstem_SOURCES) brainstem.pro

# some of the clean files are built as part of the build for windows.
# they need to be in the clean list so making the dist succeeds
CLEANFILES = ${BUILT_SOURCES} \
				 $(MSWIN_DIR)/*.o \
				Makefile.qt \
				Makefile_win.qt \
				brainstem_plugin_import.cpp \
				object_script.brainstem 

EXTRA_DIST = debian \
				 brainstem.exe \
				 ffmpeg.exe 

brainstem_CXXFLAGS = $(DEBUG_OR_NOT) -Wall -std=gnu++17 `pkg-config --cflags Qt5Gui Qt5Core Qt5Widgets Qt5OpenGL` -m64 -pipe -Wall -W -D_REENTRANT -fPIC ${DEFINES}
brainstem_LDFLAGS = `pkg-config --libs Qt5Gui Qt5Core Qt5Widgets Qt5OpenGL Qt5PrintSupport`  -lGL -lpthread  -lqscintilla2_qt5

stem2gl_CXXFLAGS = $(DEBUG_OR_NOT) -Wall -std=gnu++17 `pkg-config --cflags Qt5Gui Qt5Core Qt5Widgets Qt5OpenGL` -m64 -pipe -Wall -W -D_REENTRANT -fPIC ${DEFINES}
stem2gl_LDFLAGS = `pkg-config --libs Qt5Gui Qt5Core Qt5Widgets Qt5OpenGL Qt5PrintSupport`  -lGL -lpthread 

obj2c_CXXFLAGS = $(DEBUG_OR_NOT) -Wall -std=gnu++17 -Wall -W -D_REENTRANT -fPIC ${DEFINES}

outlines2obj_CXXFLAGS = $(DEBUG_OR_NOT) -Wall -std=gnu++17 -Wall -W -D_REENTRANT -fPIC ${DEFINES}

moc_%.cpp: %.h
	@MOC@ -o $@ $<

ui_%.h: %.ui
	@UIC@ -o $@ $<

qrc_%.cpp: %.qrc
	@RCC@ -o $@ $<

all_structures.c: all_structures.dx stem2gl
	./stem2gl --in $(srcdir)/all_structures.dx

sphere.c: sphere.object obj2c
	./obj2c --in $(srcdir)/sphere.object --out sphere.c --vname sphereVert --nname sphereNorm

mswin:
	@mkdir -p $(MSWIN_DIR)

Makefile.qt: Makefile brainstem.pro outlines2obj.pro stem2gl.pro
	qmake $(srcdir)/brainstem.pro -r 'DEFINES+=VERSION=\\\"$(VERSION)\\\"' 'DEFINES+=DEBUG_OR_NOT=\\\"$(DEBUG_OR_NOT)\\\"'

Makefile_win.qt: Makefile
	x86_64-w64-mingw32.static-qmake-qt5 $(srcdir)/brainstem.pro -r -spec win32-g++ 'DEFINES+=VERSION=\\\"$(VERSION)\\\"'

brainstem.exe$(EXEEXT): mswin Makefile_win.qt $(brainstem_SOURCES)
 ifdef MXE_QMAKE
	make -f Makefile_win.qt
 else
	@echo MXE environment not installed, windows program not built
 endif

checkin_files = $(brainstem_code) $(stem2gl_SOURCES) $(obj2c_SOURCES) $(outlines2obj_SOURCES) $(dist_doc_DATA) Makefile.am configure.ac

checkin_release:
	git add $(checkin_files) && git commit -uno -S -m "Release files for version $(VERSION)"

checkpoint:
	git add $(checkin_files) Makefile.am configure.ac && git commit -uno -S -q -m "checkpoint files for version $(VERSION)"

checkpoint_withcomment:
	git add $(checkin_files) && git commit -uno -S -q


deb:
	@echo 'Making debian packages'
	make distdir &&\
	cd $(distdir) &&\
	dpkg-buildpackage -Zgzip -uc -us && \
	cd .. && \
	rm -rf $(distdir)

