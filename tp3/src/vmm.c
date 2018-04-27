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
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */

  // Translate logical adress to page number
  // TODO : 
  fprintf(stdout, "%d", laddress);
  unsigned int page_number = 0;
  
  // Lookup TLB
  int frame_number = tlb_lookup (page_number, false);
  
  // if TLB hit, read from central memory
  if(frame_number < 0) {
	// if TLB miss, lookup page table
	frame_number = pt_lookup (page_number);
	
	if(frame_number < 0) {
	  // page fault
	  // read page from backing store
	  
	  // TODO : pick the frame to swap out?
	  frame_number = 0;
	  // Check if frame needs to be written
	  // if yes, backup to disk
	  
	  //pm_download_page (page_number, frame_number);
	  //pt_set_entry (page_number, frame_number);
	}
	
	// Add to TLB - read only or not??
	tlb_add_entry (page_number, frame_number, true);
  }
  
  // TODO : translate logical to physical adress
  unsigned int physical_address = 0;
  
  // Read from physical memory
  c = pm_read(physical_address);
  
  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "READING", laddress, 0, 0, 0, 0, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */

  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "WRITING", laddress, 0, 0, 0, 0, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
