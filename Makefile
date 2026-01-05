# TurkC Compiler - Project 3
# Semantic Analysis + Bytecode Generation + Virtual Machine

CC := gcc
FLEX := flex
BISON := bison

CFLAGS := -Wall -Wextra -std=c11 -g

LEX_SRC := scanner.l
BISON_SRC := parser.y

LEX_OUT := lex.yy.c
BISON_OUT := parser.tab.c
BISON_HDR := parser.tab.h

# All object files including new modules
OBJS := parser.tab.o lex.yy.o ast.o symbol.o semantic.o codegen.o vm.o main.o

TARGET := turkc

.PHONY: all clean test

all: $(TARGET)

# Bison generates parser
$(BISON_OUT) $(BISON_HDR): $(BISON_SRC)
	$(BISON) -d $(BISON_SRC)

# Flex generates scanner
$(LEX_OUT): $(LEX_SRC) $(BISON_HDR)
	$(FLEX) $(LEX_SRC)

# Object file rules
parser.tab.o: parser.tab.c ast.h
	$(CC) $(CFLAGS) -c parser.tab.c

lex.yy.o: lex.yy.c parser.tab.h
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

symbol.o: symbol.c symbol.h
	$(CC) $(CFLAGS) -c symbol.c

semantic.o: semantic.c semantic.h ast.h symbol.h
	$(CC) $(CFLAGS) -c semantic.c

codegen.o: codegen.c codegen.h ast.h symbol.h
	$(CC) $(CFLAGS) -c codegen.c

vm.o: vm.c vm.h codegen.h
	$(CC) $(CFLAGS) -c vm.c

main.o: main.c ast.h symbol.h semantic.h codegen.h vm.h parser.tab.h
	$(CC) $(CFLAGS) -c main.c

# Link final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Run tests
test: $(TARGET)
	@echo "=== Running Tests ==="
	@for f in tests/*.tc; do \
		echo "\n--- Testing $$f ---"; \
		./$(TARGET) -c "$$f" || true; \
	done

# Clean up
clean:
	rm -f $(TARGET) parser.tab.c parser.tab.h lex.yy.c *.o

# Help
help:
	@echo "TurkC Compiler - Project 3"
	@echo ""
	@echo "Build:"
	@echo "  make          - Build the compiler"
	@echo "  make clean    - Remove generated files"
	@echo "  make test     - Run all tests"
	@echo ""
	@echo "Usage:"
	@echo "  ./turkc program.tc      - Compile and run"
	@echo "  ./turkc -c program.tc   - Show bytecode"
	@echo "  ./turkc -a program.tc   - Show AST"
	@echo "  ./turkc -s program.tc   - Show symbol table"
	@echo "  ./turkc -d program.tc   - Debug mode (trace execution)"
