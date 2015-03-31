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

adPvaServerSupport_LIBS += Com pvData pvAccess pvDatabase


# ------------------------
# Build an IOC Application
# ------------------------

PROD_IOC = adPvaServer

# adPvaServer.dbd will be installed into <top>/dbd
DBD += adPvaServer.dbd

# adPvaServer.dbd will be created from these files
adPvaServer_DBD += base.dbd

# adPvaServer_registerRecordDeviceDriver.cpp will be created
adPvaServer_SRCS += adPvaServer_registerRecordDeviceDriver.cpp

# These two lines are needed for non-vxWorks builds, such as Linux
adPvaServer_SRCS_DEFAULT += adPvaServerMain.cpp
adPvaServer_SRCS_vxWorks += -nil-

# Add locally compiled object code
#adPvaServer_SRCS +=

# The following adds object code from base/src/vxWorks
adPvaServer_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# This line says that this IOC Application depends on the
# xxx Support Module
adPvaServer_LIBS += Com pvData pvAccess pvDatabase

# We need to link this IOC Application against the EPICS Base libraries
adPvaServer_LIBS += $(EPICS_BASE_IOC_LIBS)



# ---------------------------------------------------

#include $(AREA_DETECTOR)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
