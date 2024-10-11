#include "dberror.h"
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "dt.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "test_helper.h"

// Single Frame Structure
typedef struct PageFrameNode ;
PageFrameNode
{
    int FixCount;
    SM_PageHandle readContent;
    bool DirtyFlag;
    bool UsedFlag; // for clock strategy
    int FrameNum; 
    BM_PageHandle* bh; 
} PageFrameNode;

// Structure to store page-frame pairs
typedef struct Queue {
    int frameNumber; 
    int pageNumber; 
} Queue;

//Metadata for storing frame information
typedef struct PageFrameMD
{    
void *queue; 
int NumberOfFramesFilled;   
int NumberOfFrames;  
} PageFrameMD; 

// Global variables //
//Variables to store read/write
int NoOfWrites;
int NoOfReads;

//variable k used in LRU_K stratergy
int k;

int clockPosition = 0; // clock hand remembers positions between evictions 

struct PageFrameMD pfmd;

RC checkFileExistence(const char *fileName) {
    return (access(fileName, F_OK) != 0) ? RC_FILE_NOT_FOUND : RC_OK;
}

RC allocatePageFrameNodes(PageFrameNode **nodes, int count) {
    *nodes = (PageFrameNode *)malloc(sizeof(PageFrameNode) * count);
    return (*nodes == NULL) ? RC_FILE_NOT_FOUND : RC_OK;
}

RC allocateQueueMemory(Queue **frameQueue, int count) {
    *frameQueue = (Queue *)malloc(sizeof(Queue) * count);
    return (*frameQueue == NULL) ? RC_FILE_NOT_FOUND : RC_OK;
}

RC initializePageFrameNode(PageFrameNode *node, int index) {
    node->bh = MAKE_PAGE_HANDLE();
    node->readContent = (SM_PageHandle)malloc(PAGE_SIZE);

    if (node->readContent == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    node->bh->pageNum = NO_PAGE;
    node->bh->data = NULL;
    node->FrameNum = index;
    node->DirtyFlag = false;
    node->FixCount = 0;

    return RC_OK;
}

void resetQueueEntry(Queue *queueEntry) {
    queueEntry->pageNumber = NO_PAGE;
    queueEntry->frameNumber = NO_PAGE;
}

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData) {
    NoOfReads = 0;
    NoOfWrites = 0;

    if (checkFileExistence(pageFileName) != RC_OK) {
        return (RC_message = "Unable to locate the specified file.", RC_FILE_NOT_FOUND);
    }

    PageFrameNode *pageFrameNodes;
    Queue *frameQueue;

    if (allocatePageFrameNodes(&pageFrameNodes, numPages) != RC_OK) return RC_FILE_NOT_FOUND;

    if (allocateQueueMemory(&frameQueue, numPages) != RC_OK) {
        free(pageFrameNodes);
        return RC_FILE_NOT_FOUND;
    }

    for (int index = 0; index < numPages; index++) {
        // Initialize the page frame node
        RC rc = initializePageFrameNode(&pageFrameNodes[index], index);
        if (rc != RC_OK) {
            free(pageFrameNodes);
            free(frameQueue);
            return rc; // Handle allocation failure
        }

        // Reset the corresponding entry in the frame queue
        resetQueueEntry(&frameQueue[index]);
    }

    // Assign values to the buffer pool structure
    bm->numPages = numPages;
    bm->pageFile = (char *)pageFileName;
    bm->strategy = strategy;
    bm->mgmtData = pageFrameNodes;

    // Initialize the page frame metadata structure
    pfmd.NumberOfFrames = numPages;
    pfmd.NumberOfFramesFilled = 0;
    pfmd.queue = frameQueue;

    return RC_OK;
}

//shutting down buffer bool 
#include <stdlib.h>

RC freePageFrameResources(PageFrameNode *frame) {
    free(frame->bh);
    free(frame->readContent);
    return RC_OK;
}

RC flushDirtyPages(BM_BufferPool *const bm, PageFrameNode *pageFrames, int numPages) {
    bool flushRequired = false;

    for (int i = 0; i < numPages; i++) {
        if (pageFrames[i].DirtyFlag) {
            forcePage(bm, pageFrames[i].bh);
            flushRequired = true;
        }
    }

    return flushRequired;
}

