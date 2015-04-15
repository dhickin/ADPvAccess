TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build a Diamond Support Module
# -------------------------------

LIBRARY_IOC += adPvaServerSupport

# xxxRecord.dbd will be installed into <top>/dbd
DBD += adPvaServerSupport.dbd

# The following are compiled and added to the support library
adPvaServerSupport_SRCS += adPvaServer.cpp
adPvaServerSupport_SRCS += ndarray.cpp
adPvaServerSupport_SRCS += NDArrayRecord.cpp

adPvaServerSupport_LIBS += Com pvData pvAccess pvDatabase nt


# ---------------------------------------------------

#include $(AREA_DETECTOR)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
