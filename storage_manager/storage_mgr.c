#include <stdio.h>
#include <stdlib.h>
#include "storage_mgr.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Function to initialize the storage manager by checking write permissions in the current directory
extern void initStorageManager(void) {
    // Check if the current directory has write permission
    if (access(".", W_OK) == -1) {
        // Print error message and exit the program with an error code for write failure
        //exit(printf("Write access denied. Aborting program execution...\n") && RC_WRITE_FAILED);
        printf("Write access denied. Aborting program execution...\n");
        exit(RC_WRITE_FAILED);

    }
    // Display a message indicating successful initialization of the storage manager
    printf("Storage system successfully initialized.\n");
}

// Helper function to clean up resources
void cleanupAndFail(FILE *filePntr, char *buffer, RC errorCode) {
    if (filePntr) fclose(filePntr);  // Close file if it was opened
    if (buffer) free(buffer);        // Free memory if allocated
    exit(errorCode);                 // Exit with the provided error code
}

// Function to create a new page file with initialization
extern RC createPageFile(char *fileName) {
    // Open the file in read/write mode, create it if it doesn't exist
    FILE *filePntr = fopen(fileName, "w+");

    // Check if the file was successfully opened
    if (filePntr != NULL) {
        // Use variables for character size and page factor
        int charSize = sizeof(char);
        int pageFactor = 2;
        // Calculate the number of characters based on the page size, character size, and the page factor
        int charCount = (PAGE_SIZE * pageFactor) / charSize;
        // Allocate memory using malloc and initialize it to zero using memset
        char *buffer = (char *)malloc(charCount * charSize);
        memset(buffer, 0, charCount);
        // Attempt to write the buffer to the file
        if (fwrite(buffer, sizeof(char), charCount, filePntr) < charCount) {
            // If writing fails, clean up resources and return an error code
            cleanupAndFail(filePntr, buffer, RC_WRITE_FAILED);
        }
        // Reset the file pointer to the start of the file
        fseek(filePntr, 0L, SEEK_SET);
        // Write a metadata header to the file (in this case, the number 1)
        if (fprintf(filePntr, "%d", 1) < 0) {
            // Use the helper function for cleanup and returning failure
            cleanupAndFail(filePntr, buffer, RC_WRITE_FAILED);
        }
        // Clean up resources by freeing the buffer and closing the file, then return success
        return (free(buffer), fclose(filePntr), RC_OK);
    }

     // Log an error message, close the file, and return the file-not-found error code using the comma operator
    return (printf("Error: File not found. Closing the file and returning error code.\n"), fclose(filePntr), RC_FILE_NOT_FOUND);
}

// Function to open a file stream for read/write operations
RC openFileStream(char *fileName, FILE **filePntr) {
    // Attempt to open the file in read/write mode ("r+")
    *filePntr = fopen(fileName, "r+");  
    // If the file could not be opened, return a file-not-found error
    if (*filePntr == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    // If the file was successfully opened, return success
    return RC_OK;
}

// Function to initialize the file handle with the file name, file pointer, and starting page position
void initializeFileHandle(SM_FileHandle *fHandle, char *fileName, FILE *filePntr) {
    fHandle->fileName = fileName;       // Set the file name in the file handle
    fHandle->curPagePos = 0;            // Set the current page position to the beginning (page 0)
    fHandle->mgmtInfo = filePntr;       // Store the file pointer in the file handle's management info
}

// Function to read metadata (total number of pages) from the file
RC readFileMetadata(FILE *filePntr, SM_FileHandle *fHandle) {
    // Attempt to read the total number of pages from the file (stored at the beginning)
    if (fscanf(filePntr, "%d", &(fHandle->totalNumPages)) != 1) {
        // If reading the number of pages fails, close the file and return a read-failed error
        fclose(filePntr);
        return RC_FILE_NOT_FOUND;
    }
    // If the metadata was successfully read, return success
    return RC_OK;
}

// Main function to open a page file and initialize the file handle with relevant metadata
extern RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    FILE *filePntr;
    RC status;
    // Step 1: Open the file stream
    // This function attempts to open the file and returns an error if the file is not found
    status = openFileStream(fileName, &filePntr);
    if (status != RC_OK) {
        return status;  // Return the error code if the file could not be opened
    }
    // Step 2: Initialize the file handle
    // This function sets up the file handle with the file name, file pointer, and current page position
    initializeFileHandle(fHandle, fileName, filePntr);
    // Step 3: Read file metadata (total number of pages)
    // This function reads the total number of pages from the file and updates the file handle
    status = readFileMetadata(filePntr, fHandle);
    if (status != RC_OK) {
        return status;  // Return the error code if reading the metadata failed
    }
    // Step 4: Return success message
    // Print a success message and return RC_OK to indicate the file was opened successfully
    return (printf("File opened and metadata read successfully.\n"), RC_OK);
}

