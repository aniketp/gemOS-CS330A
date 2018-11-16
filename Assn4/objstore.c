/*
 * AUTHOR : Aniket Pandey <aniketp@iitk.ac.in>	(160113)
 */

#include "lib.h"
#include <stdbool.h>

// #define CACHE			// This need not be placed here

typedef unsigned int u32;


#if __x84_64__
typedef unsigned long u64;
typedef long s64
#else
typedef unsigned long long u64;
typedef long long s64;
#endif

// void insert_to_disk(struct objfs_state *objfs, struct object *obj);

#define INODE_INIT		0xfffffffc
#define BLOCK_INIT		0xffffffff
#define BLOCKMAP_SHIFT	(1 << 7) + (1 << 2) + (1 << 5) + (1 << 11)

#define MAX_OBJECT				(1 << 20)
#define MAX_BLOCKS 				(1 << 23)
#define MAX_INODES				(1 << 20)
#define MAX_CACHES				(1 << 15)
// #define STRUCT_BLOCK			(1 << 12)

#define B_BLOCKS 				(1 << 8)
#define I_BLOCKS				(1 << 5)
#define ID_BLOCKS				(1 << 10)
// #define C_BLOCKS 				1 << 5

#define CACHE_HTSIZE 			(1 << 15)				// CACHE_HASH_TABLE_SIZE
#define OBJECTS_PER_BLOCK 		(1 << 6)


#define CACHE

#define malloc_custom(x, y)                                                                             \
    do                                                                                               \
    {                                                                                                \
        (x) = mmap(NULL, (y) * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); \
        if ((x) == MAP_FAILED)                                                                       \
            (x) = NULL;                                                                              \
    } while (0);

#define malloc_4k(x)                                                                             \
    do                                                                                           \
    {                                                                                            \
        (x) = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); \
        if ((x) == MAP_FAILED)                                                                   \
            (x) = NULL;                                                                          \
    } while (0);

#define free_4k(x) munmap((x), BLOCK_SIZE)

#define free_custom(x,y) munmap((x), (y)*BLOCK_SIZE)

// struct objfs_state* objfs;

struct object{
	unsigned int id;
	unsigned int size;
	int cache_index;
	unsigned int blocknum;
	char key[32];

	u32 level1[4];
	// u32 level2[5];	// Don't need more than 4000
};


struct hashobj {
    u64 id;
    struct hashobj *next;
};

// For cache hashmap
struct cache_obj {
	u64 cache_addr;		// c_bl_add
	u32 disk_addr;		// d_bloc
	u32 dirty;

	// Traversal pointers
	struct cache_obj *left;
	struct cache_obj *right;
	struct cache_obj *next;
};

struct object *objs;
struct hashobj *hashTable[MAX_OBJECT] = {NULL};
struct cache_obj *cacheHashTable[CACHE_HTSIZE];

// unsigned int inodemap[MAX_INODES/8];
// unsigned int blockmap[MAX_BLOCKS/8];		// Bitmap instead of Bytemap
// unsigned int cachemap[MAX_OBJECT];

char inodemap[MAX_INODES << 3];
char blockmap[MAX_BLOCKS << 3];		// Bitmap instead of Bytemap
char cachemap[BLOCK_SIZE];

// Bitmap Iterators
u32 cache_bit_itr = 0;
u32 block_bit_itr = 0;
u32 inode_bit_itr = 0;

struct cache_obj *cache_left = NULL;
struct cache_obj *cache_right = NULL;
u32 cache_size = 0;          // number of cache blocks used

void add_at_top_of_cache(struct cache_obj *new_cache_node);
void remove_object_from_cache_and_write_back(struct cache_obj* node, struct objfs_state *objfs);
void remove_object_from_cache_dont_write_back(struct cache_obj* node, struct objfs_state *objfs);
void insert_block_in_cache_hash_table(u32 disk_addr, struct cache_obj* new_cache_node);
void remove_block_from_cache_hash_table(struct cache_obj *cache_right);
bool check_disk_block_in_cache(u32 disk_addr);
struct cache_obj* retrieve_block_from_cache(u32 disk_addr, struct objfs_state *objfs);
struct cache_obj *get_cache_obj(u32 disk_addr);
struct cache_obj* insert_block_to_cache(u32 disk_addr, u64 cache_addr, struct objfs_state *objfs);


// char cachemap[1*BLOCK_SIZE];       //   MAX_CACHE_BLOCKS/2^3



