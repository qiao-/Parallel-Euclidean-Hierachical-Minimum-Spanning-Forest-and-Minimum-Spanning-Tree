QT       += xml
QT       += gui
CONFIG += console c++11


CONFIG += bit64
CONFIG += topo_hexa
CONFIG += cuda


topo_hexa:DEFINES += TOPOLOGIE_HEXA
cuda:DEFINES += CUDA_CODE
cuda:DEFINES += CUDA_ATOMIC
#CUDA_ATOMIC


unix {
        CONFIG +=
#static
        DEFINES += QT_ARCH_ARMV6
        TARGET = ../application
}
win32 {
        TARGET = ../application
}

SOURCES +=
#../src/main.cpp

#OTHER_FILES += ../src/main.cu #QWB delete this for windows

LIBS += -L$$PWD/../bin/
#-llibOperators

CUDA_SOURCES += ../src/main.cu
#cuda_code.cu

# Here specify your own CUDA architecture
CUDA_FLOAT    = float
#CUDA_ARCH     = -gencode arch=compute_20,code=sm_20
#CUDA_ARCH     = -gencode arch=compute_12,code=sm_12

CUDA_ARCH     = -gencode arch=compute_50,code=sm_50 #


win32:{

  ##############################################################################
  # Here to add the specific QT and BOOST paths according to your Windows system.
  CUDA_DIR      = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v7.0"
  QMAKE_LIBDIR += $$CUDA_DIR/lib/x64 $$LIBS_OPERATORS_DIR $$LIBS_COMPONENTS_DIR
  INCLUDEPATH  += $$CUDA_DIR/include C:\Qt\Qt5.7.0\5.7\msvc2013_64\include C:\Qt\Qt5.7.0\5.7\msvc2013_64\include\QtCore C:\Qt\Qt5.7.0\5.7\msvc2013_64\include\QtXml C:\Qt\Qt5.7.0\5.7\msvc2013_64\include\QtOpenGL  C:/boost_1_59_0
  INCLUDEPATH  += ../include
  INCLUDEPATH  += include ../../../basic_components\components\include
  INCLUDEPATH  += include ../../../optimization_operators\operators\include
  INCLUDEPATH += include C:\ProgramData\NVIDIA Corporation\CUDA Samples\v7.0\common\inc\
################### end config for the author's windows ###################

}


unix:{

  ##############################################################################
  # Here to add the specific QT and BOOST paths according to your Linux system.
  INCLUDEPATH  += ../include /usr/local/Trolltech/Qt-4.8.4/include C:/boost_1_55_0  /home/ben/Downloads/boost_1_57_0

  CUDA_DIR      =
  QMAKE_LIBDIR += $$CUDA_DIR/lib64
  INCLUDEPATH  += $$CUDA_DIR/include /home/userName/Qt/5.4/gcc_64/include
  LIBS += -lcudart -lcuda
  QMAKE_CXXFLAGS += -std=c++0x

  INCLUDEPATH  += ../../../optimization_operators/operators/include
  INCLUDEPATH  += ../../../basic_components/components/include
  INCLUDEPATH  += ../../../adaptive_meshing/meshing/include
  INCLUDEPATH  += $$CUDA_DIR/samples/common/inc
}

LIBS         += -lcuda -lcudart
QMAKE_LFLAGS_DEBUG    = /DEBUG /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib


DEFINES += "CUDA_FLOAT=$${CUDA_FLOAT}"

NVCC_OPTIONS = --use_fast_math -DCUDA_FLOAT=$${CUDA_FLOAT}
cuda:NVCC_OPTIONS += -DCUDA_CODE
#NVCCFLAGS     = --compiler-options -fno-strict-aliasing -use_fast_math --ptxas-options=-v

QMAKE_EXTRA_COMPILERS += cuda

CUDA_INC = $$join(INCLUDEPATH,'" -I"','-I"','"')

CONFIG(release, debug|release) {
  OBJECTS_DIR = ./release
bit64:cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine 64 $$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
else:cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine 32 $$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
}
CONFIG(debug, debug|release) {
  OBJECTS_DIR = ./debug
bit64:cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine 64 $$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
else:cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$LIBS --machine 32 $$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
}

#cuda.dependency_type = TYPE_C
#cuda.depend_command = $$CUDA_DIR/bin/nvcc -O3 -M $$CUDA_INC $$NVCCFLAGS   ${QMAKE_FILE_NAME}
cuda.input = CUDA_SOURCES
cuda.output = $${OBJECTS_DIR}/${QMAKE_FILE_BASE}_cuda.o


HEADERS += \
    ../include/TestMstBoruvka.h
