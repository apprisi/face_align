OBJ=object_align.o object_detect.o 

DSTDIR=bin
FLAGS= -O3
DEPS = ${FLAGS} -I/usr/local/include $(shell pkg-config --cflags --libs opencv)

ALIGN=$(DSTDIR)/align

DST=$(ALIGN)

all: outdir ${DST}

outdir:
	mkdir -p ${DSTDIR}


${ALIGN}: main.cpp ${OBJ} ${LIBLINEAR}
	g++ -DMAIN_ALIGN -o $@ $^ ${DEPS}

${OBJ}: %.o : %.cpp
	g++ -c ${FLAGS} $^

${LIBLINEAR}:
	make -C linear

clean: 
	@rm -f ${OBJ} ${DST} 
	make -C linear clean
