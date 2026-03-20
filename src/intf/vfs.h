#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VFS_FILE,
    VFS_DIRECTORY
} NodeType;

struct Node {
    char name[256];
    NodeType type;
    char* data;
    size_t data_size;
    struct Node* parent;
    struct Node* first_child;
    struct Node* next_sibling;
};

extern struct Node* root;
extern struct Node* current_dir;

void init_filesystem();
struct Node* find_child(struct Node* directory, char* name);
struct Node* vfs_mkdir(char* name);
struct Node* vfs_touch(char* name);
struct Node* vfs_ls();
int vfs_cd(char* name);
int vfs_cat(char* path, char** out);
int vfs_rm(char* name);
char* vfs_pwd();
struct Node* resolve_path(char* path);
int vfs_write(char* path, char* data, int append);