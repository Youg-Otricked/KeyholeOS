#include "vfs.h"
#include "kmemory.h"
#include "string.h"
#include "print.h"
struct Node* root;
struct Node* current_dir;
void init_filesystem() {
    root = (struct Node*)kmalloc(sizeof(struct Node));
    strcpy(root->name, "/");
    root->type = VFS_DIRECTORY;
    root->data = NULL;
    root->data_size = 0;
    root->parent = root;
    root->first_child = NULL;
    root->next_sibling = NULL;
    
    current_dir = root;
    sudo_mode = 1;
    vfs_mkdir("home");
    vfs_mkdir("etc");
    vfs_mkdir("tmp");
    vfs_mkdir("bin");
    vfs_mkdir("var");
    
    vfs_cd("home");
    vfs_mkdir("root");
    vfs_cd("/etc");
    vfs_touch("hostname");
    vfs_write("hostname", "KeyholeOS V1.2.1\nThis is the hostnamefile\n", 0);
    vfs_cd("~");
    vfs_touch(".ksh_history");
    sudo_mode = 0;
}
int sudo_mode = 0;
struct Node* find_child(struct Node* directory, char* name) {
    struct Node* child = directory->first_child;
    while (child != NULL) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next_sibling;
    }
    return NULL;
}
int is_protected(struct Node* node) {
    struct Node* home = find_child(root, "home");
    if (home == NULL) return 1;
    struct Node* home_root = find_child(home, "root");
    if (home_root == NULL) return 1;
    
    struct Node* curr = node;
    while (curr != root) {
        if (curr == home_root) return 0;
        curr = curr->parent;
    }
    return 1;
}
struct Node* resolve_path(char* path) {
    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0) return current_dir;
    struct Node* node;
    if (path[0] == '~') {
        node = find_child(find_child(root, "home"), "root");
        if (node == NULL) node = root;
        path++;
        if (path[0] == '/') path++;
    } else if (path[0] == '/') {
        node = root;
        path++;
    } else {
        node = current_dir;
    }
    
    char token[256];
    int pos = 0;
    
    for (int i = 0; ; i++) {
        if (path[i] == '/' || path[i] == '\0') {
            token[pos] = '\0';
            if (pos > 0) {
                if (strcmp(token, "..") == 0) {
                    node = node->parent;
                } else if (strcmp(token, ".") != 0) {
                    node = find_child(node, token);
                    if (node == NULL) return NULL;
                }
            }
            pos = 0;
            if (path[i] == '\0') break;
        } else {
            token[pos++] = path[i];
        }
    }
    return node;
}
struct Node* vfs_mkdir(char* path) {
    char parent_path[256];
    char name[256];
    int last_slash = -1;
    
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') last_slash = i;
    }
    
    struct Node* parent;
    if (last_slash == -1) {
        parent = current_dir;
        strcpy(name, path);
    } else if (last_slash == 0) {
        parent = root;
        strcpy(name, path + 1);
    } else {
        strncpy(parent_path, path, last_slash);
        parent_path[last_slash] = '\0';
        strcpy(name, path + last_slash + 1);
        parent = resolve_path(parent_path);
        if (parent == NULL || parent->type != VFS_DIRECTORY) return NULL;
    }
    if (is_protected(parent) && !sudo_mode) return NULL;
    struct Node* dir = (struct Node*)kmalloc(sizeof(struct Node));
    strcpy(dir->name, name);
    dir->type = VFS_DIRECTORY;
    dir->data = NULL;
    dir->data_size = 0;
    dir->parent = parent;
    dir->first_child = NULL;
    dir->next_sibling = NULL;
    
    if (parent->first_child == NULL) {
        parent->first_child = dir;
    } else {
        struct Node* child = parent->first_child;
        while (child->next_sibling != NULL) {
            child = child->next_sibling;
        }
        child->next_sibling = dir;
    }
    return dir;
}

struct Node* vfs_touch(char* path) {
    char parent_path[256];
    char name[256];
    int last_slash = -1;
    
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') last_slash = i;
    }
    
    struct Node* parent;
    if (last_slash == -1) {
        parent = current_dir;
        strcpy(name, path);
    } else if (last_slash == 0) {
        parent = root;
        strcpy(name, path + 1);
    } else {
        strncpy(parent_path, path, last_slash);
        parent_path[last_slash] = '\0';
        strcpy(name, path + last_slash + 1);
        parent = resolve_path(parent_path);
        if (parent == NULL || parent->type != VFS_DIRECTORY) return NULL;
    }
    if (is_protected(parent) && !sudo_mode) {
        return NULL;
    }
    struct Node* file = (struct Node*)kmalloc(sizeof(struct Node));
    strcpy(file->name, name);
    file->type = VFS_FILE;
    file->data = NULL;
    file->data_size = 0;
    file->parent = parent;
    file->first_child = NULL;
    file->next_sibling = NULL;
    
    if (parent->first_child == NULL) {
        parent->first_child = file;
    } else {
        struct Node* child = parent->first_child;
        while (child->next_sibling != NULL) {
            child = child->next_sibling;
        }
        child->next_sibling = file;
    }
    return file;
}

