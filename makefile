gcc_options = -O -std=c++20 -Wall --pedantic-errors
g++ = g++-11

debug_options = -g -DDEBUG
train_options = -DTRAIN
asm_options = -S -O0 -fno-asynchronous-unwind-tables

hedder = push4_headder.h
heddergch = push4_headder.h.gch
mainfile = push4.cpp

push4 : $(mainfile) $(hedder) $(heddergch)
	$(g++) $(gcc_options) -include $(hedder) $< -o $@

$(heddergch) : $(hedder)
	$(g++) $(gcc_options) -x c++-header -o $@ $<

push4_train : $(mainfile) $(hedder) $(heddergch)
	$(g++) $(gcc_options) $(train_options) -include $(hedder) $< -o $@

push4_debug : $(mainfile) $(hedder) $(heddergch)
	$(g++) $(gcc_options) $(debug_options) -include $(hedder) $< -o $@

asm :  $(mainfile) $(hedder) $(heddergch)
	$(g++) $(gcc_options) $(asm_options) -include $(hedder) $< -o push4.s

train : push4_train
	./push4_train

debug : push4_debug
	lldb -f push4_debug

run : push4
	./push4

clean : 
	rm -f ./push4
	rm -f ./push4_debug
	rm -f ./push4_headder.h.gch
	rm -rf *.dSYM 
	rm -f ./hello.push4
	rm -f ./push4_train

.PHONY : run clean