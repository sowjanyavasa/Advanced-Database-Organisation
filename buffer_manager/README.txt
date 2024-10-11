##How to Execute the Script:

1. Open terminal.
2. Go to the project directory.
3. To remove any existing object files run make clean.
4. Run the command make.
5. To remove object files run the command make run.
6. To remove object files run make clean.

##Summary:
This task focused on building a Buffer Manager to handle memory pages that store data from files. The Buffer Manager works alongside the Storage Manager (developed in Assignment 1) and manages a fixed set of memory frames (pages) used to store file data.

##The key responsibilities of the Buffer Manager include:

1. Pinning and Unpinning Pages: Ensuring that file pages are loaded into memory when requested and remain available for use. Once no longer needed, pages are unpinned, making them eligible for replacement.
Page Replacement: Efficiently managing in-memory pages using strategies such as FIFO (First-In-First-Out) and LRU (Least Recently Used) to decide which pages to keep or evict from memory.
2. Dirty Page Tracking: Keeping track of modified (dirty) pages in memory to ensure they are written back to disk before being replaced, preventing data loss.

##Buffer Manager Functions:

1. updateBufferAndPageHandle(): Updates the buffer pool's internal data structures and the page handle with the new page number. It ensures that the page is correctly tracked and accessible by the client.
2. pinFIFO(): Pins a page using the FIFO page replacement strategy, loading the page into memory if necessary. If the buffer pool is full, it evicts the oldest page (based on FIFO) before loading the new one.
3. replacePageInClock(): Replaces a page in the buffer pool using the CLOCK page replacement strategy. The CLOCK algorithm cycles through pages and evicts based on usage flags.
4. initBufferPool(): Initializes a buffer pool with a specified number of page frames and assigns a page replacement strategy (e.g., FIFO, LRU). It sets up the required management data for handling pages and prepares the buffer for use.
5. markDirty(): Marks a page as dirty, indicating that it has been modified in memory. This ensures that the page will be written back to disk before being evicted.
6. loadPageFromDisk(): Reads a specific page from the disk and loads it into a page frame in memory. It handles the I/O operations and updates the page frame with the new page data.
7. updateClockQueue(): Updates the CLOCK page replacement queue to reflect page access and usage. It ensures that the algorithm knows which pages to evict when necessary based on usage patterns.
8. updateQueue(): Updates the queue by adding a new page or modifying an existing page entry based on the current state of the buffer pool. It ensures that the queue reflects the correct state of memory usage for page replacement.
9. handlePageReplacementLRU(): Manages page replacement using the LRU (Least Recently Used) strategy, evicting the least recently accessed page. It ensures that the buffer pool maintains efficiency by keeping frequently used pages in memory.
10. resetQueueEntry(): Resets a specific entry in the queue, marking it as free or uninitialized. This prepares the slot for a new page or reuses it during page replacement.
11. freePageFrameResources(): Frees all resources associated with a specific page frame, ensuring proper memory cleanup. This method helps avoid memory leaks when pages are removed or when the buffer pool is shut down.
12. unpinPage(): Decreases the fix count of a page, allowing it to be replaced if no clients are using it. If the fix count reaches zero, the page is eligible for eviction based on the replacement strategy.
13. updateQueueOnPageHit(): Updates the page replacement queue when a page is accessed (hit). It adjusts the page's position or state to ensure correct handling by the replacement strategy.
14. pinCLOCK(): Pins a page using the CLOCK replacement strategy, cycling through pages based on their usage flags. If the page is already in memory, it increments the fix count; otherwise, it loads the page and updates the CLOCK queue.
15. loadPageIntoFrameLRU(): Loads a page into memory using the LRU (Least Recently Used) replacement strategy. It identifies the least recently accessed page for eviction and replaces it with the new page.
16. updateLRUQueue(): Updates the LRU queue when a page is accessed, ensuring that the most recently used page is at the top. This helps maintain the correct order for future evictions based on recency of use.
17. flushDirtyPages(): Writes all dirty pages (modified pages) from the buffer pool back to disk, ensuring data integrity. It only flushes pages with a fix count of 0, meaning they are not being used by any client.
18. shutdownBufferPool(): Safely shuts down the buffer pool, writing all dirty pages to disk and freeing all associated memory. It ensures that no data is lost during the shutdown process.
19. writeDirtyPageToDisk(): Writes a specific dirty page from memory back to disk. This ensures that any modifications to the page are saved before it is replaced or evicted from memory.
20. handleBufferReplacement(): Manages the buffer replacement process by evicting a page and loading a new one. It ensures that the buffer pool maintains efficient memory use while following the specified replacement strategy.
21. loadPageIntoEmptyFrame(): Loads a new page into an available empty frame in the buffer pool. If no empty frames are available, the method will trigger the page replacement process to make room.
