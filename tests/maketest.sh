
# show size of test case sw
gcc -c jumpbug_unit_test_fw.c -Os -m32 -Wall
size jumpbug_unit_test_fw.o | awk 'FNR==2{print "exec code:     " $1 " bytes"}'