RC shutdownBufferPool(BM_BufferPool *const bm) {
    if (bm->mgmtData == NULL) {
        RC_message = "Shut down was unsuccessful as buffer pool does not exist.";
        return RC_FILE_NOT_FOUND;
    }

    PageFrameNode *pageFrames = (PageFrameNode *)bm->mgmtData;
    
    // Flush dirty pages if any
    bool flushRequired = flushDirtyPages(bm, pageFrames, bm->numPages);

    // Free memory allocated for each page frame
    for (int i = 0; i < bm->numPages; i++) {
        freePageFrameResources(&pageFrames[i]);
    }

    // Free queue and buffer pool management data
    free(pfmd.queue);
    free(bm->mgmtData);

    // Reset buffer pool properties
    bm->mgmtData = NULL;
    bm->numPages = 0;
    NoOfReads = 0;
    NoOfWrites = 0;

    // Set shutdown message based on flush status
       // Set the shutdown message based on whether flushing was required
    if (flushRequired) {
        RC_message = "Shut down successful with flushing dirty pages.";
    } else {
        RC_message = "Shut down successful without flushing any pages.";
    }

    
    return RC_OK;
}


// Flushing all dirty Pages
RC forceFlushPool(BM_BufferPool *const bm)
{
    // Validate if the buffer pool exists before proceeding
    if (bm->mgmtData == NULL)
    {
        RC_message = "Buffer pool does not exist. Force flush unsuccessful.";
        return RC_FILE_NOT_FOUND;
    }

    PageFrameNode *pageFrameList = (PageFrameNode *)bm->mgmtData;
    bool flushSuccessful = true;

    // Loop through all page frames in the buffer pool
    for (int pageIndex = 0; pageIndex < bm->numPages; pageIndex++)
    {
        // If the page is dirty and not in use, flush it
        if (pageFrameList[pageIndex].DirtyFlag == 1 && pageFrameList[pageIndex].FixCount == 0)
        {
            RC flushResult = forcePage(bm, pageFrameList[pageIndex].bh);
            if (flushResult != RC_OK)
            {
                flushSuccessful = false;
                RC_message = "Unable to flush all pages. Force flush failed.";
                break;
            }
        }
    }

    // Return the appropriate result after attempting to flush
    if (flushSuccessful)
    {
        RC_message = "Force flush completed successfully.";
        return RC_OK;
    }
    else
    {
        return RC_FILE_NOT_FOUND;
    }
}



/// Interface Access Pages///

// write the current content of the page back to the page file on disk
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // Validate if the buffer pool's management data exists
    if (bm->mgmtData == NULL)
    {
        RC_message = "Buffer pool not found.";
        return RC_FILE_NOT_FOUND;
    }

    PageFrameNode *pageFrameList = (PageFrameNode *)bm->mgmtData;
    SM_FileHandle fileHandle;
    bool pageFound = false;
    int targetFrameIndex = -1;

    // Loop through the buffer pool to find the matching dirty page
    for (int frameIndex = 0; frameIndex < bm->numPages; frameIndex++)
    {
        if (pageFrameList[frameIndex].bh->pageNum == page->pageNum &&
            pageFrameList[frameIndex].DirtyFlag == 1)
        {
            targetFrameIndex = frameIndex;
            pageFound = true;
            break;
        }
    }

    // Return error if the target page is not dirty
    if (!pageFound)
    {
        RC_message = "The given page number is not marked as dirty.";
        return RC_FILE_NOT_FOUND;
    }

    // Open the file and write the page data to disk
    openPageFile(bm->pageFile, &fileHandle);
    pageFrameList[targetFrameIndex].readContent = page->data;
    ensureCapacity(page->pageNum, &fileHandle);
    writeBlock(page->pageNum, &fileHandle, pageFrameList[targetFrameIndex].readContent);

    // Mark the page as clean after writing to disk
    pageFrameList[targetFrameIndex].DirtyFlag = 0;
return (RC_message = "Page successfully written and flushed to disk.", RC_OK);

}

