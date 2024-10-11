// Including required Header Files
#include "dberror.h"
#include "stdio.h"
#include "storage_mgr.h"
#include "test_helper.h"


FILE* currentFile = NULL; 

void initStorageManager()
{
    if (currentFile != NULL)
    {
        currentFile = NULL;
    }
}
FILE* createFile(char *fileName)
{
    FILE* currentFile = fopen(fileName, "w+");
    return currentFile;
}
void writeEmptyPage(FILE* currentFile)
{
    if (currentFile != NULL)
    {
        char buffer[PAGE_SIZE] = {'\0'};  
        fwrite(buffer, sizeof(char), PAGE_SIZE, currentFile);
        fclose(currentFile); 
    }
}
RC createPageFile(char *fileName)
{
    FILE* currentFile = createFile(fileName);

    if (currentFile == NULL)
    {
        return (sprintf(RC_message, "File could not be created"), RC_FILE_NOT_FOUND);
    }   
    writeEmptyPage(currentFile); 
    RC_message = "File has been sucessfully created.";
    return (currentFile == NULL) ? RC_FILE_NOT_FOUND : RC_OK;
}
FILE* openFile(char *fileName)
{
    FILE* filePtr = fopen(fileName, "r+");
    return filePtr;
}
void setFileHandleInfo(SM_FileHandle *fHandle, char *fileName, FILE *currentFile)
{
    fHandle->fileName = fileName;
    fHandle->mgmtInfo = currentFile;
}
int calculateTotalPages(FILE *currentFile)
{
    fseek(currentFile, 0, SEEK_END); 
    long fileSize = ftell(currentFile);
    return (int)(fileSize / PAGE_SIZE); 
}
void resetFilePointerAndPagePos(SM_FileHandle *fHandle, FILE *currentFile)
{
    fseek(currentFile, 0, SEEK_SET); 
    fHandle->curPagePos = 0; 
}
void initializeFileHandle(SM_FileHandle *fHandle, FILE *currentFile, char *fileName)
{
    setFileHandleInfo(fHandle, fileName, currentFile);
    fHandle->totalNumPages = calculateTotalPages(currentFile);
    resetFilePointerAndPagePos(fHandle, currentFile);
}
RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
    FILE *currentFile = openFile(fileName);

    if(currentFile == NULL)
    {
        return (sprintf(RC_message, "File has not been found"), RC_FILE_NOT_FOUND);

    }
    else
    {
        initializeFileHandle(fHandle, currentFile, fileName);
        RC_message = "File has been successfully opened.";
        return RC_OK;
    }
}
RC closePage(SM_FileHandle *fileHandle)
{
    if (fileHandle->mgmtInfo != NULL)
    {
        fclose(fileHandle->mgmtInfo);
        fileHandle->mgmtInfo = NULL;
        sprintf(RC_message, "Successfully closed the file.");
        return RC_OK;
    }
    else
    {
        sprintf(RC_message, "Unable to close file: File not found.");
        return RC_FILE_NOT_FOUND;
    }
}
RC destroyPageFile(char *fileName)
{
    int result = remove(fileName);
    if (result == 0)
    {
        RC_message = "File was successfully deleted.";
        return RC_OK;
    }
    else
    {
        RC_message = "Unable to find the specified file.";
        return RC_FILE_NOT_FOUND;
    }
}
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle->mgmtInfo == NULL)
    {
        RC_message = "File has not been found.";
        return RC_FILE_NOT_FOUND;
    }
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
    {
        RC_message = "Page number is not valid";
        return RC_READ_NON_EXISTING_PAGE;
    }
    fseek(fHandle->mgmtInfo, pageNum * PAGE_SIZE, SEEK_SET);
    fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    fHandle->curPagePos = pageNum;
    RC_message = "File has been sucessfully read";
    return RC_OK;
}
int retrieveCurrentBlockPosition(SM_FileHandle *fHandle)
{
    int currentPosition = fHandle->curPagePos;
    return currentPosition;
}
int getCurrPage(SM_FileHandle *fHandle, char* type)
{
    int page = 0;
    if (strcmp(type, "last") == 0)
    {
        page = fHandle->totalNumPages - 1;
    }
    else if (strcmp(type, "prev") == 0)
    {
        page = fHandle->curPagePos - 1;
    }
    else if (strcmp(type, "curr") == 0)
    {
        page = fHandle->curPagePos;
    }
    else if (strcmp(type, "next") == 0)
    {
        page = fHandle->curPagePos + 1;
    }
    else
    {
        return 0;
    }
    return page;
}

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currentPage = getCurrPage(fHandle, "default");
    RC result = readBlock(currentPage, fHandle, memPage);
    return result;
}

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int lastPage = getCurrPage(fHandle, "last");
    RC result = readBlock(lastPage, fHandle, memPage);
    return result;
}

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int previousPage = getCurrPage(fHandle, "prev");
    RC readResult = readBlock(previousPage, fHandle, memPage);
    return readResult;
}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currentPage = getCurrPage(fHandle, "curr");
    RC readCurrResult = readBlock(currentPage, fHandle, memPage);
    return readCurrResult;
}

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int nextPage = getCurrPage(fHandle, "next");
    RC readNxtResult = readBlock(nextPage, fHandle, memPage);
    return readNxtResult;
}

