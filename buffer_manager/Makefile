# Define the compiler
CC = gcc

# Define the output executables
EXE1 = run_test_assign2_1.exe
EXE2 = run_test_assign2_2.exe

# List of object files for test_assign2_1
OBJECTS1 = storage_mgr.o dberror.o test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o

# List of object files for test_assign2_2
OBJECTS2 = storage_mgr.o dberror.o test_assign2_2.o buffer_mgr.o buffer_mgr_stat.o

# Rule to link object files into the first executable
$(EXE1): $(OBJECTS1)
	$(CC) -o $@ $(OBJECTS1)

# Rule to link object files into the second executable
$(EXE2): $(OBJECTS2)
	$(CC) -o $@ $(OBJECTS2)

# Rule for compiling storage_mgr.o
storage_mgr.o: storage_mgr.c storage_mgr.h
	$(CC) -c $< -o $@

# Rule for compiling dberror.o
dberror.o: dberror.c dberror.h
	$(CC) -c $< -o $@

# Rule for compiling test_assign2_1.o
test_assign2_1.o: test_assign2_1.c
	$(CC) -c $< -o $@

# Rule for compiling test_assign2_2.o
test_assign2_2.o: test_assign2_2.c
	$(CC) -c $< -o $@

# Rule for compiling buffer_mgr.o
buffer_mgr.o: buffer_mgr.c buffer_mgr.h
	$(CC) -c $< -o $@

# Rule for compiling buffer_mgr_stat.o
buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h
	$(CC) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJECTS1) $(OBJECTS2) $(EXE1) $(EXE2)

# Rule to run both executables
run: $(EXE1) $(EXE2)
	./$(EXE1)
	./$(EXE2)

# Rule to run the first executable
run1: $(EXE1)
	./$(EXE1)

# Rule to run the second executable
run2: $(EXE2)
	./$(EXE2)

# Phony targets
.PHONY: clean run run1 run2