u32 get_block_for_cache() {
    u32 free_id = 0;
	u32 index;
    for (u32 i = cache_bit_itr; i < ((u32)(1 << 12) + cache_bit_itr); i++) {
        for (int j = 0; j < 8; j++) {
            // BLOCK_SIZE = 1 << 12
			index = i % BLOCK_SIZE;
            if(!index && ( j == 0 || j == 1 ))
				continue;

            u32 result = cachemap[index] & (u32)(1 << j);
            if(result == 0) {
                free_id = ((index) * 8) + j;
                cachemap[index] |= (u32)(1 << j);
                cache_bit_itr = index;
                return free_id;
            }
        }
    }
    return 0;
}


u32 fetch_free_id() {
    u32 free_id = 0;
    for (u32 i = inode_bit_itr; i < (MAX_OBJECT/8) + inode_bit_itr; i++) {
		u32 index = i % (MAX_OBJECT/8);
        for (int j = 0; j < 8; j++) {
            if(!index && ( j == 0 || j == 1 ) )
				continue;
            // u32 mask = 1 << j;
            u32 result = inodemap[index] & (u32)(1 << j);
            if(result == 0 ){
                free_id = (index * 8) + j;
                inodemap[index] |= (u32)(1 << j);
                inode_bit_itr = index;
                return free_id;
            }
        }
    }
    return 0;
}


u32 get_new_block_no() {
    u32 free_id = 0;
    for (u32 i = block_bit_itr; i < (MAX_BLOCKS/8) + block_bit_itr; i++) {
		u32 index = i % (MAX_BLOCKS/8);
        for (int j = 0; j < 8; j++) {
            if(!index && ( j == 0 || j == 1 ) )
				continue;

            u32 result = blockmap[index] & (u32)(1 << j);
            if(result == 0 ){
                free_id = (index * 8) + j;
                blockmap[index] |= (u32)(1 << j);
                block_bit_itr = index;
                return free_id;
            }
        }
    }
    return 0;
}


/*
 *		TODO: Implement shortcuts
 */
void free_cache_id(u32 cache_id) {
	u32 mask = ((1 << (cache_id % 8)) ^ 0xff);
    cachemap[cache_id >> 3] = cachemap[cache_id >> 3] & mask;
    return;
}

// TO DO : implement lock
void free_inode_id(u32 inode_id) {
    u32 mask = ((1 << (inode_id % 8)) ^ 0xff);
    inodemap[inode_id >> 3] = inodemap[inode_id >> 3] & mask;
    return;
}

// TO DO : implement lock
void free_block(u32 block_no) {
    u32 mask = ((1 << (block_no % 8)) ^ 0xff);
    blockmap[block_no >> 3] = blockmap[block_no >> 3] & mask;
    return;
}

u32 cache_hash(u32 disk_block) {
    return (disk_block % CACHE_HTSIZE);
}

// returns hash for a string particularly key of file
u32 hash(const char *key) {
	u32 hashval = 0;
	for (int i = 0; i < 29 && key[i]; i++)
		hashval = ((hashval * 39) + key[i]) % (CACHE_HTSIZE - 1);
	// if (!hashval)
	// 	return 1;
	return (hashval % MAX_OBJECT) + 1;
}

// inserts id in HashTable TODO: Check the order
void insert_to_hash_table(u32 id, u32 index) {
    struct hashobj* itr = hashTable[index];
    struct hashobj* par = itr;
    while (itr != NULL) {
        par = itr;
        itr = itr->next;
    }
    if (par == hashTable[index]) {
        hashTable[index] = (struct hashobj *)calloc(sizeof(struct hashobj), 1);
        hashTable[index]->id = id;
        return;
    }

    par->next = (struct hashobj *)calloc(sizeof(struct hashobj), 1);
    par->next->id = id;
    return;

}



#ifdef CACHE
u32 read_block_modified(struct objfs_state *objfs, u32 block_no, char *buf)
{
    dprintf("in %s 1\n", __func__);
    dprintf("request b number is : %d\n", block_no);
    struct cache_obj* new_node = retrieve_block_from_cache(block_no, objfs);
    if(new_node == NULL)
    {
        void *ptr;
        malloc_4k(ptr);
        if(!ptr)
            return -1;
        u32 retval = read_block(objfs, block_no , ptr);
        if (retval < 0) return retval;
        memcpy(buf, ptr, BLOCK_SIZE);
        u32 cache_id = get_block_for_cache(objfs);

        memcpy( (char*)(objfs->cache) + (cache_id*BLOCK_SIZE), buf, BLOCK_SIZE );
        struct cache_obj* new_node = insert_block_to_cache(block_no, cache_id, objfs);

        free_4k(ptr) ;
        return retval;
    }

    memcpy(buf,objfs->cache + ((new_node->cache_addr)*BLOCK_SIZE), BLOCK_SIZE);
    return BLOCK_SIZE;
}

