DEBUG ?=
DEFINE ?=
CROSS_COMPILE ?= 
ARCH ?= 
LIB ?= 
ifeq ($(ARCH),arm)   # 版本: gcc version 7.5.0 														
CROSS_COMPILE = arm-linux-gnueabihf-
INC_DIR :=  
LIB += -L ./lib/$(ARCH) -lexpat -lpython3.5m 
else	
# 仿真 x86_64-linux-gun-gcc, 版本: gcc version 9.3.0   																
ARCH := amd64
INC_DIR := 
LIB += -L ./lib/$(ARCH) -lpython3.8
endif

LIB += -lz -lpthread -lbase64 -lhiredis -lCJsonObject -Wl,-rpath,'$$ORIGIN/lib'

INC_DIR := com_inc \
					 com_inc/lib \
					 com_inc/lib/python3.8 \
						./

SRC_DIR := com_src \
					./ 
						
VPATH := $(SRC_DIR) #set MakeFile's file searching dir

C_FLAGS += -Wall -std=c11 $(DEBUG) $(DEFINE)
CPP_FLAGS += -Wall -std=c++17 $(DEBUG)  $(DEFINE)
LD_FLAGS += 
OBJ_DIR := obj
OUTPUT_DIR := com_output

################# format handler #######################
CC := $(CROSS_COMPILE)gcc
PP := $(CROSS_COMPILE)g++
LD := $(CROSS_COMPILE)ld
OBJCOPY:=$(CROSS_COMPILE)objcopy 

INC_PATH := $(patsubst %, -I %, $(INC_DIR))

SRC_PATH := $(SRC_DIR)

OBJ_PATH = $(OBJ_DIR)

C_FILES := $(notdir $(foreach dir, $(SRC_PATH),$(wildcard $(dir)/*.c)))
CPP_FILES := $(notdir $(foreach dir, $(SRC_PATH),$(wildcard $(dir)/*.cpp)))

C_OBJS := $(patsubst %.c, $(OBJ_PATH)/%.o, $(C_FILES))
CPP_OBJS := $(patsubst %.cpp, $(OBJ_PATH)/%.o, $(CPP_FILES))
OBJS := $(C_OBJS) $(CPP_OBJS)



server : $(OBJS) pythonRunner
	cd ./server && $(MAKE) $(MAKEFLAGS)
	cp $(OUTPUT_DIR)/*.elf ./server/output

client : $(OBJS) pythonRunner
	cd ./client && $(MAKE) $(MAKEFLAGS)
	cp $(OUTPUT_DIR)/*.elf ./client/output

pythonRunner:
	$(PP) -o $(OUTPUT_DIR)/pythonRunner.elf ./scriptsRunner/pythonRunner.cpp $(INC_PATH) $(LIB)


$(C_OBJS) : $(OBJ_PATH)/%.o : %.c
	$(CC) $(C_FLAGS)  $(INC_PATH) -c -o $@ $<


$(CPP_OBJS) : $(OBJ_PATH)/%.o : %.cpp
	$(PP) $(CPP_FLAGS) $(INC_PATH) -c -o  $@ $< 


.PHNOY:
clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.bin $(OBJ_DIR)/*.elf
	rm -f ./*.elf  $(OUTPUT_DIR)/*.elf 
	cd ./server && $(MAKE) clean
	cd ./client && $(MAKE) clean