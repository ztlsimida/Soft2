#ifndef __BOOT_LIB_H
#define __BOOT_LIB_H
#include "typesdef.h"
uint32 get_boot_loader_addr();
void save_boot_loader_addr();
uint32 get_boot_total_size();
uint32 get_boot_version();
void increase_boot_version();
void set_boot_msg(uint8 *msg);
uint32 get_boot_svn_version();








#endif
