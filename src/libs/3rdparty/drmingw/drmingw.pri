
LIBDRMINGWPATH     = $$PWD/bin
copytarget.path    = $$LIBDRMINGWPATH
copytarget.files  += \
    dbghelp.dll \
    exchndl.dll \
    mgwhelp.dll \
    symsrv.dll \
    symsrv.yes

#win32: copytarget.files ~= s,/,\\,g

## === copy compiler for makefile ===

DirSep = /
win32: DirSep = \\

for(f,copytarget.files) tmp += $$LIBDRMINGWPATH$$DirSep$${f} ## make absolute paths
copycompiler.input        = tmp

isEmpty(DESTDIR):DESTDIR=.
copycompiler.output       = $$DESTDIR$$DirSep${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}
copycompiler.commands     = $(COPY_FILE) ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copycompiler.CONFIG       = no_link no_clean

## other CONFIG options are: depends explicit_dependencies target_predeps

copycompiler.variable_out = QMAKE_DISTCLEAN
QMAKE_EXTRA_COMPILERS += copycompiler

## == makefile copy target ===
copyfiles.recurse_target = compiler_copycompiler_make_all
copyfiles.depends        = $$copyfiles.recurse_target
copyfiles.CONFIG        += recursive

QMAKE_EXTRA_TARGETS += copyfiles
PRE_TARGETDEPS += copyfiles
	
INCLUDEPATH += $$PWD/include
LIBS += -lexchndl