long write_block_modified(struct objfs_state *objfs, u32 block_no, char *buf)
{
    struct cache_obj* new_node = retrieve_block_from_cache(block_no, objfs);
    if(new_node == NULL) {
        u32 cache_id = get_block_for_cache();
        memcpy( (char*)(objfs->cache) + (cache_id * BLOCK_SIZE), buf, BLOCK_SIZE );

		// Due to local scope, the two lines of code below need  to be repeated
        struct cache_obj* new_node = insert_block_to_cache(block_no, cache_id, objfs);
        new_node->dirty = 1;
        return 1;
    }

    memcpy((objfs->cache) + ((new_node->cache_addr) * BLOCK_SIZE), (void *)buf, BLOCK_SIZE);
    new_node->dirty = 1;
    return 1;
}

#else
u32 read_block_modified(struct objfs_state *objfs, u32 block_no, char *buf) {
    struct cache_obj *new_node = retrieve_block_from_cache(block_no, objfs);
    if(new_node == NULL) {
		// 4k align shit
		void *ptr; malloc_4k(ptr);
		if(!ptr) return -1;

        if ((obtained_id = read_block(objfs, block_no, (char *)ptr)) < 0);
        	return -1;
		memcpy(buf, ptr, BLOCK_SIZE);

		// u32 c_id = get_block_for_cache();
		// memcpy((char *)(objfs->cache) + (c_id * BLOCK_SIZE), buf, BLOCK_SIZE);
        insert_block_to_cache(block_no, (u64)buf, objfs);
		free(ptr);
        return retval;
    }
    memcpy(buf, (char *)(new_node->cache_addr), BLOCK_SIZE);
    return BLOCK_SIZE;
}

u32 write_block_modified(struct objfs_state *objfs, u32 block_no, char *buf) {
    struct cache_obj* new_node = retrieve_block_from_cache(block_no, objfs);
    if(new_node == NULL)
        struct cache_obj* new_node = insert_block_to_cache(block_no, (u64)buf, objfs);

    memcpy(new_node->cache_addr, buf, BLOCK_SIZE);
    new_node->dirty = 1;
    return 1;
}


#endif



struct cache_obj*
retrieve_block_from_cache(u32 disk_block, struct objfs_state *objfs) {
	struct cache_obj *node, *c_node;
    if (check_disk_block_in_cache(disk_block)) {
		u32 val = cache_hash(disk_block);
		node = cacheHashTable[val];

		while (node != NULL && node->disk_addr != disk_block)
			node = node->next;

		c_node = node;


        struct cache_obj *new_node = (struct cache_obj *)(calloc(sizeof(struct cache_obj),1));
		new_node->left = NULL;
		new_node->right = NULL;
		new_node->disk_addr = disk_block;
		new_node->cache_addr = c_node->cache_addr;
        // struct cache_obj *newnode = (struct cache_obj *)(calloc(sizeof(struct cache_obj), 1));
        // *newnode = *node;		// newnode to be added back to cache

        // Remove "node" from the cache and add new node
        remove_object_from_cache_dont_write_back(c_node, objfs);
        add_at_top_of_cache(new_node);
		remove_block_from_cache_hash_table(new_node);
		insert_block_in_cache_hash_table(new_node->disk_addr, new_node);
        return new_node;
    }
    return NULL;
}

struct cache_obj *
insert_block_to_cache(u32 disk_block, u64 cache_block, struct objfs_state *objfs) {
	if (check_disk_block_in_cache(disk_block))
        return NULL;

	struct cache_obj *new_cache_node = (struct cache_obj *)(calloc(sizeof(struct cache_obj), 1));
	new_cache_node->disk_addr = disk_block;
	new_cache_node->cache_addr = cache_block;

	// Initialize cache hash pointers
	new_cache_node->left = NULL;
	new_cache_node->right = NULL;

    if (cache_size >= CACHE_HTSIZE - 1) {
        remove_block_from_cache_hash_table(cache_right);
        free_cache_id(cache_right->cache_addr);
        remove_object_from_cache_and_write_back(cache_right, objfs);
    }

	//  Add at the supposed top ()
	add_at_top_of_cache(new_cache_node);

    insert_block_in_cache_hash_table(disk_block, new_cache_node);
    return new_cache_node;
}

