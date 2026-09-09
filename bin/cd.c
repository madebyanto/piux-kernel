#include <stdint.h>
#include "../kernel/ext2.h"

typedef void (*vga_puts_t)(const char*);

extern ext2_filesystem_t fs;

void cmd_cd(const char *dirname, vga_puts_t vga_puts, void (*vga_putc)(char)) {
    if (*dirname == '\0') {
        vga_puts("Usage: cd <dirname>\n");
        return;
    }
    
    int inode = ext2_find_inode_in_dir(fs.current_dir_inode, dirname);
    
    if (inode < 0) {
        vga_puts("Directory not found\n");
        return;
    }
    
    ext2_inode_t inode_data;
    if (ext2_read_inode(inode, &inode_data) < 0) {
        vga_puts("Cannot read directory\n");
        return;
    }
    
    if ((inode_data.mode & EXT2_S_IFMT) != EXT2_S_IFDIR) {
        vga_puts("Not a directory\n");
        return;
    }
    
    fs.current_dir_inode = inode;
    vga_puts("Changed directory\n");
}
