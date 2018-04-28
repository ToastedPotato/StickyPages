
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

//utilisé pour clock
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
  //pour l'instant une boucle bête qui lit le tlb en entier jusqu'à tomber sur
  //l'entrée voulue
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
  //fprintf(stdout, "visited: %i\n", visited);
  return -1;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
  // TODO: COMPLÉTER CETTE FONCTION.
  //Si entrées vides
  if(filled_tlb_entries < TLB_NUM_ENTRIES){
	  tlb_entries[filled_tlb_entries].frame_number = frame_number;
      tlb_entries[filled_tlb_entries].page_number = page_number;
      tlb_entries[filled_tlb_entries].readonly = readonly;
      filled_tlb_entries++;
      return;
  }
  
  //CLOCK!!
  bool end_Queue = false;
  
  int i = hand;
  
  while(!end_Queue){
    
    if(tlb_entries[i].frame_number < 0){
	  //si entrée vide
	  
	  hand = i;
	  tlb_entries[i].frame_number = frame_number;
      tlb_entries[i].page_number = page_number;
      tlb_entries[i].readonly = readonly;
      return;
    }else {
        if(!tlb_entries[i].referenced){
           //si l'entrée n'a pas été référence, swap!
           
           hand = i;
	       tlb_entries[i].frame_number = frame_number;
           tlb_entries[i].page_number = page_number;
           tlb_entries[i].readonly = readonly;
           return; 
        }else{
           //sinon, en change le bit de référence
           tlb_entries[i].referenced = false; 
        }        
    }
        
    i++;
    
    //une fois qu'on arrive à la fin de la queue
    if(i == TLB_NUM_ENTRIES){
        i = 0;
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