void add_at_top_of_cache(struct cache_obj *new_cache_node) {

    new_cache_node->right = cache_left;
    new_cache_node->left = NULL;

	if (cache_left != NULL)
        cache_left->left = new_cache_node;
    cache_left = new_cache_node;

	if (cache_right == NULL)
        cache_right = cache_left;

	cache_size += 1;
    return;
}

void remove_object_from_cache_and_write_back(struct cache_obj* node,
	struct objfs_state *objfs) {
	// We need to write back to the disk
	if (node->dirty) {
		void *ptr; malloc_4k(ptr);
		if(!ptr) return;

		// Copy cache_block_address to 4k ptr variable
		memcpy(ptr, objfs->cache + (node->cache_addr * BLOCK_SIZE), BLOCK_SIZE);
        if (write_block(objfs, node->disk_addr, (char *)ptr) < 0)
            dprintf("Error in writing to disk: %d from cache: %d", node->disk_addr, node->cache_addr);
		free_4k(ptr);
	}

	// Take out the node from cache (Assuming the method works correctly)
    if (node->left != NULL)
        node->left->right = node->right;
    else
        cache_left = node->right;

    if (node->right != NULL)
        node->right->left = node->left;
    else
        cache_right = node->left;

	// Here, we don't have to free cache ID
    // free_cache_id(node->cache_addr);
    free(node);
    cache_size -= 1;
    return;
}


void remove_object_from_cache_dont_write_back(struct cache_obj* node,
	struct objfs_state *objfs) {

	// Take out the node from cache (Assuming the method works correctly)
    if (node->left != NULL)
        node->left->right = node->right;
    else
        cache_left = node->right;

    if (node->right != NULL)
        node->right->left = node->left;
    else
        cache_right = node->left;

	// Here, we don't have to free cache ID
    // free_cache_id(node->cache_addr);
    free(node);
    cache_size -= 1;
    return;
}

void insert_block_in_cache_hash_table(u32 disk_block, struct cache_obj* new_cache_node) {
	u32 val = cache_hash(disk_block);
    struct cache_obj* itr = cacheHashTable[val];
    struct cache_obj* parent = itr;
	// This is for insertion at the end
	while (itr != NULL) {
        parent = itr;
        itr = itr->next;
    }

	// Empty hashtable index
	if (parent == NULL) {
		// This can be done in an easier way
        // parent = (struct cache_obj *)calloc(sizeof(struct cache_obj),1);
        // *parent = *node;
		cacheHashTable[val] = new_cache_node;
        // free(node);
        return;
    }
    parent->next = new_cache_node;
    return;
}

struct cache_obj*		// get_c_node
get_cache_obj(u32 disk_addr) {
	u32 val = cache_hash(disk_addr);
	struct cache_obj* itr = cacheHashTable[val];

	while (itr != NULL) {
		if (itr->disk_addr == disk_addr)
			return itr;
		itr = itr->next;
	}

	// Weirdly, the object didn't exist
	return NULL;
}

// Delete the 'node' from cache hash table	// <-- remove_from_cache_hash_table
void remove_block_from_cache_hash_table(struct cache_obj *node) {
    u32 val = cache_hash(node->disk_addr);
    struct cache_obj* itr = cacheHashTable[val];
	struct cache_obj* par = itr;

	while ((itr != NULL) && (itr->disk_addr != node->disk_addr)) {
        par = itr;
        itr = itr->next;
    }

	// TODO: Uncomment this later on  ############################
	// if (itr == NULL)
	// 	return;		// Block not found

	// Single block at that location
    if (par == itr) {
        cacheHashTable[val] = NULL;
        return;
    }
    par->next = itr->next;
    return;
}

//////////////////// CHECKED
// Check if the disk block is cached
// NOTE: Can be converted to Bool EDIT: DONE <- // cache_key_present_hash_table
bool check_disk_block_in_cache(u32 disk_block) {
    u32 val = cache_hash(disk_block);
    struct cache_obj* itr = cacheHashTable[val];

	while (itr != NULL) {
        if(itr->disk_addr == disk_block)
			return true;
        itr = itr->next;
    }
    return false;
}


