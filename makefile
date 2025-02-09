
# NEWDOOM.EXE and DOOM.EXE makefile

# --------------------------------------------------------------------------
#
#      4r  use 80486 timings and register argument passing
#       c  compile only
#      d1  include line number debugging information
#      d2  include full sybolic debugging information
#      ei  force enums to be of type int
#       j  change char default from unsigned to signed
#      oa  relax aliasing checking
#      od  do not optimize
#  oe[=#]  expand functions inline, # = quads (default 20)
#      oi  use the inline library functions
#      om  generate inline 80x87 code for math functions
#      ot  optimize for time
#      ox  maximum optimization
#       s  remove stack overflow checks
#     zp1  align structures on bytes
#      zq  use quiet mode
#  /i=dir  add include directories
#
# --------------------------------------------------------------------------

#CCOPTS = /d2 /omaxet /ml -0
CCOPTS = -zp1 -oaxet -oi -zq -j -ms -s -zdp -0

#CCOPTS = /d2 /omaxet /zp1 /ei /j /zq /mt /zt100

GLOBOBJS = sqmus.obj &
sqmusopl.obj &
sqmusmpu.obj &
sqmusmid.obj &
sqmussbm.obj &
dmx.obj


#newtest.exe : sqmus.obj
sqmus.exe : $(GLOBOBJS) 
 wlink @sqmus.lnk
 wstrip sqmus.exe

.c.obj :
 wcc $(CCOPTS) $[*


clean : .SYMBOLIC
 del *.obj
 del sqmus.exe
