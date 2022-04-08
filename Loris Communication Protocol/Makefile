CC=arm-none-linux-gnueabihf-gcc
INCLUDE=include
SRCFOLDER=src
SRC=${SRCFOLDER}/main.c
SRC_TEST:=${wildcard ${SRCFOLDER}/antenna*.c}
# SRC_FIFO:=${wildcard ${SRCFOLDER}/fifo*.c} 
SRC_FIFO=${SRCFOLDER}/fifo.c ${SRCFOLDER}/fifo-emulation.c ${SRCFOLDER}/antenna.c ${SRCFOLDER}/antenna_packet.c
TARGET=lcp
TEST_TARGET=antenna_test
FIFO_TARGET=fifo
LIBCORRECT_BUILD_PATH=libcorrect/build-arm32
LIBCORRECT_BUILD_PATH_VANILLA=libcorrect/build-x86

all:
	${CC} -o ${TARGET}.bin -I ${INCLUDE} ${SRC} -L. -l correct

correct:
	cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -B ${LIBCORRECT_BUILD_PATH} -S libcorrect
	cmake --build ${LIBCORRECT_BUILD_PATH}
	cp ${LIBCORRECT_BUILD_PATH}/lib/libcorrect.a .

correct-vanilla:
	cmake -B ${LIBCORRECT_BUILD_PATH_VANILLA} -S libcorrect
	cmake --build ${LIBCORRECT_BUILD_PATH_VANILLA}
	cp ${LIBCORRECT_BUILD_PATH_VANILLA}/lib/libcorrect.a .

test:
	${CC} -o ${TEST_TARGET}.bin -I ${INCLUDE} ${SRC_TEST} -L. -l correct

fifo: correct-vanilla
	gcc -g -o ${FIFO_TARGET}.bin -I ${INCLUDE} ${SRC_FIFO} -L. -l correct -l pthread

obc: correct test
	CC=arm-none-linux-gnueabihf-gcc
	scp ${TEST_TARGET}.bin root@173.212.68.129:/home/root

obc2: correct test
	CC=arm-none-linux-gnueabihf-gcc
	scp -P 2222 ${TEST_TARGET}.bin root@173.212.68.129:/home/root