struct object *get_object_by_id(u64 objid, struct objfs_state *objfs) {
    u32 block_no = ((objid - 2) / OBJECTS_PER_BLOCK) + (B_BLOCKS) + (I_BLOCKS) + (ID_BLOCKS);
    u32 block_off = ((objid - 2) % OBJECTS_PER_BLOCK) * sizeof(struct object);

    struct object buf[BLOCK_SIZE/sizeof(struct object)];
    if(read_block_modified(objfs, block_no, ((char *)buf)) < 0)
        return NULL;

    struct object* node = (struct object *)calloc(sizeof(struct object), 1);

    *node = buf[block_off/sizeof(struct object)];
    if (node->id == objid)
        return node;
	else
    	free(node);
    return NULL;
}
//


void insert_to_disk(struct object *obj, struct objfs_state *objfs) {
	// NOTE: What if I pass actually object instead of a pointer? It kinda works
    u32 block_no = (B_BLOCKS) + (I_BLOCKS) + (ID_BLOCKS) + ((obj->id - 2) / OBJECTS_PER_BLOCK);

    struct object buf[BLOCK_SIZE/sizeof(struct object)];
    if(read_block_modified(objfs, block_no, (char *)buf) < 0)
        return;

    buf[((obj->id - 2) % OBJECTS_PER_BLOCK)] = *obj;
	// Check does not matter here
	write_block_modified(objfs, block_no, (char *)buf);

    return;
}

void clear_hash_table(struct hashobj** hashTable) {

	// Essence: Cleanup the entire hash table
	struct hashobj *itr1, *itr2;
	for(int i = 0; i < MAX_OBJECT; i++) {
        itr1 = hashTable[i];		// TODO: Change the names
        while (itr1 != NULL) {
            itr2 = itr1;
            itr1 = itr1->next;
            free(itr2);
        }
    }
}


void free_object_blocks(u32 ptr, struct objfs_state* objfs) {
    u32 blocks[1024] = {0};
    if(read_block_modified(objfs, ptr, blocks) < 0)
        dprintf("problem in loading indirect block in remove object cached\n");

    for( int i = 0; i < 1024; i++)
        if(!blocks[i]) free_block(blocks[i]);
}


#ifdef CACHE // CACHED implementation

static void init_object_cached(struct object *obj)
{

    return;
}

void remove_object_cached (struct object *obj, struct objfs_state* objfs) {
	// TODO: Modify this (Infact, this is completely wrong)
	// TODO: Convert the numbers to macros
    free_inode_id(obj->id);

	if (obj->blocknum > 3072) {
		free_object_blocks(objfs, obj->level1[0]);
		free_block(obj->level1[0]);
		free_object_blocks(objfs, obj->level1[1]);
		free_block(obj->level1[1]);
		free_object_blocks(objfs, obj->level1[2]);
		free_block(obj->level1[2]);
		free_object_blocks(objfs, obj->level1[3]);
		free_block(obj->level1[3]);
	}

	else if (obj->blocknum > 2048) {
		free_object_blocks(objfs, obj->level1[0]);
		free_block(obj->level1[0]);
		free_object_blocks(objfs, obj->level1[1]);
		free_block(obj->level1[1]);
		free_object_blocks(objfs, obj->level1[2]);
		free_block(obj->level1[2]);
	}

	else if (obj->blocknum > 1024) {
		free_object_blocks(objfs, obj->level1[0]);
		free_block(obj->level1[0]);
		free_object_blocks(objfs, obj->level1[1]);
		free_block(obj->level1[1]);
	}

	else if (obj->blocknum) {
		free_object_blocks(objfs, obj->level1[0]);
		free_block(obj->level1[0]);
	}

    return;
}

static int find_read_cached(struct objfs_state *objfs, struct object *obj, char *user_buf, int size, u32 offset)
{
	u32 block_offset = 0;
	u32 level1_ptr[1024] = {0};
	char ans[BLOCK_SIZE] = {0};

	if(obj->blocknum < (offset/BLOCK_SIZE))
		return -1;

    if ((offset/BLOCK_SIZE) < 1024) {

        if(read_block_modified(objfs, obj->level1[0], (char *)level1_ptr) < 0)
            dprintf("Error in find read cahe while loading indirect page pointer\n");
        block_offset = level1_ptr[(offset/BLOCK_SIZE)%1024];
    }

    else if ((offset/BLOCK_SIZE) < 2048) {

        if(read_block_modified(objfs, obj->level1[1], (char *)level1_ptr) < 0)
            dprintf("Error in find read cahe while loading indirect page pointer\n");
        block_offset = level1_ptr[(offset/BLOCK_SIZE) % 1024];
    }

    else if ((offset/BLOCK_SIZE) < 3072) {

        if(read_block_modified(objfs, obj->level1[2], (char *)level1_ptr) < 0)
            dprintf("Error in find read cahe while loading indirect page pointer\n");
        block_offset = level1_ptr[(offset/BLOCK_SIZE) % 1024];
    }

    else {
        if(read_block_modified(objfs, obj->level1[3], (char *)level1_ptr) < 0)
            dprintf("Error in find read cahe while loading indirect page pointer\n");
        block_offset = level1_ptr[(offset/BLOCK_SIZE) % 1024];
    }

    if ( read_block_modified(objfs, block_offset, ans ) < 0)
        dprintf("Error in find read cahe while loading disk block\n");

    memcpy(user_buf, ans, size);
    return size;
}