// Function to close the page file and release resources
extern RC closePageFile(SM_FileHandle *fHandle) {
    // Retrieve the file pointer from the file handle
    FILE *filePntr = (FILE *)fHandle->mgmtInfo;
    // Attempt to close the file and return success if successful
    if (fclose(filePntr) == 0) {
        return RC_OK;  // Successfully closed the file
    }
    // If closing the file failed, return a file-not-found error
    return RC_FILE_NOT_FOUND;
}

// Function to delete the specified page file from the system
extern RC destroyPageFile(char *fileName) {
    // Attempt to remove the file and check the result
    int result = remove(fileName);
    // If the file was successfully removed, return RC_OK
    if (result == 0) {
        return RC_OK;
    }  
    // If the file could not be removed, return RC_FILE_NOT_FOUND
    return RC_FILE_NOT_FOUND;
}

// Helper function to check if the page number is valid
RC validatePageNumber(int pageNum, SM_FileHandle *fHandle) {
    return (pageNum >= fHandle->totalNumPages) ? RC_READ_NON_EXISTING_PAGE : RC_OK;
}

// Helper function to calculate the offset for the page
long getPageOffset(int pageNum) {
    return (pageNum + 1) * PAGE_SIZE;
}

// Helper function to get the total file size
long getFileSize(FILE *filePtr) {
    fseek(filePtr, 0L, SEEK_END);  // Move to the end
    long size = ftell(filePtr);    // Get the size
    rewind(filePtr);               // Reset the file pointer to the beginning
    return size;
}

// Helper function to seek the file pointer to a specific offset
RC seekToOffset(FILE *filePtr, long offset) {
    return (fseek(filePtr, offset, SEEK_SET) == 0) ? RC_OK : RC_READ_NON_EXISTING_PAGE;
}

// Helper function to determine how many bytes to read
int calculateBytesToRead(long fileSize, long offset) {
    long readEndOffset = (fileSize < (offset + PAGE_SIZE)) ? fileSize : (offset + PAGE_SIZE);
    return (readEndOffset - offset) / sizeof(char);
}

// Helper function to read data from the file into memory
RC readDataFromFile(FILE *filePtr, SM_PageHandle memPage, int bytesToRead) {
    return (fread(memPage, sizeof(char), bytesToRead, filePtr) == bytesToRead) ? RC_OK : RC_READ_NON_EXISTING_PAGE;
}

// Main function to read a block of data from the file
extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Step 1: Validate the page number
    RC status = validatePageNumber(pageNum, fHandle);
    if (status != RC_OK) {
        return status;
    }
    // Step 2: Calculate the offset for the requested page
    long offset = getPageOffset(pageNum);
    // Step 3: Retrieve the file pointer from the file handle
    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    // Step 4: Get the total file size
    long fileSize = getFileSize(filePtr);
    // Step 5: Move the file pointer to the calculated offset
    status = seekToOffset(filePtr, offset);
    if (status != RC_OK) {
        return status;
    }
    // Step 6: Determine the number of bytes to read
    int bytesToRead = calculateBytesToRead(fileSize, offset);
    // Step 7: Read the data from the file into memory
    status = readDataFromFile(filePtr, memPage, bytesToRead);
    if (status != RC_OK) {
        return status;
    }
    // Step 8: Update the current page position in the file handle
    fHandle->curPagePos = pageNum + 1;
    // Step 9: Return success
    return RC_OK;
}

// Function to retrieve the current block position in the file
extern int getBlockPos(SM_FileHandle *fHandle) {
    // Return the current page position from the file handle
    int currentPosition = fHandle->curPagePos;
    return currentPosition;
}

// Function to read the first block of the file
extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Call the readBlock function with page number 0 to read the first block
    RC result;
    result = readBlock(0, fHandle, memPage);  // Read the first block
    return result;  // Return the result of the read operation
}

// Function to read the previous block of the file
extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Get the current block position
    int currentBlockPos = getBlockPos(fHandle);
    // Calculate the previous block position
    int previousBlockPos = currentBlockPos - 1;
    
    // Check if the previous block exists
    if (previousBlockPos < 0) {
        return RC_READ_NON_EXISTING_PAGE;  // Error: trying to read before the first page
    }
    
    // Read the previous block by calling the readBlock function
    RC result = readBlock(previousBlockPos, fHandle, memPage);
    
    // If the read was successful, update the curPagePos
    if (result == RC_OK) {
        fHandle->curPagePos = previousBlockPos;  // Update curPagePos
    }
    
    // Return the result of the read operation
    return result;
}

// Function to read the current block of the file
extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Retrieve the current block position from the file handle
    int currentPage = getBlockPos(fHandle);
    
    // Check if the current block is valid
    if (currentPage < 0 || currentPage >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;  // Error: trying to read outside the valid page range
    }
    
    // Call readBlock to read the block at the current position
    RC readStatus = readBlock(currentPage, fHandle, memPage);
    
    // If the read was successful, update curPagePos (though it shouldn't change for current block)
    if (readStatus == RC_OK) {
        fHandle->curPagePos = currentPage;  // Update curPagePos
    }
    
    // Return the result of the read operation
    return readStatus;
}

