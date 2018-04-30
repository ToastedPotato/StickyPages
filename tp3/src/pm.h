#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#include <stdint.h>
#include <stdio.h>

/* Initialise la mémoire physique.  */
void pm_init (FILE *backing_store, FILE *log);

void pm_download_page (unsigned int page_number, unsigned int frame_number);
void pm_backup_page (unsigned int frame_number, unsigned int page_number);
char pm_read (unsigned int physical_address);
void pm_write (unsigned int physical_address, char);
//Met à jour la table de pages dirty dans les frames pour le backup final
void pm_dirty_update (unsigned int frame_number, int page_number);
void pm_clean (void);

#endif