/*Find the object in the cache and update it*/
static int find_write_cached(struct objfs_state *objfs, struct object *obj, const char *user_buf, int size, u32 offset)
{
	if (size < 0) return -1;

    if (!obj->blocknum) {
        u32 new_block = get_new_block_no();
        if (new_block == 0){
            dprintf("disk is full, i am in find write cached \n");
            return -1;
        }
        obj->level1[0] = new_block;
    }


    u32 level1_ptr[1024] = {0};

    if ((offset/BLOCK_SIZE) < 1024)
        read_block_modified(objfs, obj->level1[0], (char *)level1_ptr);

    else if ((offset/BLOCK_SIZE) < 2048) {
        if ((offset/BLOCK_SIZE) == 1024 && obj->blocknum == 1023) {
            u32 new_block = get_new_block_no();
            if (new_block == 0)
                return -1;
            obj->level1[1] = new_block;
        }

        read_block_modified(objfs, obj->level1[1], (char *)level1_ptr);
    }

    else if ((offset/BLOCK_SIZE) < 3072) {
        if ((offset/BLOCK_SIZE) == 2048 && obj->blocknum == 2047) {
            u32 new_block = get_new_block_no();
            if (!new_block) return -1;
            obj->level1[2] = new_block;
        }

        read_block_modified(objfs, obj->level1[2], (char *)level1_ptr);
    }

    else {
        if ((offset/BLOCK_SIZE) == 3072 && obj->blocknum == 3071) {
            u32 new_block = get_new_block_no();
            if (!new_block) return -1;
            obj->level1[3] = new_block;
        }

        read_block_modified(objfs, obj->level1[3], (char *)level1_ptr);
    }

    if( (offset/BLOCK_SIZE) < obj->blocknum) {
        u32 block_on_disk = level1_ptr[(offset/BLOCK_SIZE) % 1024];
        char write_buf[BLOCK_SIZE] = {0};
        memcpy( write_buf, user_buf, size );

		if (write_block_modified(objfs, block_on_disk, write_buf))
            return -1;
        return size;
    }

    else {
        u32 new_block_id = get_new_block_no();
        level1_ptr[(obj->blocknum) % 1024] = new_block_id;
        if(write_block_modified(objfs, obj->level1[0], (char *)level1_ptr) < 0)
			return -1;
        obj->blocknum++;
        char write_buf[BLOCK_SIZE] = {0};
        memcpy( write_buf, user_buf, size );
        if (write_block_modified(objfs, new_block_id, write_buf) < 0)
            return -1;

		//TODO: Check against blocknum again and update the size accordingly


		// TODO; IMPORTANT """""" FIx variable naming and the general syntax
        if ((offset/BLOCK_SIZE) < obj->blocknum)
            obj->size = size;
        else if((offset/BLOCK_SIZE) == obj->blocknum)
            obj->size = size + (((obj->size)/BLOCK_SIZE) * BLOCK_SIZE);
        else
            obj->size += size;

        obj->blocknum++;

        insert_to_disk(obj, objfs);
        return size;
    }

}

/*Sync the cached block to the disk if it is dirty*/
void obj_sync(struct objfs_state *objfs) {
	struct cache_obj *itr;
    for( int i = 0; i < CACHE_HTSIZE; i++) {
        itr = cacheHashTable[i];
        while (itr != NULL) {
            if(itr->dirty) {
                void *ptr; malloc_4k(ptr);
                if(!ptr) return -1;

                memcpy(ptr, objfs->cache + (itr->cache_addr * BLOCK_SIZE),  BLOCK_SIZE);
                if (write_block(objfs, itr->disk_addr, (char *)ptr) < 0)
                    dprintf("Error in writing disk_addr: %d from c_block: %d", itr->disk_addr, itr->cache_addr);
                free_4k(ptr);

            }
            itr = itr->next;
        }
    }
	return;
}


