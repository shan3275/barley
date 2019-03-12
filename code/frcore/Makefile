#
#
# OCTEON SDK
#
# Copyright (c) 2010 Cavium Networks. All rights reserved.
#
# This file, which is part of the OCTEON SDK which also includes the
# OCTEON SDK Package from Cavium Networks, contains proprietary and
# confidential information of Cavium Networks and in some cases its
# suppliers. 
#
# Any licensed reproduction, distribution, modification, or other use of
# this file or the confidential information or patented inventions
# embodied in this file is subject to your license agreement with Cavium
# Networks. Unless you and Cavium Networks have agreed otherwise in
# writing, the applicable license terms "OCTEON SDK License Type 5" can be found 
# under the directory: $OCTEON_ROOT/components/driver/licenses/
#
# All other use and disclosure is prohibited.
#
# Contact Cavium Networks at info@caviumnetworks.com for more information.
#
################################################################################



#
# FRCORE simple exec application Makefile
#

# The driver files are kept in this location.
DRIVER_ROOT = $(OCTEON_ROOT)/components/driver

# The application specific files are located here.
APP_ROOT = $(PWD)

# The application binary is copied here after compilation.
BINDIR = $(DRIVER_ROOT)/bin

#  application specification
TARGET        =  frcore

all: $(TARGET)_version.c $(TARGET) default

#  default target
default: $(TARGET)
	cp $^ $^.strip;
	mipsisa64-octeon-elf-strip $^.strip;
	@ln -fs $(PWD)/$^.strip $(BINDIR)/$^.strip;

#  standard common Makefile fragment
include $(OCTEON_ROOT)/common.mk

#  global debug setting for compile
# DEBUG_FLAGS = -DOCTEON_DEBUG_LEVEL=2 -g
# DEBUG_FLAGS = -DOCTEON_DEBUG_LEVEL=0 -g
DEBUG_FLAGS = -g

COMPILE += -O2 $(DEBUG_FLAGS) -DMAKE_CORE

COMPILE += -DSDKVER="\"`oct-version | sed -n "s/.* version \(.*\)/\1/p"`\""

COMPILE += -Wall -Wa,-a=$@.list -Wno-unused-parameter -O2 -fno-strict-aliasing $(DEBUG_FLAGS) $(CORE_MASKS) $(CC_FLAGS) $(INCLUDE_DIRS) $(DRIVER_DEFINES)


dir := $(OCTEON_ROOT)/components/driver
include $(dir)/driver.mk

COMPILE += $(OCTDRVFLAGS)


#  include required application common
dir := $(OCTEON_ROOT)/applications/pci-core-app/common
include $(dir)/cvmcs-common.mk

#  include required components common sanity code
dir := $(OCTEON_ROOT)/components/common
include $(dir)/common.mk



#  include required Core driver Makefile fragments
dir := $(DRIVER_ROOT)/core
include $(dir)/core-drv.mk

#  include required simple executive Makefile fragments
dir := $(OCTEON_ROOT)/executive
include $(dir)/cvmx.mk


#Action for making frcore
OBJS =  $(OBJ_DIR)/frcore_stat.o \
	$(OBJ_DIR)/frcore_phy.o \
	$(OBJ_DIR)/frcore_ip.o \
	$(OBJ_DIR)/frcore_udp.o \
	$(OBJ_DIR)/frcore_tcp.o \
	$(OBJ_DIR)/frcore_pkt.o \
	$(OBJ_DIR)/frcore_ssn.o \
	$(OBJ_DIR)/frcore_tcpflowrec.o \
        $(OBJ_DIR)/frcore_version.o \
	$(OBJ_DIR)/frcore_main.o   \
	$(OBJ_DIR)/frcore_filter.o \
	$(OBJ_DIR)/frcore_twotuple.o \
	$(OBJ_DIR)/frcore_init.o   \
	$(OBJ_DIR)/frcore_dma.o \
	$(OBJ_DIR)/frcore_cmd.o \
	$(OBJ_DIR)/frcore_chan.o \
	$(OBJ_DIR)/frcore_fr.o \
	$(OBJ_DIR)/frcore_rfc.o \
	$(OBJ_DIR)/frcore_acl.o \
	$(OBJ_DIR)/frcore_rule.o \
	$(OBJ_DIR)/frcore_bmm.o \
	$(OBJ_DIR)/frcore_misc.o \
	$(OBJ_DIR)/frcore_alg.o \
	$(OBJ_DIR)/frcore_vlan_check.o \
	$(OBJ_DIR)/frcore_timestamp_check.o \
	$(OBJ_DIR)/frcore.o \
	$(OBJ_DIR)/frcore_mac_statistics.o \
	$(OBJ_DIR)/frcore_ipc.o

CFLAGS_LOCAL = -I$(OCTEON_ROOT)/applications/pci-core-app/common \
	       -I$(OCTEON_ROOT)/target/include -I../include \
		-I$(DRIVER_ROOT)/common -W -Wall -Wno-unused-parameter

include $(OCTEON_ROOT)/application.mk


#  clean target
clean: 
	rm -rf $(TARGET) $(OBJ_DIR) *.o *.d *~ $(TARGET).strip
	rm -rf $(TARGET)_version.c

include $(FRCDIR)/version.mk


NUM_PROCESSORS=8

TCPDUMP_FILES=-I 0:input.data


ifndef OCTEON_SIM_PACKET_TCP_PORT
OCTEON_SIM_PACKET_TCP_PORT=2000
endif

sim: $(TARGET)
	oct-packet-io -p ${OCTEON_SIM_PACKET_TCP_PORT} -c 1 -t output.data ${TCPDUMP_FILES} &
	oct-sim $(TARGET) -quiet -serve=${OCTEON_SIM_PACKET_TCP_PORT} -maxcycles=120000000 -numcores=${NUM_PROCESSORS} ${PASSTHROUGH_SIM_MODES}
	

# $Id: Makefile 48226 2010-04-02 21:42:18Z panicker $
