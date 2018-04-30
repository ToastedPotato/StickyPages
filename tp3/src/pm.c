#include <stdio.h>
#include <string.h>


#include "conf.h"
#include "pm.h"

//used for updating the backing store at the very end of the execution
int dirty_ref[NUM_FRAMES];

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
  pm_backing_store = backing_store;
  pm_log = log;
  memset (pm_memory, '\0', sizeof (pm_memory));
}

// Charge la page demandée du backing store
void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
  download_count++;
  /* ¡ TODO: COMPLÉTER ! */
  //on place le curseur au bon endroit et on lit. J'assume ici que les frames
  //sont numérotées de 0 à 31
  fseek(pm_backing_store, page_number*256, SEEK_SET);
  fread(pm_memory+(frame_number*256), 256, 1, pm_backing_store);
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_page (unsigned int frame_number, unsigned int page_number)
{
  backup_count++;
  /* ¡ TODO: COMPLÉTER ! */
  //on place le curseur au bon endroit et on écrit. J'assume ici que les frames
  //sont numérotées de 0 à 31
  fseek(pm_backing_store, page_number*256, SEEK_SET);
  fwrite(pm_memory+(frame_number*256), 256, 1, pm_backing_store);
}

char pm_read (unsigned int physical_address)
{
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */
  if (physical_address < PHYSICAL_MEMORY_SIZE) {
    return pm_memory[physical_address];
  }
  return '!';
}

void pm_write (unsigned int physical_address, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */
  if (physical_address < PHYSICAL_MEMORY_SIZE) {
    pm_memory[physical_address] = c;
  }
}

//Met à jour la table de pages dirty dans les frames pour le backup final
void pm_dirty_update (unsigned int frame_number, int page_number){
    //valid page number if dirty, -1 otherwise
    dirty_ref[frame_number] = page_number; 
}

void pm_clean (void)
{
  // Enregistrement des données de la mémoire physique dans le backing store
  // à la fin de l'exécution
  for(int i = 0; i < NUM_FRAMES; i++){
    if(dirty_ref[i] != -1){
        pm_backup_page (i, dirty_ref[i]);
    }
    
  }
  // Enregistre l'état de la mémoire physique.
  if (pm_log)
    {
      for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
	{
	  if (i % 80 == 0)
	    fprintf (pm_log, "%c\n", pm_memory[i]);
	  else
	    fprintf (pm_log, "%c", pm_memory[i]);
	}
    }
  fprintf (stdout, "Page downloads: %2u\n", download_count);
  fprintf (stdout, "Page backups  : %2u\n", backup_count);
  fprintf (stdout, "PM reads : %4u\n", read_count);
  fprintf (stdout, "PM writes: %4u\n", write_count);
}
