CC := gcc
FLEX := flex
BISON := bison

CFLAGS := -Wall -Wextra -std=c11

LEX_SRC := scanner.l
BISON_SRC := parser.y

LEX_OUT := lex.yy.c
BISON_OUT := parser.tab.c
BISON_HDR := parser.tab.h

OBJS := parser.tab.o lex.yy.o ast.o main.o

.PHONY: all clean

all: turkc

$(BISON_OUT) $(BISON_HDR): $(BISON_SRC)
	$(BISON) -d $(BISON_SRC)

$(LEX_OUT): $(LEX_SRC) $(BISON_HDR)
	$(FLEX) $(LEX_SRC)

parser.tab.o: parser.tab.c ast.h
	$(CC) $(CFLAGS) -c parser.tab.c

lex.yy.o: lex.yy.c parser.tab.h
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

main.o: main.c ast.h parser.tab.h
	$(CC) $(CFLAGS) -c main.c

turkc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f turkc parser.tab.c parser.tab.h lex.yy.c *.o

