#include "lib.h"

typedef unsigned int u32;
typedef 	 int s32;
typedef unsigned short u16;
typedef int      short s16;
typedef unsigned char u8;
typedef char	      s8;

#if __x84_64__
typedef unsigned long u64;
typedef long s64
#else
typedef unsigned long long u64;
typedef long long s64;
#endif

#define MAX_OBJECT				1 << 20
#define MAX_BLOCKS 				1 << 23
#define MAX_INODES				1 << 20
#define MAX_CACHES				1 << 15
// #define STRUCT_BLOCK			(1 << 12)

#define B_BLOCKS 				1 << 8
#define I_BLOCKS				1 << 5
#define ID_BLOCKS				1 << 10
// #define C_BLOCKS 				1 << 5

#define CACHE_HTSIZE 			1 << 15
#define OBJECTS_PER_BLOCK 		1 << 6

// Allocating 4k aligned memory area
#define malloc_4k(x) do{\
                         (x) = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);\
                         if((x) == MAP_FAILED)\
                              (x)=NULL;\
                     }while(0);

// Custom malloc
#define malloc_y(x, size) do{\
                      (x) = mmap(NULL, (size) * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);\
                      if((x) == MAP_FAILED)\
                           (x)=NULL;\
                  }while(0);


#define free_4k(x) munmap((x), BLOCK_SIZE)
#define free_y(x, y) munmap((x), (y) * BLOCK_SIZE)

struct object{
	unsigned int id;
	unsigned int size;
	int cache_index;
	int dirty;
	char key[32];

	u32 level1[4];
	// u32 level2[5];	// Don't need more than 4000
};

struct cache_obj {
	u64 cache_addr;
	u32 disk_addr;
	u32 dirty;

	// Traversal pointers
	struct cache_obj *left;
	struct cache_obj *right;
	struct cache_obj *next;
};

struct hashobj {
	u64 id;
	struct hashobj *next;
};

struct object *objs;
struct hashobj *hashTable[MAX_OBJECT] = {NULL};
struct cache_obj *cacheHashTable[CACHE_HTSIZE];

// unsigned int inodemap[MAX_INODES/8];
// unsigned int blockmap[MAX_BLOCKS/8];		// Bitmap instead of Bytemap
// unsigned int cachemap[MAX_OBJECT];

unsigned int *inodemap;
unsigned int *blockmap;		// Bitmap instead of Bytemap
unsigned int *cachemap;

u64 metadata_store = 0;
u64 prev_meta_store = 0;
struct cache_obj *c_start = NULL;
struct cache_obj *c_end = NULL;
u32 c_size = 0;

// Iterators
u32 cache_bit_itr = 0;
u32 block_bit_itr = 0;
u32 inode_bit_itr = 0;

u32 hash(const char *key) {
	u32 hashval = 0;
	for (int i = 0; i < 29; i++)
		hashval = ((hashval * 39) + key[i]) % MAX_OBJECT;
	if (!hashval)
		return 1;
	return hashval;
}

// update when block is removed from lru queue
u32 get_cache_block() {
    u32 free_id = 0;
    for (u32 i = cache_bit_itr; i < ((1 << 12) + cache_bit_itr); i++) {

        for (int j = 0; j < 8; j++) {
            // BLOCK_SIZE = 1 << 12
            if(i % BLOCK_SIZE == 0 && ( j == 0 || j == 1 ) )
				continue;
            u32 mask = 1 << j;
            u32 result = cachemap[i % BLOCK_SIZE] & mask;
            if(result != 0 ){
                free_id = ((i%BLOCK_SIZE)*8) + j;
                cachemap[i%BLOCK_SIZE] = cachemap[i%BLOCK_SIZE] | mask;
                cache_bit_itr = i % MAX_OBJECT ;
                return free_id;
            }
        }
    }
    return 0;
}