#else //uncached implementation
static void init_object_cached(struct object *obj)
{
    return;
}
static void remove_object_cached(struct object *obj)
{
    return;
}
static int find_read_cached(struct objfs_state *objfs, struct object *obj, char *user_buf, int size)
{
    void *ptr;
    malloc_4k(ptr);
    if (!ptr)
        return -1;
    // if (read_block(objfs, obj->id, ptr) < 0)
        return -1;
    memcpy(user_buf, ptr, size);
    free_4k(ptr);
    return 0;
}
static int find_write_cached(struct objfs_state *objfs, struct object *obj, const char *user_buf, int size)
{
    void *ptr;
    malloc_4k(ptr);
    if (!ptr)
        return -1;
    memcpy(ptr, user_buf, size);
    if (write_block(objfs, obj->id, ptr) < 0)
        return -1;
    free_4k(ptr);
    return 0;
}
static int obj_sync(struct objfs_state *objfs, struct object *obj)
{
    return 0;
}
#endif

/*
Returns the object ID.  -1 (invalid), 0, 1 - reserved
*/
long find_object_id(const char *key, struct objfs_state *objfs)
{
	u32 val = hash(key);
    struct hashobj *itr = hashTable[val];

    while (itr != NULL) {
        struct object *obj = get_object_by_id(itr->id, objfs);
        if (obj == NULL) return -1;
        if (itr->id && (strcmp(obj->key, key) == 0)) {
            free(obj); return itr->id;
        }
        itr = itr->next;
    }
    return -1;
}

/*
  Creates a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.

  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/
long create_object(const char *key, struct objfs_state *objfs) {
    u32 val = hash(key);
    struct hashobj *itr = hashTable[val];
    struct hashobj *par = itr;

    // checking for duplicates
    while (itr != NULL) {
        struct object *obj = get_object_by_id(itr->id, objfs);
        if (obj == NULL) return -1;
        if (itr->id && !strcmp(obj->key, key)) {
            free(obj);
            return -1;
        }
        par = itr;
        itr = itr->next;
    }

	u32 new_id = fetch_free_id();
	if (!new_id) {
               dprintf("%s: objstore full: No ID found\n", __func__);
               return -1;
    }

	struct hashobj *newnode = (struct hashobj *)calloc(sizeof(struct hashobj), 1);
	struct object *newobj = (struct object *)calloc(sizeof(struct object), 1);
	newnode->id = new_id;
	newnode->next = NULL;

	strcpy(newobj->key, key);
	newobj->id = new_id;
	newobj->cache_index = -1;

	if (par == NULL)
		hashTable[val] = newnode;
	else
		par->next = newnode;

    insert_to_hash_table(newobj->id, hash(newobj->key));
    insert_to_disk(newobj, objfs);
    free(newobj);
    return new_id;
}
/*
  One of the users of the object has dropped a reference
  Can be useful to implement caching.
  Return value: Success --> 0
                Failure --> -1
*/
long release_object(int objid, struct objfs_state *objfs)
{
    return 0;
}

/*
  Destroys an object with obj.key=key. Object ID is ensured to be >=2.

  Return value: Success --> 0
                Failure --> -1
*/
long destroy_object(const char *key, struct objfs_state *objfs) {
    u32 val = hash(key);
    struct hashobj *itr = hashTable[val];

    // Check for object's existence
    while (itr != NULL) {
		struct object *obj = get_object_by_id(itr->id, objfs);
        if (obj == NULL) return -1;
        if (itr->id && (strcmp(obj->key, key) == 0)) {
            remove_object_cached(obj, objfs);
            free(itr); free(obj);
            return 0;
        }
        itr = itr->next;
		free(obj);
    }
    return -1;
}