//making pages dirty
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // Validate if the buffer pool's management data is available
    if (bm->mgmtData == NULL)
    {
        RC_message = "Error: Buffer pool is not initialized or is unavailable.";
        return RC_FILE_NOT_FOUND;
    }

    PageFrameNode *pageFrameList = (PageFrameNode *)bm->mgmtData;
    bool pageMarkedDirty = false;

    // Loop through the buffer pool to find the page
    for (int frameIndex = 0; frameIndex < bm->numPages; frameIndex++)
    {
        if (pageFrameList[frameIndex].bh->pageNum == page->pageNum)
        {
            // Mark the page as dirty and increment the write count
            pageFrameList[frameIndex].DirtyFlag = 1;
            NoOfWrites++;
            pageMarkedDirty = true;
            break;
        }
    }

    // Set success or error message based on whether the page was found and marked dirty
    return (pageMarkedDirty ? (RC_message = "Success: Page flagged as dirty.", RC_OK) : (RC_message = "Error: Page not found in buffer pool, could not mark as dirty.", RC_FILE_NOT_FOUND));

}



//unpinning the page
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // Get the page frame nodes from the buffer pool
    PageFrameNode *pageFrame = (PageFrameNode *)bm->mgmtData;
    bool pageFoundAndUnpinned = false;

    // Iterate through the page frames to find the matching page
    for (int i = 0; i < bm->numPages; i++)
    {
        if (pageFrame[i].bh->pageNum == page->pageNum && pageFrame[i].FixCount > 0)
        {
            pageFrame[i].FixCount--;
            pageFoundAndUnpinned = true;
            break;
        }
    }

    // Single line for message and return
    return (pageFoundAndUnpinned ? (RC_message = "Page successfully unpinned.", RC_OK) : (RC_message = "Failed to unpin page as it is not available in the buffer pool.", RC_FILE_NOT_FOUND));
}

// Exit Strategies //
// Function to write dirty page back to disk
void writeDirtyPageToDisk(PageFrameNode *pageFrame, SM_FileHandle *fileHandle) 
{
    // Fetch the dirty page's data from its buffer
    SM_PageHandle bufferData = pageFrame->bh->data;

    // Store the buffer data into the page frame's readContent field
    pageFrame->readContent = bufferData;

    // Write the data to the disk at the appropriate block
    writeBlock(pageFrame->bh->pageNum, fileHandle, bufferData);
}

// Function to ensure disk capacity and read new page from disk
void loadPageFromDisk(PageFrameNode *pageFrame, SM_FileHandle *fileHandle, PageNumber pageNum)
{
    // Ensure enough capacity in the file before loading the page
    ensureCapacity(pageNum, fileHandle);

    // Read the new page data into the buffer
    readBlock(pageNum, fileHandle, pageFrame->readContent);

    // Update the page frame with the new data
    pageFrame->bh->pageNum = pageNum;
    pageFrame->bh->data = pageFrame->readContent;
    pageFrame->DirtyFlag = 0;  // Mark the page as clean
    pageFrame->FixCount = 1;   // Set FixCount as it is now pinned
}

// Function to update the buffer page and page handle with new page data
void updateBufferAndPageHandle(PageFrameNode *pageFrame, BM_PageHandle *page, PageNumber pageNum) 
{
    // Update the page frame's pageNum and data
    pageFrame->bh->pageNum = pageNum;
    pageFrame->bh->data = pageFrame->readContent;

    // Update the page handle to reflect the new page's data
    page->pageNum = pageFrame->bh->pageNum;
    page->data = pageFrame->bh->data;
}

// Function to shift queue and insert new page at the end
void updateQueue(Queue *queue, int numPages, PageNumber pageNum, int frameNumber)
{
    // Shift pages in the queue
    for (int z = 1; z < numPages; z++)
    {
        queue[z - 1].pageNumber = queue[z].pageNumber;
        queue[z - 1].frameNumber = queue[z].frameNumber;
    }

    // Place the new page at the end of the queue
    queue[numPages - 1].pageNumber = pageNum;
    queue[numPages - 1].frameNumber = frameNumber;
}

