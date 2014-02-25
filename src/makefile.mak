# Makefile for MMIR programs
.SWAP
.AUTODEPEND

# Version Generation
#generate a new version of this file every time make is activated
# help
#version.c : ver_ctl.exe
#           ver_ctl

# Explicit rule to build the file
eprom.omf: rel_tst.o96 res_rism.o96 ccb.o96 build.ltx makefile.mak\
           version.o96 \
           sch.o96 util.o96 \
           init_int.o96 init_itr.o96 init_mem.o96 dummy.o96\
           drm.o96 drm_ram.o96\
           ldd.o96 ldd_tlx.o96\
           hw.o96 hw_fec.o96 hw_key.o96\
           dpl.o96 dpl_stat.o96\
           mfd.o96\
           dim.o96\
           uif.o96  uif_demo.o96 uif_opts.o96 uif_uplds.o96 uif_revw.o96\
           ssp.o96 ssp_uart.o96\
           pts_uart.o96 \
           dfm_mng.o96 dfm_i.o96 dfm_i196.o96 dfm_asm.o96 dfm_asm1.o96\
           in_ram.o96\
           version.o96\
           dbg_if.o96 dbg_pstr.o96 dbg_tim.o96
           -1 c:\n\bin\ic96\bin\rl96 rel_tst.o96, & <build.ltx
           C:\n\bin\ic96\bin\oh eprom.omf



# Implicit rules

#cd filters
#.flt.hf:
#        prc_fdas $*.flt


#cd ..
.c.o96:
        -1 c:\n\bin\ic96\bin\ic96.exe $*.c oj($*.o96) co ot(1) df(IC96) df(MMIR_UNIT)

.a96.o96:
        a96 $*

