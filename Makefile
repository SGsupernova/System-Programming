#---------------------------------------------------------------

SOURCES = func_def.c 20141570.c
OBJECTS = $(SOURCES:.c=.o)
MYPROGRAM = 20141570.out

CC = gcc
CFLAG = -Wall
#---------------------------------------------------------------

all: $(SOURCES) $(MYPROGRAM)

$(MYPROGRAM) : $(OBJECTS)
		$(CC) $(CFLAG) -o $@ $(OBJECTS)

clean :
		rm -f *.o $(MYPROGRAM)