// Function to read the next block of the file
extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Retrieve the current block position from the file handle
    int currentBlockPos = getBlockPos(fHandle);
    // Calculate the next block position
    int nextBlockPos = currentBlockPos + 1;
    
    // Check if the next block exists
    if (nextBlockPos >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;  // Error: trying to read beyond the last page
    }
    
    // Call readBlock to read the block at the next position
    RC status = readBlock(nextBlockPos, fHandle, memPage);
    
    // If the read was successful, update curPagePos
    if (status == RC_OK) {
        fHandle->curPagePos = nextBlockPos;  // Update curPagePos
    }
    
    // Return the result of the read operation
    return status;
}

// Function to read the last block of the file
extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Get the total number of pages in the file
    int totalPages = fHandle->totalNumPages;
    // Calculate the position of the last block
    int lastBlockPos = totalPages - 1;
    // Call readBlock to read the last block
    RC result = readBlock(lastBlockPos, fHandle, memPage);
    // Return the result of the read operation
    return result;
}

// Helper function to check if the page number is valid
RC checkPageValidity(int pageNum, SM_FileHandle *fHandle) {
    return (pageNum >= fHandle->totalNumPages) ? RC_WRITE_FAILED : RC_OK;
}

// Helper function to calculate the offset for the page
long calculatePageOffset(int pageNum) {
    return (pageNum + 1) * PAGE_SIZE;
}

// Helper function to move the file pointer to the correct offset
RC seekToPage(FILE *filePtr, long offset) {
    return (fseek(filePtr, offset, SEEK_SET) == 0) ? RC_OK : RC_WRITE_FAILED;
}

// Helper function to write data to the file
RC writeData(FILE *filePtr, SM_PageHandle memPage) {
    return (fwrite(memPage, sizeof(char), PAGE_SIZE, filePtr) == PAGE_SIZE) ? RC_OK : RC_WRITE_FAILED;
}

// Main function to write a block of data to the file
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Step 1: Validate the page number
    RC status = checkPageValidity(pageNum, fHandle);
    if (status != RC_OK) {
        return status;
    }
    // Step 2: Calculate the offset for the page
    long pageOffset = calculatePageOffset(pageNum);
    // Step 3: Retrieve the file pointer from the file handle
    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    // Step 4: Seek to the calculated offset in the file
    status = seekToPage(filePtr, pageOffset);
    if (status != RC_OK) {
        return status;
    }
    // Step 5: Write the data from memory to the file
    status = writeData(filePtr, memPage);
    if (status != RC_OK) {
        return status;
    }
    // Step 6: Update the current page position in the file handle
    fHandle->curPagePos = pageNum + 1;
    // Step 7: Return success
    return RC_OK;
}

// Function to write to the current block in the file
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Retrieve the current block position
    int currentBlockPos = getBlockPos(fHandle);
    // Write data to the current block by calling writeBlock
    RC result = writeBlock(currentBlockPos, fHandle, memPage);
    // Return the result of the write operation
    return result;
}

// Function to append an empty block to the file
extern RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Get the file pointer from the file handle
    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    // Move the file pointer to the end of the file
    if (fseek(filePtr, 0L, SEEK_END) != 0) {
        return RC_WRITE_FAILED;
    }
    // Use variables for character size and the number of characters
    int charSize = sizeof(char);
    int blockSize = PAGE_SIZE;
    // Calculate the number of characters based on the page size and character size
    int charCount = blockSize / charSize;
    // Allocate memory for the empty block and initialize it to zero
    char *buffer = (char *)malloc(charCount * charSize);
    if (buffer == NULL) {
        // Handle memory allocation failure
        return RC_WRITE_FAILED;
    }
    memset(buffer, 0, charCount);
    // Write the empty block to the file
    if (fwrite(buffer, sizeof(char), charCount, filePtr) < charCount) {
        // If writing fails, clean up resources and return an error code
        free(buffer);
        return RC_WRITE_FAILED;
    }
    // Free the allocated memory for the buffer
    free(buffer);
    // Update the total number of pages in the file handle
    fHandle->totalNumPages++;
    // Reset the file pointer to the beginning and update the page metadata
    if (fseek(filePtr, 0L, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }
    if (fprintf(filePtr, "%d", fHandle->totalNumPages) < 0) {
        return RC_WRITE_FAILED;
    }
    // Move the file pointer back to the current position
    if (fseek(filePtr, getBlockPos(fHandle) * PAGE_SIZE, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }
    // Return success
    return RC_OK;
}

// Function to ensure that the file has at least the specified number of pages
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    // Use a for loop to keep appending empty blocks until the total number of pages meets the required number
    for (; fHandle->totalNumPages < numberOfPages; ) {
        // Try to append an empty block to the file
        RC result = appendEmptyBlock(fHandle);
        // If appending fails, return a write failure code
        if (result != RC_OK) {
            return RC_WRITE_FAILED;
        }
    }
    // Once the capacity is ensured, return success
    return RC_OK;
}