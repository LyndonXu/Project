ifeq ($(PARAM_FILE), ) 
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

ifeq ($(CROSS), )
QMAKE = ../lib_x86/json-c/
else
JSON_DIR = ../lib_target/json-c/
endif

all:
	@cd  ./common; make all

clean:
	@cd  ./common; make clean

install:
	@cd  ./common; make install

uninstall:
	@cd  ./common; make uninstall


