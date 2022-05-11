CC=arm-none-linux-gnueabihf-gcc
INCLUDE=-Iinclude -Irscode-master
SRCFOLDER=src
SRC=$(SRCFOLDER)/main.c
SRC_TEST:=$(wildcard $(SRCFOLDER)/antenna*.c)
# SRC_FIFO:=$(wildcard $(SRCFOLDER)/fifo*.c) 
# SRC_FIFO=$(SRCFOLDER)/fifo.c $(SRCFOLDER)/fifo-emulation.c $(SRCFOLDER)/antenna.c $(SRCFOLDER)/antenna_packet.c
SRC_FIFO=$(wildcard $(SRCFOLDER)/fifo*.c) $(filter-out $(SRCFOLDER)/antenna_test.c, $(wildcard $(SRCFOLDER)/antenna*.c))
LDFLAGS=-L. -lcorrect -lpthread -lecc
TARGET=lcp
TEST_TARGET=antenna_test
TEST_TARGET_PC=antenna_test_pc
FIFO_TARGET=fifo
LIBCORRECT_BUILD_PATH=libcorrect/build-arm32
LIBCORRECT_BUILD_PATH_VANILLA=libcorrect/build-x86

.PHONY: all correct correct-vanilla obc obc2 test clean

all:
	$(CC) -o $(TARGET).bin $(INCLUDE) $(SRC) -L. -l correct

correct:
	cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -B $(LIBCORRECT_BUILD_PATH) -S libcorrect
	cmake --build $(LIBCORRECT_BUILD_PATH)
	cp $(LIBCORRECT_BUILD_PATH)/lib/libcorrect.a .

correct-vanilla:
	cmake -B $(LIBCORRECT_BUILD_PATH_VANILLA) -S libcorrect
	cmake --build $(LIBCORRECT_BUILD_PATH_VANILLA)
	cp $(LIBCORRECT_BUILD_PATH_VANILLA)/lib/libcorrect.a .

test: test-pc test-arm

test-arm: correct rscode-arm
	$(CC) -o $(TEST_TARGET).bin $(INCLUDE) $(SRC_TEST) $(LDFLAGS)

test-pc: correct-vanilla rscode-pc
	gcc -static -g -o $(TEST_TARGET_PC).bin $(INCLUDE) $(SRC_TEST) $(LDFLAGS)

fifo: correct-vanilla
	gcc -g -o $(FIFO_TARGET).bin $(INCLUDE) $(SRC_FIFO) -L. -l correct -l pthread

rscode-arm: clean-rscode
	make -C rscode-master CC=arm-none-linux-gnueabihf-gcc
	cp rscode-master/libecc.a .

rscode-pc: clean-rscode
	make -C rscode-master
	cp rscode-master/libecc.a .

obc: correct test
	scp $(TEST_TARGET).bin root@173.212.68.129:/home/root

obc2: correct test
	scp -P 2222 $(TEST_TARGET).bin root@173.212.68.129:/home/root

clean: clean-rscode
	rm -rf $(LIBCORRECT_BUILD_PATH) $(LIBCORRECT_BUILD_PATH_VANILLA) libcorrect.a $(FIFO_TARGET) $(TEST_TARGET) $(TEST_TARGET_PC)

clean-rscode:
	make -C rscode-master clean