struct Node* vfs_ls(char* path) {
    struct Node* dir;
    if (path == NULL || path[0] == '\0') {
        dir = current_dir;
    } else {
        dir = resolve_path(path);
        if (dir == NULL) return NULL;
        if (dir->type != VFS_DIRECTORY) return NULL;
    }
    return dir->first_child;
}
int vfs_cd(char* name) {
    struct Node* node = resolve_path(name);
    if (node == NULL) return -1;
    if (node->type != VFS_DIRECTORY) return -2;
    current_dir = node;
    return 0;
}
int vfs_cat(char* path, char** out) {
    struct Node* node = resolve_path(path);
    if (node == NULL) return -1;
    if (node->type != VFS_FILE) return -2;
    *out = node->data;
    return 0;
}
int vfs_rm(char* path) {
    char parent_path[256];
    char name[256];
    int last_slash = -1;
    
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') last_slash = i;
    }
    
    struct Node* parent;
    if (last_slash == -1) {
        parent = current_dir;
        strcpy(name, path);
    } else if (last_slash == 0) {
        parent = root;
        strcpy(name, path + 1);
    } else {
        strncpy(parent_path, path, last_slash);
        parent_path[last_slash] = '\0';
        strcpy(name, path + last_slash + 1);
        parent = resolve_path(parent_path);
        if (parent == NULL) return -1;
    }
    
    struct Node* prev = NULL;
    struct Node* child = parent->first_child;
    while (child != NULL) {
        if (strcmp(child->name, name) == 0) {
            if (is_protected(child) && !sudo_mode) return -3;
            if (child->type == VFS_DIRECTORY) {
                return -2;
            }
            // unlink from sibling chain
            if (prev == NULL) {
                parent->first_child = child->next_sibling;
            } else {
                prev->next_sibling = child->next_sibling;
            }
            if (child->data) kfree(child->data);
            kfree(child);
            return 0;
        }
        prev = child;
        child = child->next_sibling;
    }
    return -1;
}
int vfs_rmdir(char* path) {
    struct Node* node = resolve_path(path);
    if (node == NULL) return -1;
    if (node == root) return -4;
    if (node == current_dir) return -5;
    if (node->type != VFS_DIRECTORY) return -2;
    if (node->first_child != NULL) return -3;
    if (is_protected(node) && !sudo_mode) return -4;
    
    struct Node* parent = node->parent;
    struct Node* prev = NULL;
    struct Node* child = parent->first_child;
    
    while (child != NULL) {
        if (child == node) {
            if (prev == NULL) {
                parent->first_child = child->next_sibling;
            } else {
                prev->next_sibling = child->next_sibling;
            }
            kfree(child);
            return 0;
        }
        prev = child;
        child = child->next_sibling;
    }
    return -1;
}
char* vfs_pwd() {
    static char path[4096];
    struct Node* node = current_dir;
    
    if (node == root) {
        strcpy(path, "/");
        return path;
    }
    
    path[0] = '\0';
    int pos = 4095;
    path[pos] = '\0';
    
    while (node != root) {
        int len = strlen(node->name);
        pos -= len;
        if (pos < 1) break;
        kmemcpy(path + pos, node->name, len);
        pos--;
        path[pos] = '/';
        node = node->parent;
    }
    
    return path + pos;
}
int vfs_write(char* path, char* data, int append) {
    struct Node* node = resolve_path(path);
    if (node == NULL) {
        node = vfs_touch(path);
        if (node == NULL) return -1;
    }
    if (node->type != VFS_FILE) return -2;
    
    int new_len = strlen(data);
    if (is_protected(node) && !sudo_mode) return -3;
    if (append && node->data != NULL) {
        int old_len = node->data_size;
        char* new_data = (char*)kmalloc(old_len + new_len + 1);
        kmemcpy(new_data, node->data, old_len);
        kmemcpy(new_data + old_len, data, new_len);
        new_data[old_len + new_len] = '\0';
        kfree(node->data);
        node->data = new_data;
        node->data_size = old_len + new_len;
    } else {
        if (node->data) kfree(node->data);
        node->data = (char*)kmalloc(new_len + 1);
        kmemcpy(node->data, data, new_len);
        node->data[new_len] = '\0';
        node->data_size = new_len;
    }
    return 0;
}
int vfs_rm_recursive(struct Node* node) {
    struct Node* child = node->first_child;
    while (child != NULL) {
        if (is_protected(child) && !sudo_mode) return -3;
        struct Node* next = child->next_sibling;
        if (child->type == VFS_DIRECTORY) {
            vfs_rm_recursive(child);
        } else {
            if (child->data) kfree(child->data);
            kfree(child);
        }
        child = next;
    }
    if (is_protected(node) && !sudo_mode) return -3;
    kfree(node);
}
int super_sudo_mode = 0;
int vfs_rm_r(char* path) {
    struct Node* node = resolve_path(path);
    if (node == NULL) return -1;
    if (node == root && !super_sudo_mode) return -5;
    if (node == current_dir) return -4;
    if (is_protected(node) && !sudo_mode) return -3;
    
    // unlink from parent
    struct Node* parent = node->parent;
    struct Node* prev = NULL;
    struct Node* child = parent->first_child;
    if (node == root && super_sudo_mode) {
        vfs_cd("/");
        struct Node* child = root->first_child;
        while (child != NULL) {
            struct Node* next = child->next_sibling;
            vfs_rm_recursive(child);
            child = next;
        }
        root->first_child = NULL;
        return 0;
    }
    while (child != NULL) {
        if (child == node) {
            struct Node* check = current_dir;
            while (check != root) {
                if (check == node) {
                    current_dir = root;
                    break;
                }
                check = check->parent;
            }
            if (prev == NULL) {
                parent->first_child = child->next_sibling;
            } else {
                prev->next_sibling = child->next_sibling;
            }
            vfs_rm_recursive(child);
            return 0;
        }
        prev = child;
        child = child->next_sibling;
    }
    return -1;
}
int vfs_mv(char* src, char* dest) {
    struct Node* node = resolve_path(src);
    if (node == NULL) return -1;
    if (node == root) return -3;
    if (is_protected(node) && !sudo_mode) return -3;
    struct Node* dest_node = resolve_path(dest);
    if (dest_node != NULL && dest_node->type == VFS_DIRECTORY) {
        if (is_protected(dest_node) && !sudo_mode) return -4;
    }
    struct Node* parent = node->parent;
    struct Node* prev = NULL;
    struct Node* child = parent->first_child;
    while (child != NULL) {
        if (child == node) {
            if (prev == NULL) parent->first_child = child->next_sibling;
            else prev->next_sibling = child->next_sibling;
            break;
        }
        prev = child;
        child = child->next_sibling;
    }
    
    if (dest_node != NULL && dest_node->type == VFS_DIRECTORY) {
        node->parent = dest_node;
        node->next_sibling = dest_node->first_child;
        dest_node->first_child = node;
    } else {
        strcpy(node->name, dest);
        node->next_sibling = parent->first_child;
        parent->first_child = node;
    }
    return 0;
}