// TO DO : implement lock
u32 fetch_free_id() {
    u32 free_id = 0;
    for (u32 i = inode_bit_itr; i < (MAX_OBJECT/8) + inode_bit_itr; i++) {

        for (int j = 0; j < 8; j++) {
            if(i%(MAX_OBJECT/8) == 0 && ( j == 0 || j == 1 ) ) continue;
            u32 mask = 1 << j;
            u32 result = inodemap[i%(MAX_OBJECT/8)] & mask;
            if(result != 0 ){
                free_id = ((i%(MAX_OBJECT/8))*8) + j;
                inodemap[i%(MAX_OBJECT/8)] = (inodemap[i%(MAX_OBJECT/8)] | mask);
                inode_bit_itr = i % (MAX_OBJECT/8);
                return free_id;
            }
        }
    }
    return 0;
}

// TO DO : implement lock
u32 get_new_block_no()
{
    u32 free_id = 0;
    for (u32 i = block_bit_itr; i < (MAX_BLOCKS/8) + block_bit_itr; i++)
    {

        for (int j = 0; j < 8; j++)
        {
            if(i%(MAX_BLOCKS/8) == 0 && ( j == 0 || j == 1 ) ) continue;
            u32 mask = 1 << j;
            u32 result = blockmap[i % (MAX_BLOCKS/8)] & mask;
            if(result != 0 ){
                free_id = ((i%(MAX_BLOCKS/8))*8) + j;
                blockmap[i%(MAX_BLOCKS/8)] = (blockmap[i%(MAX_BLOCKS/8)] | mask);
                block_bit_itr = i %(MAX_BLOCKS/8);
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
	u32 mask = (1 << (cache_id % 8)) ^ 0xff;
    cachemap[cache_id >> 3] = cachemap[cache_id >> 3] & mask;
    return;
}

// TO DO : implement lock
void free_inode_id(u32 inode_id) {
    u32 mask = (1 << (inode_id % 8)) ^ 0xff;
    inodemap[inode_id >> 3] = inodemap[inode_id >> 3] & mask;
    return;
}

// TO DO : implement lock
void free_block(u32 block_no) {
    u32 mask = (1 << (block_no % 8)) ^ 0xff;
    blockmap[block_no >> 3] = blockmap[block_no >> 3] & mask;
    return;
}

u32 cache_hash(u32 disk_block) {
    return (disk_block % CACHE_HTSIZE);
}

static void remove_object_cached(struct object *obj)
{
    return;
}

static int find_read_cached(struct objfs_state *objfs, u32 block_id, char *user_buf, int size)
{
   void *ptr;
   malloc_4k(ptr);
   if(!ptr)
        return -1;
   if(read_block(objfs, block_id, (char *)ptr) < 0)
       return -1;
   memcpy(user_buf, ptr, size);
   free_4k(ptr);
   return 0;
}

static int find_write_cached(struct objfs_state *objfs, u32 block_id, const char *user_buf, int size)
{
   void *ptr;
   malloc_4k(ptr);
   if(!ptr)
        return -1;
   memcpy(ptr, user_buf, size);
   if(write_block(objfs, block_id, (char *)ptr) < 0)
       return -1;
   free_4k(ptr);
   return 0;
}

////////////////////
// Check if the disk block is cached
// NOTE: Can be converted to Bool
u32 check_disk_block_in_cache(u32 disk_block)
{
    u32 val = cache_hash(disk_block);
    struct cache_obj* itr = cacheHashTable[val];

	while (itr != NULL) {
        if(itr->disk_addr == disk_block)
			return 1;
        itr = itr->next;
    }
    return 0;
}


// Remove object from cache and if dirty, write back to the disk
void remove_object_from_cache_and_write_back(struct objfs_state *objfs, struct cache_obj* node)
{
    if (node->dirty == 1)
        if (write_block(objfs, node->disk_addr, (char *)node->cache_addr) < 0)
            dprintf("Error in writing d_block: %d from c_block: %d", node->disk_addr, node->cache_addr);

    if (node->left != NULL)
        node->left->right = node->right;
    else
        c_start = node->right;

    if (node->right != NULL)
        node->right->left = node->left;
    else
        c_end = node->left;

	free_cache_id(node->cache_addr);
    free(node);
    c_size--;
    return;
}

// Adds the element at the top of the cache
void add_at_top_of_cache(struct cache_obj *new_object) {
	// Top element
    new_object->right = c_start;
    new_object->left = NULL;
    if (c_start != NULL)
        c_start->left = new_object;
    c_start = new_object;
    if (c_end == NULL)
        c_end = c_start;
    c_size++;
    return;
}


struct cache_obj*
get_cache_obj(u32 disk_addr) {
	u32 val = cache_hash(disk_addr);
	struct cache_obj* itr = cacheHashTable[val];

	while (itr != NULL) {
		if (itr->disk_addr == disk_addr)
			return itr;
		itr = itr->next;
	}

	return NULL;
}

// Delete the 'node' from cache hash table
void remove_block_from_cache_hash_table(struct cache_obj *node)
{
    u32 val = cache_hash(node->disk_addr);
    struct cache_obj* itr = cacheHashTable[val];
	struct cache_obj* par = itr;

	while ((itr != NULL) && (itr->disk_addr != node->disk_addr)) {
        par = itr;
        itr = itr->next;
    }

	if (itr == NULL)
		return;		// Block not found

    if (par == itr) {
        cacheHashTable[val] = NULL;
        return;
    }
    par->next = itr->next;
    return;
}

// Inser the node with given block address in the cache
// TODO: Tweak this
void insert_block_in_cache_hash_table(struct cache_obj *node, u32 disk_block) {
    u32 val = cache_hash(disk_block);
    struct cache_obj* temp = cacheHashTable[val];
    struct cache_obj* parent = cacheHashTable[val];
    while (temp != NULL) {
        parent = temp;
        temp = temp->next;
    }

	if(parent == NULL) {
        parent = (struct cache_obj *)calloc(sizeof(struct cache_obj),1);
        *parent = *node;
        free(node);
        return;
    }
    parent->next = node;
    return;
}

struct cache_obj *
retrieve_block_from_cache(struct objfs_state *objfs, u32 disk_block) {
    if (check_disk_block_in_cache(disk_block)) {
        struct cache_obj *node = get_cache_obj(disk_block);
        // struct cache_obj *newnode = (struct cache_obj *)(calloc(sizeof(struct cache_obj), 1));
        // *newnode = *node;		// newnode to be added back to cache

        // Remove "node" from the cache and add new node
        remove_object_from_cache_and_write_back(objfs, node);
        add_at_top_of_cache(node);
        return node;
    }
    return NULL;
}

struct cache_obj *			// This is put_c_block_add
insert_block_to_cache(struct objfs_state *objfs, u32 disk_block, u64 cache_block) {

    if (check_disk_block_in_cache(disk_block)) {
        dprintf("Disk block was already loaded in cache");
        return NULL;
    }
    struct cache_obj *new_cache_node = (struct cache_obj *)(calloc(sizeof(struct cache_obj), 1));
    new_cache_node->left = NULL;
    new_cache_node->right = NULL;

    // Is this notation correct ? I think so
    new_cache_node->disk_addr = disk_block;
    new_cache_node->cache_addr = cache_block;
    if (c_size >= CACHE_HTSIZE) {
        remove_block_from_cache_hash_table(c_end);
        remove_object_from_cache_and_write_back(objfs, c_end);
    }
	// Add the node to cache
    add_at_top_of_cache(new_cache_node);

    insert_block_in_cache_hash_table(new_cache_node, disk_block);
    return new_cache_node;
}

///////////////////////////////////////////////////////////////////////////
#ifdef CACHE
static void init_object_cached(struct object *obj)
{
    obj->cache_index = -1;
    obj->dirty = 0;
    return;
}
static void remove_object_cached(struct object *obj)
{
    obj->cache_index = -1;
    obj->dirty = 0;
    return;
}
static int find_read_cached(struct objfs_state *objfs, struct object *obj, char *user_buf, int size)
{

    // check bitmaps

    char *cache_ptr = objfs->cache + (obj->id << 12);
    if (obj->cache_index < 0)
    { /*Not in cache*/
        if (read_block(objfs, obj->id, cache_ptr) < 0)
            return -1;
        obj->cache_index = obj->id;
    }
    memcpy(user_buf, cache_ptr, size);
    return 0;
}
/*Find the object in the cache and update it*/
static int find_write_cached(struct objfs_state *objfs, struct object *obj, const char *user_buf, int size)
{

    // check bitmaps

    char *cache_ptr = objfs->cache + (obj->id << 12);
    if (obj->cache_index < 0)
    { /*Not in cache*/
        if (read_block(objfs, obj->id, cache_ptr) < 0)
            return -1;
        obj->cache_index = obj->id;
    }
    memcpy(cache_ptr, user_buf, size);
    obj->dirty = 1;
    return 0;
}

/*Sync the cached block to disk if the block was dirty*/
void obj_sync(struct objfs_state *objfs)
{
    // Check the order for retrieval of dirty blocks
	struct cache_obj *itr;
    for(int i = 0; i < CACHE_HTSIZE; i++) {
        itr = cacheHashTable[i];
        while(itr != NULL) {
            if(itr->dirty == 1)
                if (write_block(objfs, itr->disk_addr, (char *)itr->cache_addr) < 0)
                    dprintf("Error in writing d_block: %d from c_block: %d", itr->disk_addr, itr->cache_addr);
        }
    }
	return;		// All dirty blocks saved back
}

#else

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
    if (read_block(objfs, obj->id, ptr) < 0)
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
//////////////////////////////////////////////////////////////////////////////


void clear_hash_table(struct hashobj** hashTable) {

	// Essence: Free up the entire hash table
	struct hashobj *temp1, *temp2;
	for(int i = 0; i < MAX_OBJECT; i++) {
        temp1 = hashTable[i];		// TODO: Change the names
        while(temp1 != NULL) {
            temp2 = temp1;
            temp1 = temp1->next;
            free(temp2);
        }
    }
}


/////////////////////

long read_block_modified(struct objfs_state *objfs, u32 block_no, char *buf) {

    struct cache_obj *new_node = retrieve_block_from_cache(objfs, block_no);
	u32 obtained_id;
	if(new_node == NULL) {
        if ((obtained_id = read_block(objfs, block_no, buf)) < 0);
        	return -1;

#ifdef CACHE
		u32 c_id = get_cache_block();
		memcpy(((char *)objfs->cache) + (c_id * BLOCK_SIZE), buf, BLOCK_SIZE);
#endif

		insert_block_to_cache(objfs, block_no, (u64)buf);
        return obtained_id;
    }
    memcpy(buf, (char *)(new_node->cache_addr), BLOCK_SIZE);
    return BLOCK_SIZE;
}

long write_block_modified(struct objfs_state *objfs, u32 block_no, char *buf) {

    struct cache_obj *new_node = retrieve_block_from_cache(objfs, block_no);
    if(new_node == NULL) {

#ifdef CACHE
		u32 c_id = get_cache_block();
        memcpy(((char *)objfs->cache) + (c_id * BLOCK_SIZE), buf, BLOCK_SIZE);
#endif

        struct cache_obj *next_node = insert_block_to_cache(objfs, block_no, (u64)buf);
        memcpy((char *)next_node->cache_addr, buf, BLOCK_SIZE);
		next_node->dirty = 1;
        return 1;
    }
    memcpy((void *)new_node->cache_addr, buf, BLOCK_SIZE);
    new_node->dirty = 1;
    return 1;
}

struct object *object_fetch(struct objfs_state *objfs, u32 objid) {
    u32 base = (B_BLOCKS) + (I_BLOCKS) + (ID_BLOCKS);

    u32 block_no = ((objid - 2) / OBJECTS_PER_BLOCK) + base;
    u32 block_off = ((objid - 2) % OBJECTS_PER_BLOCK) * sizeof(struct object);

    struct object buf[BLOCK_SIZE/sizeof(struct object)];
    if(read_block_modified(objfs, block_no, ((char*)buf)) < 0)
        return NULL;

    struct object* temp = (struct object *)calloc(sizeof(struct object), 1);

    *temp = buf[block_off];
    if (temp->id == objid)
        return temp;
	else
    	free(temp);
    return NULL;
}

void insert_to_disk(struct objfs_state *objfs, struct object *obj){
	// NOTE: What if I pass actual object instead of a pointer?
    u32 base = (B_BLOCKS) + (I_BLOCKS) + (ID_BLOCKS);
    u32 objid = obj->id;
    u32 block_no = base + ((objid - 2) / OBJECTS_PER_BLOCK);

    struct object buf[BLOCK_SIZE/sizeof(struct object)];
    if(read_block_modified(objfs, block_no, (char *)buf) < 0)
        return;

    buf[objid] = *obj;
	// Check does not matter here
	write_block_modified(objfs, block_no, (char *)buf);

    return;
}


// inserts id in HashIdTable
void insert_to_hash_table(u32 id, u32 index) {		// TODO: Check the order
    struct hashobj* temp = hashTable[index];
    struct hashobj* parent = hashTable[index];
    while(temp != NULL) {
        parent = temp;
        temp = temp->next;
    }
    if (parent == hashTable[index]) {
        hashTable[index] = (struct hashobj *)calloc(sizeof(struct hashobj), 1);
        hashTable[index]->id = id;
        return;
    }
    parent->next = (struct hashobj *)calloc(sizeof(struct hashobj), 1);
    parent->next->id = id;
    return;

}


/*
Returns the object ID.  -1 (invalid), 0, 1 - reserved
*/
long find_object_id(const char *key, struct objfs_state *objfs)
{
	u32 val = hash(key);
	struct hashobj *itr = hashTable[val];

	while (itr != NULL) {
		struct object *obj = object_fetch(objfs, itr->id);
		if(itr->id && (strcmp(obj->key, key) == 0))
			return itr->id;
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
long create_object(const char *key, struct objfs_state *objfs)
{
	u32 val = hash(key);
	struct hashobj *itr = hashTable[val];
	struct hashobj *par = hashTable[val];

	// Check if duplicate exists
	while (itr != NULL) {
		struct object *obj = object_fetch(objfs, itr->id);
		if(itr->id && (strcmp(obj->key, key) == 0)) {
			dprintf("Duplicate object for key: %s and Id: %d found", obj->key, itr->id);
			free(obj);
			return -1;
		}
		par = itr;
		itr = itr->next;
	}

	u32 new_id = fetch_free_id();		// TODO: Implement this function
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

	insert_to_hash_table(new_id, hash(key));
	insert_to_disk(objfs, newobj);
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
long destroy_object(const char *key, struct objfs_state *objfs)
{
	u32 val = hash(key);
	struct hashobj *itr = hashTable[val];

	// Check for object's existence
	while (itr != NULL) {
		struct object *obj = object_fetch(objfs, itr->id);

		if (itr->id && (strcmp(obj->key, key) == 0)) {
			remove_object_cached(obj);
			free(itr); free(obj);
			return 0;
		}
		itr = itr->next;
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
long objstore_write(int objid, const char *buf, int size, struct objfs_state *objfs)
{
	struct object *obj = object_fetch(objfs, objid);
	if (obj == NULL || objid < 2)
		return -1;

	dprintf("Doing write size = %d\n", size);
    if(find_write_cached(objfs, obj, buf, size) < 0)
        return -1;
    obj->size = size;
    return size;
}

/*
  Reads the content of the object onto the buffer with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/
long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs)
{
	struct object *obj = object_fetch(objfs, objid);
	if (obj == NULL || objid < 2)
		return -1;

	dprintf("Doing read size = %d\n", size);
	if(find_read_cached(objfs, obj, buf, size) < 0)
	   return -1;
	return size;
}

/*
  Reads the object metadata for obj->id = buf->st_ino
  Fillup buf->st_size and buf->st_blocks correctly
  See man 2 stat
*/
int fillup_size_details(struct stat *buf)
{
	// NOTE: Picked up from provided example implementation
	struct object *obj = objs + buf->st_ino - 2;
	if (buf->st_ino < 2 || obj->id != buf->st_ino)
		return -1;
	buf->st_size = obj->size;
	buf->st_blocks = obj->size >> 9;
	if (((obj->size >> 9) << 9) != obj->size)
		buf->st_blocks++;
	return 0;
}

/*
   Set your private pointeri, modified you like.
*/
int objstore_init(struct objfs_state *objfs)
{
	// struct object *obj = NULL;

	// Make it a new malloc instead (LOL)
	malloc_y(hashTable, MAX_OBJECT * (sizeof(struct hashobj)/BLOCK_SIZE));
	if (!hashTable) {		// NULL returned
		dprintf("%s: malloc: Hash Table\n", __func__);
		return -1;
	}

	malloc_y(blockmap, B_BLOCKS);
    if (!blockmap) {		// NULL returned
        dprintf("%s: malloc: blockmap\n", __func__);
        return -1;
    }

	malloc_y(inodemap, I_BLOCKS);
    if (!inodemap) {		// NULL returned
        dprintf("%s: malloc: inodemap\n", __func__);
        return -1;
    }

	// TODO: Check the exact code for this part
	char temp[BLOCK_SIZE];
	u32 check = 0, uid;
	for (int i = 0; i < ID_BLOCKS; i++) {
		if (read_block(objfs, i, (char *)temp) < 0)
			return -1;

		for (u32 j = 0; j < BLOCK_SIZE / sizeof(u32); j++) {
			uid = (u32)temp + j;					// TODO: Check the array conversion
			if (!i) {
				check = 1;
				break;
			}

			// Restore the values to hash table
			insert_to_hash_table(uid, i*BLOCK_SIZE/sizeof(u32));	// TODO: Implement this
		}
		if (check) break;
	}


	for (int i = 0; i < I_BLOCKS; i++)
		if(read_block(objfs, (ID_BLOCKS) + i, ((char *)inodemap) + (i * BLOCK_SIZE)) < 0)
	        return -1;

	for (int i = 0; i < B_BLOCKS; i++)
		if(read_block(objfs, (ID_BLOCKS) + I_BLOCKS + i, ((char *)blockmap) + (i * BLOCK_SIZE)) < 0)
	        return -1;

	for (int i = 0; i < MAX_OBJECT; i++)
		cachemap[i] = -1;

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

	// Reverse insert the hashmap entries
	for(int ctr = 0; ctr < MAX_OBJECT; ctr++){
 		node = hashTable[ctr];
		while (node != NULL) {
			temp[node->id] = ctr;
			node = node->next;
		}
	}

	for (int i = 0; i < ID_BLOCKS; i++)
		if(write_block(objfs, i, ((char *)objs) + (i * BLOCK_SIZE)) < 0)
			return -1;

	for (int i = 0; i < I_BLOCKS; i++)
		if(write_block(objfs, (ID_BLOCKS) + i, ((char *)inodemap) + (i * BLOCK_SIZE)) < 0)
			return -1;

	for (int i = 0; i < B_BLOCKS; i++)
		if(write_block(objfs, (ID_BLOCKS) + I_BLOCKS + i, ((char *)blockmap) + (i * BLOCK_SIZE)) < 0)
			return -1;

	obj_sync(objfs);
	clear_hash_table(hashTable);
	free_y(inodemap, (I_BLOCKS));
	free_y(blockmap, (B_BLOCKS));


	objfs->objstore_data = NULL;
	dprintf("Done objstore destroy\n");
	return 0;
}
