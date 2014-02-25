# Makefile for converting filter *.flt files to *.hf
.SWAP
.AUTODEPEND

# Version Generation
#generate a new version of this file every time make is activated
# help
#ver_ctl

# Explicit rule to build the file
filter.h:  340_00T1.hf\
        270_70B1.hf\
        215_55B1.hf\
        170_45B1.hf\
        135_35B1.hf\
        115_20B1.hf\
        095_20B1.hf\
        079_16B1.hf\
        068_11S.hf\
        053_16B.hf\
        037_16B.hf\
        023_14B.hf\
        011_12B.hf\
        005__5S1.hf\
        640_LPI.hf\
        320_LPI.hf\
        160_LPI.hf


# Implicit rules

#cd filters
.flt.hf:
        prc_fdas $*.flt


#cd ..
#.c.o96:
#        -1 c:\bin\ic96\bin\ic96.exe $*.c oj($*.o96) df(IC96) df(MMIR_UNIT)

#.a96.o96:
#        a96 $*


