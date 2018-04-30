#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

//Needed for implementing the clock policy
unsigned int f_hand = 0;
unsigned int filled_frames = 0;

struct frame_ref
{  
  bool referenced : 1;
  bool readonly : 1;
  unsigned int page_number;
};

static struct frame_ref f_ref_table[NUM_FRAMES];

// Helper functions
int lookup_frame_number(unsigned int page_number, bool write);
unsigned int compute_page_number(unsigned int laddress);
unsigned int compute_offset(unsigned int laddress);
unsigned int compute_paddress(unsigned int frame_number, unsigned int offset);

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
		             unsigned int page,
                             unsigned int frame,
                             unsigned int offset,
                             unsigned int paddress, /* Physical address.  */
		             char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, 
        laddress, page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */

  // Translate logical adress to page number
  unsigned int page_number = compute_page_number(laddress);
  unsigned int offset = compute_offset(laddress);
  fprintf(stdout, "page : %d, offset : %d\n", page_number, offset);

  int frame_number = lookup_frame_number(page_number, false);
  
  // TODO : translate logical to physical adress
  unsigned int physical_address = compute_paddress(frame_number, offset);
  
  // Read from physical memory
  c = pm_read(physical_address);
  fprintf(stdout, "phys address : %d, char : %c\n", physical_address, c);
  
  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "READING", laddress, page_number, frame_number, 
    offset, physical_address, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */

  // Translate logical adress to page number
  unsigned int page_number = compute_page_number(laddress);
  unsigned int offset = compute_offset(laddress);
  fprintf(stdout, "page : %d, offset : %d\n", page_number, offset);
  
  int frame_number = lookup_frame_number(page_number, true);
  
  // Translate logical to physical adress
  unsigned int physical_address = compute_paddress(frame_number, offset);
  
  // Read from physical memory
  pm_write(physical_address, c);
  fprintf(stdout, "phys address : %d, char : %c\n", physical_address, c);
  
  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "WRITING", laddress, page_number, frame_number, 
    offset, physical_address, c);
}

int lookup_frame_number(unsigned int page_number, bool write) {
  // Lookup TLB
  int frame_number = tlb_lookup (page_number, write);
  
  // if TLB miss, lookup page table
  if(frame_number < 0) {

	frame_number = pt_lookup (page_number);
	
	if(frame_number < 0) {
	  // page fault
	  	  
	  //If there are free frames remaining
	  if(filled_frames < NUM_FRAMES){
	    frame_number = filled_frames;
	    filled_frames++;
	  }else{
	  
	      //Time to find a victim page/frame pair with CLOCK!
	      bool page_found = false;
      	  bool iteration = false;
      	  unsigned int visited = 0;
      	  
	      while(!page_found){
	        	        
	        if(!iteration){
	            //looking for (previously) unreferenced and unmodified pages
	            if(!f_ref_table[f_hand].referenced && 
	                !f_ref_table[f_hand].readonly){
	                //best/3rd best victim! (0,0/1,0)
	                frame_number = f_hand;
	                pt_unset_entry (f_ref_table[frame_number].page_number);
	                page_found = true;
	            }        
	        }else{
	            //looking for (previously) unreferenced and modified pages
	            if(!f_ref_table[f_hand].referenced && 
	                f_ref_table[f_hand].readonly){
	                //second best/worst victim (0,1/1,1)
	                frame_number = f_hand;
	                pm_backup_page (frame_number, 
	                    f_ref_table[frame_number].page_number);
                    pt_unset_entry (f_ref_table[frame_number].page_number);
                    page_found = true;
                }else{
                    //no luck yet, set ref bit to 0
                    f_ref_table[f_hand].referenced = false;
                }
	        }
	        //place hand at next position, also, modulo is for chickens   
	        f_hand = (f_hand+1) & (NUM_FRAMES-1);	        
	        
	        visited = (visited+1) & (NUM_FRAMES-1);
	        //went through all frames with no luck
	        if(visited == 0){    
	            iteration = !iteration;
	        }
	                    	      
	      }
	      	  
	  }
	  	  
	  // download page from backing store
	  pm_download_page (page_number, frame_number);
	  pt_set_entry (page_number, frame_number);
	  pt_set_readonly (page_number, !write);
	  
	  //update frame ref table
	  f_ref_table[frame_number].readonly = !write;
  	  f_ref_table[frame_number].referenced = true;
  	  f_ref_table[frame_number].page_number = page_number;
  	  
	}
	
	// Add to TLB - read only or not??
	tlb_add_entry (page_number, frame_number, pt_readonly_p(page_number));
  }
  return frame_number;
}

unsigned int compute_page_number(unsigned int laddress) {
	return laddress / PAGE_FRAME_SIZE;
}

unsigned int compute_offset(unsigned int laddress) {
	return laddress % PAGE_FRAME_SIZE;
}

unsigned int compute_paddress(unsigned int frame_number, unsigned int offset) {
	return frame_number * PAGE_FRAME_SIZE + offset;
}

// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