RC writeDataToFile(SM_FileHandle *fHandle, SM_PageHandle memPage, int pageNum)
{
    long offset = pageNum * PAGE_SIZE;
    fseek(fHandle->mgmtInfo, offset, SEEK_SET);

    size_t totalBytesWritten = 0;
    while (totalBytesWritten < PAGE_SIZE)
    {
        size_t remainingBytes = PAGE_SIZE - totalBytesWritten; 
        size_t writeResult = fwrite(memPage + totalBytesWritten, sizeof(char), remainingBytes, fHandle->mgmtInfo);
        if (writeResult == 0)
        {
            return RC_WRITE_FAILED;
        }
        totalBytesWritten += writeResult;
    }
    
    return RC_OK;
}
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    RC rc = RC_OK;

    if (fHandle->mgmtInfo == NULL)
    {
        return (RC_message = "Unable to locate the specified file.", RC_FILE_NOT_FOUND);
    }
    else if (pageNum < 0 || pageNum > fHandle->totalNumPages)
    {
        return (RC_message = "Page number is not exist in the file", RC_READ_NON_EXISTING_PAGE);
    }
    else
    {
        rc = writeDataToFile(fHandle, memPage, pageNum);
        fHandle->curPagePos = pageNum;
        fclose(fHandle->mgmtInfo);
        RC_message = (rc == RC_OK) ? "Content written to File successfully." : "Error occurred during writing.";
    }

    return rc;
}
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int currentPage = getCurrPage(fHandle, "curr");
    RC wrireResult = writeBlock(currentPage, fHandle, memPage);
    return wrireResult;
}
void writeEmptyBlock(SM_FileHandle *fHandle)
{
    fseek(fHandle->mgmtInfo, 0, SEEK_END);  
    char *emptyBlock = (char *)malloc(PAGE_SIZE * sizeof(char));
    if (emptyBlock != NULL) {
        memset(emptyBlock, 0, PAGE_SIZE);
        fwrite(emptyBlock, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
        free(emptyBlock); 
    }  
    fHandle->totalNumPages++;
    fHandle->curPagePos = fHandle->totalNumPages - 1;
}

RC appendEmptyBlock(SM_FileHandle *fHandle)
{
    if (fHandle->mgmtInfo == NULL)
    {
        return (RC_message = "Unable to locate the specified file.", RC_FILE_NOT_FOUND);
    }
    else
    {
        writeEmptyBlock(fHandle);
        RC_message = "Appended an empty block at the end of the file successfully, filled with null values.";
        return RC_OK;
    }
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
	if(fHandle->mgmtInfo == NULL)
    {
        return (RC_message = "Unable to locate the specified file.", RC_FILE_NOT_FOUND);
    }
    else
    {
        int totalNumPages = fHandle->totalNumPages;
        while(numberOfPages > totalNumPages)
        {
            appendEmptyBlock(fHandle);
            totalNumPages = fHandle->totalNumPages;
        }
        return RC_OK;
    }
}