// Main pinFIFO function
bool pinFIFO(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    Queue *queue = (Queue *)pfmd.queue;
    SM_FileHandle fileHandle;
    PageFrameNode *pageFrameList = (PageFrameNode *)bm->mgmtData;
    bool pagePinned = false;

    for (int i = 0; i < bm->numPages && !pagePinned; i++)
    {
        int targetPageNum = queue[i].pageNumber;

        for (int j = 0; j < bm->numPages && !pagePinned; j++)
        {
            PageFrameNode *currentFrame = &pageFrameList[j];

            // Check if the frame can be replaced
            if (currentFrame->bh->pageNum == targetPageNum)
            {
                if (currentFrame->FixCount == 0)
                {
                    openPageFile(bm->pageFile, &fileHandle);

                    // Check dirty status
                    if (currentFrame->DirtyFlag == 1)
                    {
                        writeDirtyPageToDisk(currentFrame, &fileHandle);
                    }

                    // Load the new page from disk
                    loadPageFromDisk(currentFrame, &fileHandle, pageNum);
                    
                    // Update the page handle with new data
                    updateBufferAndPageHandle(currentFrame, page, pageNum);

                    // Adjust the queue with new page info
                    updateQueue(queue, bm->numPages, pageNum, currentFrame->FrameNum);

                    // Mark page as pinned and increment read counter
                    NoOfReads++;
                    pagePinned = true;
                }
            }
        }
    }

    return pagePinned;
}



// Function to handle dirty and clean page actions
// Function to handle dirty and clean page actions
void handlePageReplacementLRU(BM_BufferPool *const bm, PageFrameNode *pageFrame, PageNumber pageNum, SM_FileHandle *fHandle)
{
    if (pageFrame->DirtyFlag == 0)
    {
        // Handle clean page, ensuring file capacity
        openPageFile(bm->pageFile, fHandle);
        ensureCapacity(pageNum, fHandle);
    }
    else
    {
        // Handle dirty page by forcing it to disk
        forcePage(bm, pageFrame->bh);
    }
}

// Function to read the new page from disk and assign to buffer page handler
void loadPageIntoFrameLRU(BM_BufferPool *const bm, BM_PageHandle *const page, PageFrameNode *pageFrame, PageNumber pageNum, SM_FileHandle *fHandle)
{
    // Read block from disk into the buffer
    readBlock(pageNum, fHandle, pageFrame->readContent);

    // Assign the page number to both the page frame and page handle
    pageFrame->bh->pageNum = pageNum;
    page->pageNum = pageNum;

    // Copy the data from the buffer to both the page frame and page handle
    pageFrame->bh->data = pageFrame->readContent;
    page->data = pageFrame->readContent;

    // Mark the page as clean and reset the fix count
    pageFrame->DirtyFlag = 0;
    pageFrame->FixCount = 0;
}

// Function to update the queue after pinning the page
void updateLRUQueue(Queue *queue, int i, int numPages, PageFrameNode *pageFrame, PageNumber pageNum)
{
    for (int z = i + 1; z < numPages; z++)
    {
        queue[z - 1].pageNumber = queue[z].pageNumber;
        queue[z - 1].frameNumber = queue[z].frameNumber;
    }

    int lastPageIdx = numPages - 1;
    queue[lastPageIdx].pageNumber = pageNum;
    queue[lastPageIdx].frameNumber = pageFrame->FrameNum;
}

bool pinLRU(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    bool isSet = false;
    SM_FileHandle fHandle;
    PageFrameNode *pageFrameList = (PageFrameNode *)bm->mgmtData;
    Queue *queue = (Queue *)pfmd.queue;

    for (int i = 0; i < bm->numPages && !isSet; i++)
    {
        int currentPageNumber = queue[i].pageNumber;

        for (int j = 0; j < bm->numPages && !isSet; j++)
        {
            if (pageFrameList[j].bh->pageNum == currentPageNumber && pageFrameList[j].FixCount == 0)
            {
                // Handle the page replacement process (dirty/clean page)
                handlePageReplacementLRU(bm, &pageFrameList[j], pageNum, &fHandle);

                // Load the page into the frame and update the page handler
                loadPageIntoFrameLRU(bm, page, &pageFrameList[j], pageNum, &fHandle);

                // Update the LRU queue
                updateLRUQueue(queue, i, bm->numPages, &pageFrameList[j], pageNum);

                // Increment read count and mark as set
                NoOfReads++;
                isSet = true;
            }
        }
    }

    return isSet;
}