/*
  Renames a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.
  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/

long rename_object(const char *key, const char *newname, struct objfs_state *objfs)
{

	return -1;
}

/*
  Writes the content of the buffer into the object with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/
long objstore_write(int objid, const char *buf, int size,
				struct objfs_state *objfs, off_t offset)
{
	struct object *obj = get_object_by_id(objid, objfs);
	if (obj == NULL || objid < 2)
		return -1;

	dprintf("Doing write size = %d\n", size);
    if (find_write_cached(objfs, obj, buf, size, offset) < 0)
        return -1;
    obj->size = size;
    return size;
}

/*
  Reads the content of the object onto the buffer with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/
long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs, off_t offset)
{
	struct object *obj = get_object_by_id(objid, objfs);
	if (obj == NULL || objid < 2)
		return -1;

	dprintf("Doing read size = %d\n", size);
    if (find_read_cached(objfs, obj, buf, size, offset) < 0)
        return -1;
    return size;
}

/*
  Reads the object metadata for obj->id = objid.
  Fillup buf->st_size and buf->st_blocks correctly
  See man 2 stat
*/
int fillup_size_details(struct stat *buf, struct objfs_state *objfs) {
	// NOTE: Picked up from provided example implementation (and modified)
	struct object *obj = get_object_by_id(buf->st_ino, objfs);
	if (buf->st_ino < 2 || obj == NULL)
		return -1;

    buf->st_size = obj->size;
    buf->st_blocks = obj->blocknum;
	// if (((obj->size >> 9) << 9) != obj->size)
	// 	buf->st_blocks++;
    return 0;
}

/*
   Set your private pointeri, anyway you like.
*/
int objstore_init(struct objfs_state *objfs) {
	u32 uid;
    cache_left = NULL;
    cache_right = NULL;

	char *temp; malloc_4k(temp);
	if (!temp) return -1;

    for (int i = 0; i < ID_BLOCKS; i++) {
        if (read_block(objfs, (i) , (char *)temp) < 0)
            return -1;

		for (u32 x = 0; x < BLOCK_SIZE / sizeof(u32); x++) {
            uid = (temp)[x];
            if (uid)
				insert_to_hash_table((i * BLOCK_SIZE/sizeof(u32)) + x, uid);
        }
    }

    for (int i = 0; i < I_BLOCKS; i++) {
        if (read_block(objfs, ((ID_BLOCKS) + i), temp) < 0)
            return -1;
        memcpy(((char *)inodemap) + (i * BLOCK_SIZE), temp, BLOCK_SIZE);
    }


    for (int i = 0; i < B_BLOCKS; i++) {
        if (read_block(objfs, ((ID_BLOCKS) + (I_BLOCKS) + i), temp) < 0)
            return -1;
        memcpy(((char *)blockmap) + (i * BLOCK_SIZE), temp, BLOCK_SIZE);
    }
	free_4k(temp);

    for(int i = 0; i < BLOCKMAP_SHIFT; i++)
        blockmap[i] = BLOCK_INIT;

	inodemap[0] &= (INODE_INIT);
    objfs->objstore_data = hashTable;
    dprintf("Done objstore init\n");
    return 0;
}

/*
   Cleanup private data. FS is being unmounted
*/
int objstore_destroy(struct objfs_state *objfs)
{
	unsigned long temp[MAX_OBJECT];
	struct hashobj *node;
	for (int i = 0; i < MAX_OBJECT; i++)
		temp[i] = 0;		// Same as buf[]


    for (int i = 0; i < MAX_OBJECT; i++) {
        node = hashTable[i];
        while (node != NULL) {
            temp[node->id] = i;
            node = node->next;
        }
    }


	// NOTE: This to be used in the later loops
	char *temp2; malloc_4k(temp2);
	if (!temp2) return -1;

    for (u32 i = 0; i < ID_BLOCKS; i++) {
        memcpy(temp2, ((char *)temp) + (i * BLOCK_SIZE),  BLOCK_SIZE);
        if (write_block(objfs, i, temp2) < 0)
            return -1;
    }

    for (u32 i = 0; i < I_BLOCKS; i++) {
		memcpy(temp2, ((char *)inodemap) + (i * BLOCK_SIZE),  BLOCK_SIZE);
        if (write_block(objfs, (ID_BLOCKS + i), temp2) < 0)
            return -1;
    }

    for (u32 i = 0; i < B_BLOCKS; i++) {
        memcpy(temp2, ((char *)blockmap) + i * BLOCK_SIZE, BLOCK_SIZE);
        if (write_block(objfs, (ID_BLOCKS + I_BLOCKS + i), temp2) < 0)
            return -1;
    }
	free_4k(temp2);

    obj_sync(objfs);
    clear_hash_table(hashTable);
    free_custom(inodemap, I_BLOCKS);
    free_custom(blockmap, B_BLOCKS);
    objfs->objstore_data = NULL;
    dprintf("Done objstore destroy\n");
    return 0;
}