int vfs_cp(char* src, char* dest) {
    struct Node* node = resolve_path(src);
    if (node == NULL) return -1;
    if (node->type != VFS_FILE) return -2;
    struct Node* dest_node = resolve_path(dest);
    char full_dest[512];
    if (dest_node != NULL && dest_node->type == VFS_DIRECTORY) {
        strcpy(full_dest, dest);
        strcat(full_dest, "/");
        strcat(full_dest, node->name);
        dest = full_dest;
    }
    
    struct Node* copy = vfs_touch(dest);
    if (copy == NULL) return -3;
    
    if (node->data != NULL) {
        copy->data = (char*)kmalloc(node->data_size + 1);
        kmemcpy(copy->data, node->data, node->data_size);
        copy->data[node->data_size] = '\0';
        copy->data_size = node->data_size;
    }
    return 0;
}

int vfs_cp_r(char* src, char* dest) {
    struct Node* node = resolve_path(src);
    if (node == NULL) return -1;
    
    if (node->type == VFS_FILE) {
        return vfs_cp(src, dest);
    }
    struct Node* dest_node = resolve_path(dest);
    char full_dest[512];
    if (dest_node != NULL && dest_node->type == VFS_DIRECTORY) {
        strcpy(full_dest, dest);
        strcat(full_dest, "/");
        strcat(full_dest, node->name);
        dest = full_dest;
    }
    
    struct Node* new_dir = vfs_mkdir(dest);
    if (new_dir == NULL) return -3;
    
    struct Node* child = node->first_child;
    while (child != NULL) {
        char src_path[512];
        char dest_path[512];
        strcpy(src_path, src);
        strcat(src_path, "/");
        strcat(src_path, child->name);
        strcpy(dest_path, dest);
        strcat(dest_path, "/");
        strcat(dest_path, child->name);
        
        if (child->type == VFS_DIRECTORY) {
            vfs_cp_r(src_path, dest_path);
        } else {
            vfs_cp(src_path, dest_path);
        }
        child = child->next_sibling;
    }
    return 0;
}
void ls_recursive(struct Node* dir, char* path, int show_all) {
    print_str(path);
    print_str(":\n");
    uint8_t saved = color;
    struct Node* child = dir->first_child;
    while (child != NULL) {
        if (child->name[0] != '.' || show_all) {
            if (child->type == VFS_DIRECTORY) {
                print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_BLACK);
            }
            print_str(child->name);
            color = saved;
            print_char(' ');
        }
        child = child->next_sibling;
    }
    print_str("\n\n");
    child = dir->first_child;
    while (child != NULL) {
        if (child->type == VFS_DIRECTORY && (child->name[0] != '.' || show_all)) {
            char subpath[512];
            strcpy(subpath, path);
            strcat(subpath, "/");
            strcat(subpath, child->name);
            ls_recursive(child, subpath, show_all);
        }
        child = child->next_sibling;
    }
}