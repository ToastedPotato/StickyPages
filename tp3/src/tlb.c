
#include <stdint.h>
#include <stdio.h>

#include "tlb.h"

#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number;             /* Invalide si négatif.  */
  bool readonly : 1;
  bool referenced : 1;
};

//Needed for implementing CLOCK
unsigned int hand = 0;
unsigned int filled_tlb_entries = 0;

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

/* Initialise le TLB, et indique où envoyer le log des accès.  */
void tlb_init (FILE *log)
{
  for (int i = 0; i < TLB_NUM_ENTRIES; i++)
    tlb_entries[i].frame_number = -1;
  tlb_log = log;
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans le TLB.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
static int tlb__lookup (unsigned int page_number, bool write)
{
  // TODO: COMPLÉTER CETTE FONCTION.
  //Reads the TLB starting from the CLOCK "hand" until it checks all the entry
  bool end_Queue = false;
  
  int i = hand;
  
  int visited = 0;
   
  while(!end_Queue){
    if(tlb_entries[i].page_number == page_number){
	  if(write) {
		  tlb_entries[i].readonly = false;
	  }
	  hand = i;
	  tlb_entries[i].referenced = true;
      return tlb_entries[i].frame_number;
    }
    
    i++;
    visited++;
    
    //une fois qu'on arrive à la fin de la queue
    if(i == TLB_NUM_ENTRIES){
        i = 0;
    }
    
    //On a visité toutes les entrées dans le TLB
    if(visited == TLB_NUM_ENTRIES){end_Queue = true;}
    
  }
  return -1;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
  // TODO: COMPLÉTER CETTE FONCTION.
  //If there are empty entries
  if(filled_tlb_entries < TLB_NUM_ENTRIES){
	  tlb_entries[filled_tlb_entries].frame_number = frame_number;
      tlb_entries[filled_tlb_entries].page_number = page_number;
      tlb_entries[filled_tlb_entries].readonly = readonly;
      tlb_entries[filled_tlb_entries].referenced = true;
      filled_tlb_entries++;
      return;
  }
  
  //CLOCK!!
 
  while(true){
    
    if(!tlb_entries[hand].referenced){
       //no reference, swap!
       
       tlb_entries[hand].frame_number = frame_number;
       tlb_entries[hand].page_number = page_number;
       tlb_entries[hand].readonly = readonly;
       tlb_entries[hand].referenced = true;
       
       //setting the hand to its new position
       hand++;
    
       //end of queue
       if(hand == TLB_NUM_ENTRIES){
            hand = 0;
       }
       return; 
    }else{
       //flip reference bit
       tlb_entries[hand].referenced = false;
    }
    
    //next position
    hand++;
    
    //end of queue
    if(hand == TLB_NUM_ENTRIES){
          hand = 0;
    }        
  }
      
}

/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void tlb_add_entry (unsigned int page_number,
                    unsigned int frame_number, bool readonly)
{
  tlb_mod_count++;
  tlb__add_entry (page_number, frame_number, readonly);
}

int tlb_lookup (unsigned int page_number, bool write)
{
  int fn = tlb__lookup (page_number, write);
  (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
  return fn;
}

/* Imprime un sommaires des accès.  */
void tlb_clean (void)
{
  fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
  fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
  fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
  fprintf (stdout, "TLB miss rate: %.1f%%\n",
           100 * tlb_miss_count
           /* Ajoute 0.01 pour éviter la division par 0.  */
           / (0.01 + tlb_hit_count + tlb_miss_count));
}
