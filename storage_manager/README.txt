##How to execute the Script:

1. Open terminal 
2. Go to the project directory
3.  To remove any existing object files run "make clean"
4. Run the command "make"
5. To remove object files run the command "make run"
6. To remove object files run "make clean"

##Summary:

The objective of this task is to develop a basic storage manager module. This module should be able to read and write blocks of data to and from a file on disk. The storage manager will handle pages (blocks) of a fixed size, known as PAGE SIZE. It should include functions for creating, opening, and closing files.

##Storage Manager Functions:
1. initStorageManager(void)
 - Purpose: Initializes the storage manager by checking write permissions in the current directory.
 - Functionality: Checks if write access is available in the current directory. If not, it prints an error message and exits. If write access is available, it prints a success message.
2. cleanupAndFail(FILE *filePntr, char *buffer, RC errorCode)
 - Purpose: Cleans up resources and exits with an error code.
 - Functionality: Closes the file pointer if it's open, frees the allocated buffer, and then exits the program with the specified error code.
3. createPageFile(char *fileName)
 - Purpose: Creates a new page file and initializes it.
 - Functionality: Creates a new file, writes an initial buffer to it, and writes metadata (number 1) at the beginning. Cleans up resources if any operation fails and returns appropriate error codes.
4. openFileStream(char *fileName, FILE **filePntr)
 - Purpose: Opens a file stream for reading and writing.
 - Functionality: Attempts to open the file in read/write mode. Returns an error code if the file cannot be opened.
5. initializeFileHandle(SM_FileHandle *fHandle, char *fileName, FILE *filePntr)
 - Purpose: Initializes the file handle.
 - Functionality: Sets the file name, current page position to 0, and the management information (file pointer) in the file handle.
6. readFileMetadata(FILE *filePntr, SM_FileHandle *fHandle)
 - Purpose: Reads metadata (total number of pages) from the file.
 - Functionality: Reads the total number of pages from the file and updates the file handle. Returns an error code if reading fails.
7. openPageFile(char *fileName, SM_FileHandle *fHandle)
 - Purpose: Opens a page file and initializes the file handle.
 - Functionality: Opens the file, initializes the file handle, reads file metadata, and returns appropriate success or error codes.
8. closePageFile(SM_FileHandle *fHandle)
 - Purpose: Closes the page file and releases resources.
 - Functionality: Closes the file pointer and returns success or failure based on whether the file was successfully closed.
9. destroyPageFile(char *fileName)
 - Purpose: Deletes the specified page file.
 - Functionality: Removes the file from the filesystem and returns success or failure based on whether the file was successfully removed.
10. validatePageNumber(int pageNum, SM_FileHandle *fHandle)
 - Purpose: Checks if the page number is valid.
 - Functionality: Compares the page number with the total number of pages and returns an error code if the page number is invalid.
11. getPageOffset(int pageNum)
 - Purpose: Calculates the file offset for a given page number.
 - Functionality: Returns the byte offset for the specified page number based on the page size.
12. getFileSize(FILE *filePtr)
 - Purpose: Retrieves the size of the file.
 - Functionality: Moves the file pointer to the end of the file, gets the file size, and then resets the file pointer to the beginning.
13. seekToOffset(FILE *filePtr, long offset)
 - Purpose: Moves the file pointer to a specific offset.
 - Functionality: Sets the file pointer to the specified offset and returns success or failure based on the result.
14. calculateBytesToRead(long fileSize, long offset)
 - Purpose: Determines the number of bytes to read based on the file size and offset.
 - Functionality: Calculates the number of bytes to read, ensuring it doesn't exceed the file size.
15. readDataFromFile(FILE *filePtr, SM_PageHandle memPage, int bytesToRead)
 - Purpose: Reads data from the file into memory.
 - Functionality: Reads the specified number of bytes from the file into the provided memory page and returns success or failure.
16. readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Reads a block of data from the file.
 - Functionality: Validates the page number, calculates the offset, seeks to the offset, reads data into memory, and updates the current page position in the file handle.
17. getBlockPos(SM_FileHandle *fHandle)
 - Purpose: Retrieves the current block position in the file.
 - Functionality: Returns the current page position stored in the file handle.
18. readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Reads the first block of the file.
 - Functionality: Calls readBlock with page number 0 to read the first block.
19. readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Reads the previous block of the file.
 - Functionality: Gets the current block position, calculates the previous block position, and reads the previous block.
20. readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Reads the current block of the file.
 - Functionality: Calls readBlock for the current block position.
21. readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Reads the next block of the file.
 - Functionality: Gets the current block position, calculates the next block position, and reads the next block.
22. readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Reads the last block of the file.
 - Functionality: Calculates the position of the last block and reads it.
23. checkPageValidity(int pageNum, SM_FileHandle *fHandle)
 - Purpose: Checks if the page number is valid for writing.
 - Functionality: Validates the page number and returns an error code if it is invalid.
24. calculatePageOffset(int pageNum)
 - Purpose: Calculates the file offset for a given page number for writing.
 - Functionality: Returns the byte offset for the specified page number based on the page size.
25. seekToPage(FILE *filePtr, long offset)
 - Purpose: Moves the file pointer to the correct offset for writing.
 - Functionality: Sets the file pointer to the specified offset and returns success or failure.
26. writeData(FILE *filePtr, SM_PageHandle memPage)
 - Purpose: Writes data from memory to the file.
 - Functionality: Writes the data from the memory page to the file and returns success or failure.
27. writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Writes a block of data to the file.
 - Functionality: Validates the page number, calculates the offset, seeks to the offset, writes data from memory to the file, and updates the current page position in the file handle.
28. writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
 - Purpose: Writes to the current block in the file.
 - Functionality: Retrieves the current block position and writes data to it.
29. appendEmptyBlock(SM_FileHandle *fHandle)
 - Purpose: Appends an empty block to the file.
 - Functionality: Moves to the end of the file, writes an empty block, updates the total number of pages in the file handle, and updates the page metadata.
30. ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
 - Purpose: Ensures that the file has at least the specified number of pages.
 - Functionality: Appends empty blocks to the file until the total number of pages meets or exceeds the required number. Returns success or failure based on the result of appending blocks.