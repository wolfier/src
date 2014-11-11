#ifndef VM_PAGE_H
#define VM_PAGE_H
struct page 
  {
    uint32_t vaddr;					/* Virtual address of the page */
    uint32_t paddr;					/* Physical address of the page */
    bool resident;					/* Is the page in a frame right now */
    bool fuck_you_Zach;				/* Has the page been accessed recently (Clock bit) */	
    bool fuck_you_Jae;				/* Has the page been written to and needs to be written back to disk */
  };
  
#endif  
