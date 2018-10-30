TEMPLATE = app
CONFIG -= qt
CONFIG -= c++11
CONFIG -= c++14

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++

# universal arguments
QMAKE_CXXFLAGS += -std=c++14 -pipe
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti

QMAKE_CXXFLAGS_DEBUG += -O0 -g3
QMAKE_CXXFLAGS_RELEASE += -Os


#QMAKE_CXXFLAGS_RELEASE += -fno-threadsafe-statics
QMAKE_CXXFLAGS_RELEASE += -fno-asynchronous-unwind-tables
#QMAKE_CXXFLAGS_RELEASE += -fstack-protector-all
QMAKE_CXXFLAGS_RELEASE += -fstack-protector-strong

# optimizations
QMAKE_CXXFLAGS_RELEASE += -fdata-sections
QMAKE_CXXFLAGS_RELEASE += -ffunction-sections
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

# libraries
LIBS += -lrt

# defines
debug:DEFINES += INTERRUPTED_WRAPPER
DEFINES += CATALOG_NAME=director

#LIBS += -lpthread
experimental {
#QMAKE_CXXFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -nostdinc
INCLUDEPATH += /usr/include/x86_64-linux-musl
INCLUDEPATH += /usr/include/c++/v1
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/x86_64-linux-gnu
QMAKE_LFLAGS += -L/usr/lib/x86_64-linux-musl -dynamic-linker /lib/ld-musl-x86_64.so.1
LIBS += -lc++
}

PDTK = ../pdtk
INCLUDEPATH += $$PDTK

SOURCES = main.cpp \
    directorcore.cpp \
    directorconfigclient.cpp \
    configclient.cpp \
    jobcontroller.cpp \
    dependencysolver.cpp \
    eventpending.cpp \
    servicecheck.cpp \
    string_helpers.cpp \
    $$PDTK/application.cpp \
    $$PDTK/socket.cpp \
    $$PDTK/childprocess.cpp \
    $$PDTK/cxxutils/vfifo.cpp \
    $$PDTK/cxxutils/configmanip.cpp \
    $$PDTK/cxxutils/syslogstream.cpp \
    $$PDTK/cxxutils/translate.cpp \
    $$PDTK/cxxutils/mountpoint_helpers.cpp \
    $$PDTK/specialized/fstable.cpp \
    $$PDTK/specialized/peercred.cpp \
    $$PDTK/specialized/procstat.cpp \
    $$PDTK/specialized/proclist.cpp \
    $$PDTK/specialized/eventbackend.cpp \
    $$PDTK/specialized/FileEvent.cpp \
    $$PDTK/specialized/PollEvent.cpp \
    $$PDTK/specialized/ProcessEvent.cpp \
    $$PDTK/specialized/TimerEvent.cpp

tui:SOURCES += \
    $$PDTK/tui/widget.cpp \
    $$PDTK/tui/window.cpp \
    $$PDTK/tui/screen.cpp \
    $$PDTK/tui/tuiutils.cpp \
    $$PDTK/tui/layout.cpp \
    $$PDTK/tui/layoutitem.cpp \
    $$PDTK/tui/event.cpp \
    $$PDTK/tui/keyboard.cpp

HEADERS += \
    directorcore.h \
    directorconfigclient.h \
    configclient.h \
    jobcontroller.h \
    dependencysolver.h \
    eventpending.h \
    servicecheck.h \
    string_helpers.h \
    $$PDTK/object.h \
    $$PDTK/application.h \
    $$PDTK/socket.h \
    $$PDTK/childprocess.h \
    $$PDTK/cxxutils/vfifo.h \
    $$PDTK/cxxutils/configmanip.h \
    $$PDTK/cxxutils/syslogstream.h \
    $$PDTK/cxxutils/translate.h \
    $$PDTK/cxxutils/mountpoint_helpers.h \
    $$PDTK/cxxutils/posix_helpers.h \
    $$PDTK/cxxutils/socket_helpers.h \
    $$PDTK/cxxutils/error_helpers.h \
    $$PDTK/cxxutils/hashing.h \
    $$PDTK/cxxutils/colors.h \
    $$PDTK/cxxutils/misc_helpers.h \
    $$PDTK/cxxutils/pipedspawn.h \
    $$PDTK/cxxutils/signal_helpers.h \
    $$PDTK/specialized/fstable.h \
    $$PDTK/specialized/peercred.h \
    $$PDTK/specialized/procstat.h \
    $$PDTK/specialized/proclist.h \
    $$PDTK/specialized/capabilities.h \
    $$PDTK/specialized/eventbackend.h \
    $$PDTK/specialized/FileEvent.h \
    $$PDTK/specialized/PollEvent.h \
    $$PDTK/specialized/ProcessEvent.h \
    $$PDTK/specialized/TimerEvent.h


tui:HEADERS += \
    $$PDTK/tui/widget.h \
    $$PDTK/tui/window.h \
    $$PDTK/tui/screen.h \
    $$PDTK/tui/tuiutils.h \
    $$PDTK/tui/layout.h \
    $$PDTK/tui/layoutitem.h \
    $$PDTK/tui/tuitypes.h \
    $$PDTK/tui/sizepolicy.h \
    $$PDTK/tui/event.h \
    $$PDTK/tui/keyboard.h
