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
QMAKE_CXXFLAGS_RELEASE += -fstack-clash-protection

# optimizations
QMAKE_CXXFLAGS_RELEASE += -fdata-sections
QMAKE_CXXFLAGS_RELEASE += -ffunction-sections
QMAKE_CXXFLAGS_RELEASE += -flto
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections
QMAKE_LFLAGS_RELEASE += -flto

# libraries
LIBS += -lrt
LIBS += -lpthread

# defines
QMAKE_CXXFLAGS_DEBUG += -DDEBUG
QMAKE_CXXFLAGS_RELEASE += -DRELEASE
#DEFINES += __CONTINUOUS_INTEGRATION__

#DEFINES += DISABLE_INTERRUPTED_WRAPPER
#DEFINES += SINGLE_THREADED_APPLICATION
#DEFINES += FORCE_POSIX_TIMERS
#DEFINES += FORCE_POSIX_POLL
#DEFINES += FORCE_POSIX_MUTEXES
#DEFINES += FORCE_PROCESS_POLLING

SOURCES += main.cpp \
    directorcore.cpp \
    directorconfigclient.cpp \
    configclient.cpp \
    jobcontroller.cpp \
    jobcontainer.cpp \
    dependencysolver.cpp \
    eventpending.cpp \
    servicecheck.cpp \
    string_helpers.cpp

units:SOURCES += \
    units/jobcontainer_unit.cpp \
    units/process_control_unit.cpp

HEADERS += \
    directorcore.h \
    directorconfigclient.h \
    configclient.h \
    jobcontroller.h \
    jobcontainer.h \
    dependencysolver.h \
    eventpending.h \
    servicecheck.h \
    string_helpers.h

include(put/put.pri)
