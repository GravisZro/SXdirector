TEMPLATE = app
CONFIG -= qt
CONFIG -= c++11
CONFIG -= c++14

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++

# universal arguments
QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -pipe -Os
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti

#QMAKE_CXXFLAGS += -fno-threadsafe-statics
QMAKE_CXXFLAGS += -fno-asynchronous-unwind-tables
#QMAKE_CXXFLAGS += -fstack-protector-all
QMAKE_CXXFLAGS += -fstack-protector-strong

# optimizations
QMAKE_CXXFLAGS += -fdata-sections
QMAKE_CXXFLAGS += -ffunction-sections
QMAKE_LFLAGS += -Wl,--gc-sections

# libraries
LIBS += -lrt

# defines
debug:DEFINES += INTERRUPTED_WRAPPER

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
    string_helpers.cpp \
    exitpending.cpp \
    $$PDTK/application.cpp \
    $$PDTK/socket.cpp \
    $$PDTK/cxxutils/vfifo.cpp \
    $$PDTK/cxxutils/configmanip.cpp \
    $$PDTK/cxxutils/syslogstream.cpp \
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
    string_helpers.h \
    $$PDTK/object.h \
    $$PDTK/application.h \
    $$PDTK/socket.h \
    $$PDTK/cxxutils/vfifo.h \
    $$PDTK/cxxutils/configmanip.h \
    $$PDTK/cxxutils/syslogstream.h \
    $$PDTK/cxxutils/posix_helpers.h \
    $$PDTK/cxxutils/socket_helpers.h \
    $$PDTK/cxxutils/error_helpers.h \
    $$PDTK/cxxutils/hashing.h \
    $$PDTK/cxxutils/colors.h \
    $$PDTK/cxxutils/misc_helpers.h \
    $$PDTK/cxxutils/pipedfork.h \
    $$PDTK/cxxutils/signal_helpers.h \
    $$PDTK/specialized/peercred.h \
    $$PDTK/specialized/procstat.h \
    $$PDTK/specialized/proclist.h \
    $$PDTK/specialized/capabilities.h \
    $$PDTK/specialized/eventbackend.h \
    $$PDTK/specialized/FileEvent.h \
    $$PDTK/specialized/PollEvent.h \
    $$PDTK/specialized/ProcessEvent.h \
    $$PDTK/specialized/TimerEvent.h \
    exitpending.h


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
