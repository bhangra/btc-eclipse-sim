#Makefile for sim-eclipse.c
COMPILER 	= gcc#
#COMPILER	= mingw32-gcc.exe
#LINKER		= ld
CFLAGS		= -Wall  -g -O0 
#LDFLAGS	=  --allow-multiple-definition 
LDFLAGS		= -z muldefs #-lmcheck
LIBS		= -lcrypto -lssl -lpthread -lm #/lib64/libpthread.so.0 /lib64/libm.so.6 #/lib/i686/nosegneg/libm-2.14.1.so #-lm
INCLUDE		= -I./
TARGET		= ./$(shell basename `readlink -f .`)
OBJDIR		= ./obj
ifeq "$(strip $(OBJDIR))" ""
	OBJDIR = .
endif
SOURCES		= $(wildcard *.c)
OBJECTS 	= $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
DEPENDS		= $(OBJECTS:.o=.d)

#sim-eclipse: sim-eclipse.c
#	gcc -Wall -O2 -pthread sim-eclipse.c -o simulator

$(TARGET): $(OBJECTS) $(LIBS)
	$(COMPILER) -o $@ $^ $(LDFLAGS)
#$(TARGET): $(OBJECTS) $(LIBS)
#	$(LINKER) -o $@ $^ $(LDFLAGS)


$(OBJDIR)/%.o: %.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(TARGET)

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET)
	@rmdir --ignore-fail-on-non-empty `readlink -f $(OBJDIR)`

-include $(DEPENDS)
