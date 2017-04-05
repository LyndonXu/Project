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
JSON_DIR = ../../lib_x86/json-c/
else
JSON_DIR = ../../lib_target/json-c/
endif


#include directory
INC_FLAGS = -I ./inc
INC_FLAGS += -I ../mpp/include
INC_FLAGS += -I ../mpp/extdrv/tlv320aic31
INC_FLAGS += -I ../mpp/extdrv/nvp6124
INC_FLAGS += -I ./common
INC_FLAGS += -I $(JSON_DIR)include

PROJECT = FB

INSTALL_DIR = ../../lib_x86/

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
CFLAGS += -O2 -fpermissive
endif



CFLAGS += -fPIC -Wall $(INC_FLAGS) $(CDEF)
ifeq ($(CROSS), )
ifeq ($(SYS_BIT), 64)
CFLAGS += -m32 
endif
endif


LIBDIR = -L ./
LIBDIR += -L $(JSON_DIR)lib
LIBDIR += -L ../mpp/lib


LIBS = $(LIBDIR) 
#-l$(PROJECT)
LIBS += -lpthread
LIBS += -lmpi -lhdmi -ltde -ljpeg -lupvqe -ldnvqe -lVoiceEngine


LIBS_NEW = $(LIBDIR)
LIBS_NEW += -l$(PROJECT)
LIBS_NEW += -lpthread
LIBS_NEW += -ljson-c


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
SRC += $(wildcard common/*.$(FILETYPE))
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

all : test

test : $(TARGET)
$(TARGET) : $(OBJ)
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
	@cp $(LIB_TARGET) inc/common.h $(INSTALL_DIR)

uninstall:
	@rm -rf $(INSTALL_DIR)$(LIB_TARGET) $(INSTALL_DIR)$(TARGET) $(INSTALL_DIR)$(TEST_TARGET) $(INSTALL_DIR)upgrade.h 

DATE=`date +%Y%m%d%H%M%S`
NAME=backup_$(DATE).tar.bz2
FILE=`ls | grep -v backup_`
backup:clean
	@echo $(DATE)
	@echo $(NAME)
#	tar cfvj $(NAME) inc lib src Makefile rmoldbackup.sh
	tar cfvj $(NAME) $(FILE)
	./rmoldbackup.sh backup
	



