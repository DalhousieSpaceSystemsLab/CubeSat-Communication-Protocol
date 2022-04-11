CC=arm-none-linux-gnueabihf-gcc
INCLUDE=include
SRCFOLDER=src
SRC=$(SRCFOLDER)/main.c
SRC_TEST:=$(wildcard $(SRCFOLDER)/antenna*.c)
# SRC_FIFO:=$(wildcard $(SRCFOLDER)/fifo*.c) 
# SRC_FIFO=$(SRCFOLDER)/fifo.c $(SRCFOLDER)/fifo-emulation.c $(SRCFOLDER)/antenna.c $(SRCFOLDER)/antenna_packet.c
SRC_FIFO=$(wildcard $(SRCFOLDER)/fifo*.c) $(filter-out $(SRCFOLDER)/antenna_test.c, $(wildcard $(SRCFOLDER)/antenna*.c))
TARGET=lcp
TEST_TARGET=antenna_test
TEST_TARGET_PC=antenna_test_pc
FIFO_TARGET=fifo
LIBCORRECT_BUILD_PATH=libcorrect/build-arm32
LIBCORRECT_BUILD_PATH_VANILLA=libcorrect/build-x86

.PHONY: all correct correct-vanilla obc obc2 test

all:
	$(CC) -o $(TARGET).bin -I $(INCLUDE) $(SRC) -L. -l correct

correct:
	cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -B $(LIBCORRECT_BUILD_PATH) -S libcorrect
	cmake --build $(LIBCORRECT_BUILD_PATH)
	cp $(LIBCORRECT_BUILD_PATH)/lib/libcorrect.a .

correct-vanilla:
	cmake -B $(LIBCORRECT_BUILD_PATH_VANILLA) -S libcorrect
	cmake --build $(LIBCORRECT_BUILD_PATH_VANILLA)
	cp $(LIBCORRECT_BUILD_PATH_VANILLA)/lib/libcorrect.a .

test: test-pc test-arm

test-arm: correct
	$(CC) -static -o $(TEST_TARGET).bin -I $(INCLUDE) $(SRC_TEST) -L. -lcorrect -lpthread

test-pc: correct-vanilla
	gcc -o $(TEST_TARGET_PC).bin -I$(INCLUDE) $(SRC_TEST) -L. -lcorrect -lpthread

fifo: correct-vanilla
	gcc -g -o $(FIFO_TARGET).bin -I $(INCLUDE) $(SRC_FIFO) -L. -l correct -l pthread

obc: correct test
	scp $(TEST_TARGET).bin root@173.212.68.129:/home/root

obc2: correct test
	scp -P 2222 $(TEST_TARGET).bin root@173.212.68.129:/home/root		