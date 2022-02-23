CC=gcc
INCLUDE=include
SRCFOLDER=src
SRC=${SRCFOLDER}/main.c
SRC_TEST:=${wildcard ${SRCFOLDER}/antenna*.c}
TARGET=lcp
TEST_TARGET=antenna_test

all:
	${CC} -o ${TARGET}.bin -I ${INCLUDE} ${SRC} -L. -l correct

test:
	${CC} -o ${TEST_TARGET}.bin -I ${INCLUDE} ${SRC_TEST} -L. -l correct

obc: test
	scp ${TEST_TARGET}.bin root@71.7.234.152:/home/root

obc2: test
	scp -P 2222 ${TEST_TARGET}.bin root@71.7.234.152:/home/root