/// Function to handle page replacement in the CLOCK algorithm
void replacePageInClock(BM_BufferPool *const bm, PageFrameNode *pageFrame, PageNumber pageNum, SM_FileHandle *fHandle)
{
    // Open the file associated with the buffer pool
    openPageFile(bm->pageFile, fHandle);

    // If the page is dirty, write its contents to disk
    if (pageFrame->DirtyFlag == 1)
    {
        forcePage(bm, pageFrame->bh);
    }
    else
    {
        // Ensure there is sufficient capacity for the new page
        ensureCapacity(pageNum, fHandle);
    }
}

// Function to load a page into the frame and update the buffer manager's page handle
void loadNewPageIntoFrameCLOCK(PageFrameNode *pageFrame, BM_PageHandle *const page, PageNumber pageNum, SM_FileHandle *fHandle)
{
    // Read the new page from disk into the frame's readContent buffer
    readBlock(pageNum, fHandle, pageFrame->readContent);

    // Update the page number and content of both the page frame and the page handle
    pageFrame->bh->pageNum = pageNum;
    page->pageNum = pageNum;

    pageFrame->bh->data = pageFrame->readContent;
    page->data = pageFrame->readContent;

    // Mark the page as clean and set its FixCount to 1
    pageFrame->DirtyFlag = 0;
    pageFrame->FixCount = 1;
}

// Function to update the queue after replacing a page
void updateClockQueue(Queue *queue, int i, int j, int numPages, PageFrameNode *pageFrame, PageNumber pageNum)
{
    // Shift the pages in the queue to update their positions
    queue[i].pageNumber = queue[j].pageNumber;
    queue[i].frameNumber = queue[j].frameNumber;

    // Place the newly loaded page at the end of the queue
    queue[numPages - 1].pageNumber = pageNum;
    queue[numPages - 1].frameNumber = pageFrame->FrameNum;
}

// Main pinCLOCK function implementation
bool pinCLOCK(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    bool isPagePinned = false;  // Flag to track whether a page has been pinned
    SM_FileHandle fileHandle;  // File handle for reading/writing pages to/from disk
    PageFrameNode *pageFrameList = (PageFrameNode *)bm->mgmtData;  // Retrieve the page frames from the buffer manager
    Queue *queue = (Queue *)pfmd.queue;  // Retrieve the queue

    for (int i = 0; i < bm->numPages && !isPagePinned; i++)
    {
        int currentPageNumber = queue[i].pageNumber;  // Retrieve the current page number from the queue

        for (int j = clockPosition; j < bm->numPages && !isPagePinned; j++)
        {
            PageFrameNode *currentFrame = &pageFrameList[j];  // Get the current frame being checked

            // Check whether the page number matches and if it's not currently pinned
            if (currentFrame->bh->pageNum == currentPageNumber)
            {
                // Separate out FixCount check for clarity
                if (currentFrame->FixCount == 0)
                {
                    if (currentFrame->UsedFlag == 1)
                    {
                        // If the page was recently used, reset the flag and continue
                        currentFrame->UsedFlag = 0;
                    }
                    else
                    {
                        // The page is not recently used, so handle replacement and pinning
                        currentFrame->UsedFlag = 1;
                        replacePageInClock(bm, currentFrame, pageNum, &fileHandle);  // Handle page replacement
                        loadNewPageIntoFrameCLOCK(currentFrame, page, pageNum, &fileHandle);  // Load the new page into the frame
                        updateClockQueue(queue, i, j, bm->numPages, currentFrame, pageNum);  // Update the queue for the newly loaded page

                        NoOfReads++;  // Increment the read count
                        clockPosition = (j + 1) % bm->numPages;  // Update the clock hand position
                        isPagePinned = true;
                    }
                }
            }
        }
    }

    return isPagePinned;  // Return whether the page has been pinned or not
}



