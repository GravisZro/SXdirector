TEMPLATE = app
CONFIG -= qt
CONFIG += c++14
CONFIG += strict_c++
CONFIG += exceptions_off
CONFIG += rtti_off

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -fconstexpr-depth=256
#QMAKE_CXXFLAGS += -fconstexpr-steps=900000000

# universal arguments
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
#DEFINES += __CONTINUOUS_INTEGRATION__
#DEFINES += DISABLE_INTERRUPTED_WRAPPER
DEFINES += CATALOG_NAME=director
#DEFINES += SINGLE_THREADED_APPLICATION
#DEFINES += FORCE_POSIX_TIMERS
#DEFINES += FORCE_POSIX_POLL
#DEFINES += FORCE_POSIX_MUTEXES
#DEFINES += FORCE_PROC_POLLING

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

PUT = ../put
INCLUDEPATH += $$PUT

SOURCES = main.cpp \
    directorcore.cpp \
    directorconfigclient.cpp \
    configclient.cpp \
    jobcontroller.cpp \
    dependencysolver.cpp \
    eventpending.cpp \
    servicecheck.cpp \
    string_helpers.cpp \
    $$PUT/application.cpp \
    $$PUT/socket.cpp \
    $$PUT/childprocess.cpp \
    $$PUT/cxxutils/vfifo.cpp \
    $$PUT/cxxutils/configmanip.cpp \
    $$PUT/cxxutils/syslogstream.cpp \
    $$PUT/cxxutils/translate.cpp \
    $$PUT/cxxutils/stringtoken.cpp \
    $$PUT/specialized/eventbackend.cpp \
    $$PUT/specialized/mutex.cpp \
    $$PUT/specialized/peercred.cpp \
    $$PUT/specialized/procstat.cpp \
    $$PUT/specialized/proclist.cpp \
    $$PUT/specialized/fstable.cpp \
    $$PUT/specialized/mountpoints.cpp \
    $$PUT/specialized/FileEvent.cpp \
    $$PUT/specialized/PollEvent.cpp \
    $$PUT/specialized/ProcessEvent.cpp \
    $$PUT/specialized/TimerEvent.cpp

tui:SOURCES += \
    $$PUT/tui/widget.cpp \
    $$PUT/tui/window.cpp \
    $$PUT/tui/screen.cpp \
    $$PUT/tui/tuiutils.cpp \
    $$PUT/tui/layout.cpp \
    $$PUT/tui/layoutitem.cpp \
    $$PUT/tui/event.cpp \
    $$PUT/tui/keyboard.cpp

HEADERS += \
    directorcore.h \
    directorconfigclient.h \
    configclient.h \
    jobcontroller.h \
    dependencysolver.h \
    eventpending.h \
    servicecheck.h \
    string_helpers.h \
    $$PUT/object.h \
    $$PUT/application.h \
    $$PUT/socket.h \
    $$PUT/childprocess.h \
    $$PUT/cxxutils/vfifo.h \
    $$PUT/cxxutils/configmanip.h \
    $$PUT/cxxutils/syslogstream.h \
    $$PUT/cxxutils/translate.h \
    $$PUT/cxxutils/stringtoken.h \
    $$PUT/cxxutils/posix_helpers.h \
    $$PUT/cxxutils/socket_helpers.h \
    $$PUT/cxxutils/error_helpers.h \
    $$PUT/cxxutils/hashing.h \
    $$PUT/cxxutils/colors.h \
    $$PUT/cxxutils/misc_helpers.h \
    $$PUT/cxxutils/pipedspawn.h \
    $$PUT/cxxutils/signal_helpers.h \
    $$PUT/specialized/osdetect.h \
    $$PUT/specialized/eventbackend.h \
    $$PUT/specialized/mutex.h \
    $$PUT/specialized/peercred.h \
    $$PUT/specialized/procstat.h \
    $$PUT/specialized/proclist.h \
    $$PUT/specialized/fstable.h \
    $$PUT/specialized/mountpoints.h \
    $$PUT/specialized/capabilities.h \
    $$PUT/specialized/FileEvent.h \
    $$PUT/specialized/PollEvent.h \
    $$PUT/specialized/ProcessEvent.h \
    $$PUT/specialized/TimerEvent.h

tui:HEADERS += \
    $$PUT/tui/widget.h \
    $$PUT/tui/window.h \
    $$PUT/tui/screen.h \
    $$PUT/tui/tuiutils.h \
    $$PUT/tui/layout.h \
    $$PUT/tui/layoutitem.h \
    $$PUT/tui/tuitypes.h \
    $$PUT/tui/sizepolicy.h \
    $$PUT/tui/event.h \
    $$PUT/tui/keyboard.h
