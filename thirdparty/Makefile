#include the global parameter
ifeq ($(PARAM_FILE), ) 
    PARAM_FILE:=./Makefile.param
    include $(PARAM_FILE)
endif

#for open source library, if this file is exist, the compiler will
#not compile the source again
HAS_MAKE = hasmake.param	
CHECK_MAKE = $(wildcard $(HAS_MAKE))

all:
ifeq ($(CROSS), )

ifeq ($(CHECK_MAKE),)
	#compile the json-c-0.11 and install it
	cd json-c-0.11; \
	CFLAGS="-m32" ./configure --prefix=$(LIB_TMP_DIR)json-c


	@echo 1 > $(HAS_MAKE)
	@make install
	
else	
	@echo "I need not to make the open source lib"
endif

else


ifeq ($(CHECK_MAKE),)
	#compile the json-c-0.11 and install it
	cd json-c-0.11; \
	./config_arm_linux.sh; \
	CC=$(CROSS)gcc ./configure --host=arm-linux --prefix=$(LIB_TMP_DIR)json-c --cache-file=arm-linux.cache


	@echo 1 > $(HAS_MAKE)
	@make install
	
else	
	@echo "I need not to make the open source lib"
endif

endif
	
	

clean:
	@cd json-c-0.11;	make clean
	@rm -rf $(HAS_MAKE)

install:
ifeq ($(CHECK_MAKE),)
	make all
	
endif
	#create the intall directory
	mkdir -p $(INSTALL_DIR)	

	#install the json-c-0.11
	@rm -rf $(LIB_TMP_DIR)json-c
	@echo 'rm -rf $(LIB_TMP_DIR)json-c'	
	@cd json-c-0.11;	make install
	@cp -rf $(LIB_TMP_DIR)json-c $(INSTALL_DIR)
	@rm -rf $(LIB_TMP_DIR)json-c	


uninstall:
	@rm -rf $(INSTALL_DIR)json-c


#intall the openssl's header file and library to the cross
installjson-c:
	@rm -rf $(LIB_TMP_DIR)openssl	
	@echo 'rm -rf $(LIB_TMP_DIR)openssl'	
	@cd openssl-0.9.8x;	make install
	@echo -e "\033[47;31mI need administrator privileges!!\033[0m"	
	@sudo cp -rf $(LIB_TMP_DIR)openssl/include $(CROSS_DIR)usr
	@sudo cp -rf $(LIB_TMP_DIR)openssl/lib $(CROSS_DIR)usr


test:
	@echo "$(CROSS)"	
	@echo "$(CROSS_DIR)"	
	
		