// Function prototype for updateQueueOnPageHit
void updateQueueOnPageHit(Queue *queue, int j, int numPages, PageFrameNode *pageFrame, int frameNumber, PageNumber pageNum);

// Function to check if the page is already present in the buffer pool
bool checkPageInBuffer(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum, Queue *queue, PageFrameNode *pageFrame)
{
    // Iterate through the buffer pool's pages
    for (int j = 0; j < bm->numPages; j++)
    {
        // Compare the current page number in the queue with the required page number
        if (!(queue[j].pageNumber - pageNum))  // Alternative to queue[j].pageNumber == pageNum
        {
            // Extract the frame index from the queue
            int frameIndex ;
            frameIndex= queue[j].frameNumber;

            // Retrieve the page handle associated with the frame
            BM_PageHandle* currentBufferHandle;
             currentBufferHandle= pageFrame[frameIndex].bh;

            // Update the page handle's information (page number and data)
            page->pageNum = currentBufferHandle->pageNum;
            page->data = currentBufferHandle->data;

            // Increment the frame's fix count
            ++pageFrame[frameIndex].FixCount;

            // Adjust the queue to reflect that this page has been accessed
            updateQueueOnPageHit(queue, j, bm->numPages, pageFrame, frameIndex, pageNum);

            // The page is already in the buffer, return true
            return true;
        }
    }

    // Page not found in the buffer, return false
    return false;
}



// Function to update the queue when the page is already in the buffer pool
void updateQueueOnPageHit(Queue *queue, int j, int numPages, PageFrameNode *pageFrame, int frameNumber, PageNumber pageNum)
{
    for (int z = j + 1; z < numPages; z++)
    {
        queue[z - 1].pageNumber = queue[z].pageNumber;
        queue[z - 1].frameNumber = queue[z].frameNumber;
    }

    int lastIdx = numPages - 1;
    queue[lastIdx].pageNumber = pageNum;
    queue[lastIdx].frameNumber = pageFrame[frameNumber].FrameNum;
}

// Function to handle buffer replacement when the buffer is full (if-else version)
bool handleBufferReplacement(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    if (bm->strategy == RS_FIFO)
    {
        return pinFIFO(bm, page, pageNum);
    }
    else if (bm->strategy == RS_LRU)
    {
        return pinLRU(bm, page, pageNum);
    }
    else if (bm->strategy == RS_LRU_K)
    {
        return pinLRU(bm, page, pageNum); // Handle LRU_K as a fallback strategy
    }
    else if (bm->strategy == RS_CLOCK)
    {
        return pinCLOCK(bm, page, pageNum);
    }
    else
    {
        // Return false if the strategy is unknown
        return false;
    }
}

// Function to find an empty frame and load the page into it
bool loadPageIntoEmptyFrame(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum, Queue *queue, PageFrameNode *pageFrame)
{
    SM_FileHandle fHandle;

    // Iterate over the buffer pool's frames to find an empty one
    for (int i = 0; i < bm->numPages; i++)
    {
        // Check if the page frame is empty by verifying the page number is invalid
        if (pageFrame[i].bh->pageNum < 0)
        {
            // Open the associated file for the buffer pool
            openPageFile(bm->pageFile, &fHandle);

            // Ensure that the file has the required capacity
            ensureCapacity(pageNum, &fHandle);

            // Fetch the content from the block
            SM_PageHandle currentPageData ;
            currentPageData= pageFrame[i].readContent;
            readBlock(pageNum, &fHandle, currentPageData);

         // Define variables to hold the new state
int isDirty = 0; // Not dirty
int fixCount = 1; // Pinned state

// Set the page frame properties using the defined variables
pageFrame[i].DirtyFlag = isDirty; 
pageFrame[i].FixCount = fixCount; 

// Store the read content into the page handle
SM_PageHandle pageContent = currentPageData; // Store the read content
page->data = pageContent;
page->pageNum = pageNum;

// Update the buffer frame's page handle data and page number
BM_PageHandle* bufferHandle = pageFrame[i].bh; // Store the buffer handle
bufferHandle->data = pageContent;
bufferHandle->pageNum = pageNum;

// Reflect the newly loaded page in the queue
int frameNumber = pageFrame[i].FrameNum; // Store the frame number
queue[i].frameNumber = frameNumber;
queue[i].pageNumber = pageNum;

// Increment metadata for filled frames and read operations
pfmd.NumberOfFramesFilled++; // Increment filled frame count
NoOfReads++; // Increment the number of read operations


            return true;
        }
    }

    return false;  // Return false if no empty frame was found
}


