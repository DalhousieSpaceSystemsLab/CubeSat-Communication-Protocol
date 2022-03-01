CC=arm-none-linux-gnueabihf-gcc
INCLUDE=include
SRCFOLDER=src
SRC=${SRCFOLDER}/main.c
SRC_TEST:=${wildcard ${SRCFOLDER}/antenna*.c}
TARGET=lcp
TEST_TARGET=antenna_test
LIBCORRECT_BUILD_PATH=libcorrect/build-arm32

all:
	${CC} -o ${TARGET}.bin -I ${INCLUDE} ${SRC} -L. -l correct

correct:
	cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -B ${LIBCORRECT_BUILD_PATH} -S libcorrect
	cmake --build ${LIBCORRECT_BUILD_PATH}
	cp ${LIBCORRECT_BUILD_PATH}/lib/libcorrect.a .

test:
	${CC} -o ${TEST_TARGET}.bin -I ${INCLUDE} ${SRC_TEST} -L. -l correct

obc: correct test
	CC=arm-none-linux-gnueabihf-gcc
	scp ${TEST_TARGET}.bin root@71.7.234.152:/home/root

obc2: correct test
	CC=arm-none-linux-gnueabihf-gcc
	scp -P 2222 ${TEST_TARGET}.bin root@71.7.234.152:/home/root