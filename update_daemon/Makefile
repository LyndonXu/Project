#CUR_DIR = ../../
#ifeq ($(PARAM_FILE), ) 
#    PARAM_FILE:=$(CUR_DIR)Makefile.param
#    include $(PARAM_FILE)
#endif

DEBUG ?= 1

SYS_BIT = $(shell getconf LONG_BIT)

#file type
FILETYPE = cpp

#tool chain
#CROSS = mipsel-linux-
CC = $(CROSS)gcc
CXX = $(CROSS)g++
AR = $(CROSS)ar


ifeq ($(CROSS), )
JSON_DIR = ../lib_x86/json-c/
else
JSON_DIR = ../lib_target/json-c/
endif

ifeq ($(CROSS), )
INSTALL_DIR = ../lib_x86/
else
INSTALL_DIR = ../lib_target/
endif


#include directory
INC_FLAGS = -I ./inc
INC_FLAGS += -I $(JSON_DIR)include
INC_FLAGS += -I $(INSTALL_DIR)

PROJECT = $(notdir $(CURDIR))


#compile parameters
ifeq ($(DEBUG), 1)
CDEF += -D_DEBUG
endif

CDEF += -DPROJECT_NAME=\"$(PROJECT)\"

ifeq ($(CROSS), )
CDEF += -DHAS_CROSS=0
else
CDEF += -DHAS_CROSS=1
endif

ifeq ($(CROSS), )
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g3
else
CFLAGS += -O2
endif
else
CFLAGS += -O2
endif



CFLAGS += -fPIC -Wall $(INC_FLAGS) $(CDEF)
ifeq ($(CROSS), )
ifeq ($(SYS_BIT), 64)
CFLAGS += -m32 
endif
endif


LIBDIR = -L ./
LIBDIR += -L $(JSON_DIR)lib
LIBDIR += -L $(INSTALL_DIR)


LIBS = $(LIBDIR) -lcommon
LIBS += -lpthread -ljson-c


ifeq ($(FILETYPE), cpp) 
COMPILE = $(CXX)
else
COMPILE = $(CC)
endif

COMPILE.$(FILETYPE) = $(COMPILE) $(CFLAGS) -c
LINK.$(FILETYPE) = $(COMPILE) $(CFLAGS)

#lib source
LIB_SRC = $(wildcard lib/*.$(FILETYPE))
LIB_OBJ = $(LIB_SRC:%.$(FILETYPE)=%.o)

#target source
SRC = $(wildcard *.$(FILETYPE))
SRC += $(wildcard src/*.$(FILETYPE))
OBJ = $(SRC:%.$(FILETYPE)=%.o)

#test source
TEST_SRC = $(wildcard src_test/*.$(FILETYPE))
#TEST_SRC += $(shell ls src/*.$(FILETYPE) | grep -v "main.c")
TEST_OBJ = $(TEST_SRC:%.$(FILETYPE)=%.o)


RM_OBJ += $(LIB_OBJ)
RM_OBJ += $(OBJ)
RM_OBJ += $(TEST_OBJ)


#include source
INC = $(wildcard inc/*.h)

LIB_TARGET = lib$(PROJECT).so
TARGET = $(PROJECT)
TEST_TARGET = test_$(PROJECT)

RM_TARGET += $(LIB_TARGET)
RM_TARGET += $(TARGET)
RM_TARGET += $(TEST_TARGET)

RM_FILE += $(RM_OBJ)
RM_FILE += $(RM_TARGET)

.PHONY : clean all

all : $(TARGET) test
$(TARGET) : $(OBJ)
	$(LINK.$(FILETYPE)) -o $@ $^ $(LIBS)

lib : $(LIB_TARGET)
$(LIB_TARGET) : $(LIB_OBJ)
	$(LINK.$(FILETYPE)) -o $@ $^ -shared
	@make libinstall

test : $(TEST_TARGET)
$(TEST_TARGET) : $(TEST_OBJ)
	$(LINK.$(FILETYPE)) -o $@ $^ $(LIBS)
	
#$(OBJ): %.o : %.cpp
#	@$(COMPILE.cpp) -o $@ $^
	

clean:
	@rm -f $(RM_FILE)

cleanall:
	@make clean
	
#install lib an head file, may be you need write the file you want to install(eg. ProjectA.h) 
libinstall : libuninstall
	@mkdir -p $(INSTALL_DIR)
	@cp $(LIB_TARGET) inc/common.h $(INSTALL_DIR)
	
libuninstall:	
	@rm -rf $(INSTALL_DIR)$(LIB_TARGET) $(INSTALL_DIR)upgrade.h
	
install:
	@mkdir -p $(INSTALL_DIR)	
	@cp $(TARGET) $(INSTALL_DIR)

uninstall:
	@rm -rf $(INSTALL_DIR)$(LIB_TARGET) $(INSTALL_DIR)$(TARGET) $(INSTALL_DIR)$(TEST_TARGET) 