// Main pinPage function
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    // Check if the buffer pool exists and page number is valid
   if (bm->mgmtData == NULL) {
    // If the management data is NULL, the buffer pool does not exist
    return RC_FILE_NOT_FOUND; 
} else if (pageNum < 0) {
    // If the page number is negative, it is considered invalid
    return RC_FILE_NOT_FOUND; 
}


    bool isPageSet;
   isPageSet  = false;
    Queue *queue ;
    queue= (Queue *)pfmd.queue;
    PageFrameNode *pageFrame = (PageFrameNode *)bm->mgmtData;

    // Check if the page is already in the buffer pool
    if (checkPageInBuffer(bm, page, pageNum, queue, pageFrame))
    {
        return RC_OK;
    }

    // If the buffer pool is full, handle page replacement based on the strategy
    if (pfmd.NumberOfFramesFilled == pfmd.NumberOfFrames)
    {
        isPageSet = handleBufferReplacement(bm, page, pageNum);
    }
    else
    {
        // Otherwise, find an empty frame and load the page
        isPageSet = loadPageIntoEmptyFrame(bm, page, pageNum, queue, pageFrame);
    }

    return isPageSet ? RC_OK : RC_FILE_NOT_FOUND;
}

//returns array with all the page numbers in the buffer pool.
PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    // Allocate memory for the array of PageNumbers
    PageNumber *pageNumbers ;
    pageNumbers = malloc(sizeof(PageNumber) * bm->numPages);
    
    // Access the page frames from the buffer pool's management data
    PageFrameNode *pageFrame ;
    pageFrame = (PageFrameNode *)bm->mgmtData;
    
    // Iterate over the page frames using a for loop
    for (int i = 0; i < bm->numPages; i++)
{
    // Check if the frame is empty and assign the appropriate value
    int currentPageNum = pageFrame[i].bh->pageNum;
    pageNumbers[i] = (currentPageNum == -1) ? NO_PAGE : currentPageNum;
}

// Finally, return the array of page numbers
return pageNumbers;

}


// Returns an array with dirty flags for each frame in the buffer pool
bool *getDirtyFlags(BM_BufferPool *const bm)
{

    bool *dirtyFlags;
    dirtyFlags = malloc(sizeof(bool) * bm->numPages);
    

    PageFrameNode *pageFrame;
    pageFrame= (PageFrameNode *)bm->mgmtData;

    for (int i = 0; i < bm->numPages; i++)
    {

        dirtyFlags[i] = pageFrame[i].DirtyFlag ? true : false;
    }


    return dirtyFlags;
}

//returns array with all the fixed counts for each frame in the buffer pool.
int *getFixCounts(BM_BufferPool *const bm)
{
    // Allocate memory for the array of fix counts
    int *FixCounts ;
    FixCounts = malloc(sizeof(int) * bm->numPages);
    
    // Access the page frames from the buffer pool's management data
    PageFrameNode *pageFrame ;
    pageFrame = (PageFrameNode *)bm->mgmtData;

    // Iterate over each page frame using a for loop
    for (int i = 0; i < bm->numPages; i++)
    {
        // Set the fix count for each page frame, or NO_PAGE if FixCount is -1
        FixCounts[i] = (pageFrame[i].FixCount != -1) ? pageFrame[i].FixCount : NO_PAGE;
    }

    // Return the array of fix counts
    return FixCounts;
}


//returns total number of wites
int getNumWriteIO (BM_BufferPool *const bm)
{
   return NoOfWrites;
}

//returns total number of reads
int getNumReadIO (BM_BufferPool *const bm)
{
   return NoOfReads;
}