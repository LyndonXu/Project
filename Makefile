ifeq ($(PARAM_FILE), ) 
    PARAM_FILE:=./Makefile.param
    include $(PARAM_FILE)
endif

DIRS = user_sdk \
		UartDaemon
#		user_sdk \
#		main_process \
#		bp_process \
#		bg_process \
#		am_process \
#		communicate_process
		
all : 
	@ for dir in ${DIRS}; do (cd $${dir}; ${MAKE}) ; done

clean: 
	@ for dir in ${DIRS}; do (cd $${dir}; ${MAKE} clean) ; done

lib:
	@ for dir in ${DIRS}; do (cd $${dir}; ${MAKE} lib) ; done
	
install:
	@ for dir in ${DIRS}; do (cd $${dir}; ${MAKE} install) ; done

uninstall:
	@ for dir in ${DIRS}; do (cd $${dir}; ${MAKE} uninstall) ; done
	
