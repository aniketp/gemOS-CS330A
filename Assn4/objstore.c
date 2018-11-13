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

#define STRUCT_BLOCKS			21000

#define B_BLOCKS 				1 << 8
#define I_BLOCKS				1 << 5
#define C_BLOCKS 				1 << 8

u64 metadata_store = 0;
u64 prev_meta_store = 0;

struct object{
	unsigned int id;
	unsigned int size;
	int cache_index;
	int dirty;
	char key[32];

	u32 level1[4];
	u32 level2[4];	// Don't need more than 4000

	// struct object *next;
};

struct hashobj {
	struct object obj;
	struct hashobj *next;
}

struct object *objs;
struct hashobj *hashTable[MAX_OBJECT] = {NULL};

unsigned int inodemap[MAX_INODES/8];
unsigned int blockmap[MAX_BLOCKS/8];		// Bitmap instead of Bytemap
unsigned int cachemap[MAX_OBJECT];

// Allocating 4k aligned memory area
#define malloc_4k(x) do{\
                         (x) = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);\
                         if((x) == MAP_FAILED)\
                              (x)=NULL;\
                     }while(0);

// Custom malloc
#define malloc_y(x, size) do{\
                      (x) = mmap(NULL, size * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);\
                      if((x) == MAP_FAILED)\
                           (x)=NULL;\
                  }while(0);


#define free_4k(x) munmap((x), BLOCK_SIZE)
#define free_y(x, y) munmap((x), y * BLOCK_SIZE)

/*
Returns the object ID.  -1 (invalid), 0, 1 - reserved
*/
long find_object_id(const char *key, struct objfs_state *objfs)
{
	u32 val = hash(key);
	struct hashobj *itr = hashTable[val];

	while (itr != NULL) {
		if(itr->obj.id && (strcmp(itr->obj.key, key) == 0))
			return itr->obj.id;
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
	int ctr;
    struct object *obj = objs;
    struct object *free = NULL;
    for(ctr=0; ctr < MAX_OBJS; ++ctr){
          if(obj->id == 0 && free == NULL){
                free = obj;
                free->id = ctr+2;
          }
          else if(obj->id && strcmp(obj->key, key) == 0){
                return -1;		// Object already exists
          }
          obj++;
    }

    if(!free){
               dprintf("%s: objstore full\n", __func__);
               return -1;
    }
    strcpy(free->key, key);
    init_object_cached(obj);
    return free->id;
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
   return -1;
}

/*
  Reads the content of the object onto the buffer with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/
long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs)
{
   return -1;
}

/*
  Reads the object metadata for obj->id = buf->st_ino
  Fillup buf->st_size and buf->st_blocks correctly
  See man 2 stat
*/
int fillup_size_details(struct stat *buf)
{
   return -1;
}

/*
   Set your private pointeri, anyway you like.
*/
int objstore_init(struct objfs_state *objfs)
{
	struct object *obj = NULL;

	// Make it a new malloc instead (LOL)
	malloc_y(objs, STRUCT_BLOCKS);
	if(!objs){		// NULL returned
		dprintf("%s: malloc: objs structure\n", __func__);
		return -1;
	}

	malloc_y(blockmap, B_BLOCKS);
    if(!blockmap){		// NULL returned
        dprintf("%s: malloc: blockmap\n", __func__);
        return -1;
    }

	malloc_y(inodemap, I_BLOCKS);
    if(!inodemap){		// NULL returned
        dprintf("%s: malloc: inodemap\n", __func__);
        return -1;
    }

	for (int i = 0; i < STRUCT_BLOCKS; i++)
		if(read_block(objfs, i, ((char *)objs) + (i * BLOCK_SIZE))) < 0)
			return -1;

	for (int i = 0; i < I_BLOCKS; i++)
		if(read_block(objfs, STRUCT_BLOCKS + i, ((char *)inodemap) + (i * BLOCK_SIZE))) < 0)
	        return -1;

	for (int i = 0; i < B_BLOCKS; i++)
		if(read_block(objfs, STRUCT_BLOCKS + I_BLOCKS + i, ((char *)blockmap) + (i * BLOCK_SIZE))) < 0)
	        return -1;

	for (int i = 0; i < MAX_OBJECT; i++)
		cachemap[i] = -1;

#if 0
	// Respective bitmaps for blocks and inodes
	if(read_block(objfs, 0, (char *)inodemap) < 0)
        return -1;
	if(read_block(objfs, 1, (char *)blockmap) < 0)
        return -1;
	// if(read_block(objfs, 2, (char *)objs) < 0)
    //     return -1;

	metadata_store = metadata_store + 2;
	void *temp;
	malloc_4k(temp);
	if (!temp)
		return -1;

	for(int i = 0; i < (MAX_OBJS << 10); i++){
		metadata_store = metadata_store + 1;
        if(read_block(objfs, metadata_store, temp) < 0)
			return -1;

        for(int j = 0; j < 1024; j++) {
            if(!*((int *)temp + j))
				continue;
            push_to_ht(*((int *)temp + j), (i >> 10) + j);
        }
    }

	prev_meta_store = metadata_store;
	metadata_store = metadata_store + (OBJECT_BLOCKS);
	free_4k(temp);
	objfs->objstore_data = objs;
#endif

	dprintf("Done objstore init\n");
	return 0;
}

/*
   Cleanup private data. FS is being unmounted
*/
int objstore_destroy(struct objfs_state *objfs)
{
	struct object *obj = objs;
	for(int ctr = 0; ctr < MAX_OBJECT; ctr++, obj++){
			 if(obj->id)
				 obj_sync(objfs, obj);
	}

	for (int i = 0; i < STRUCT_BLOCKS; i++)
		if(write_block(objfs, i, ((char *)objs) + (i * BLOCK_SIZE))) < 0)
			return -1;

	for (int i = 0; i < I_BLOCKS; i++)
		if(write_block(objfs, STRUCT_BLOCKS + i, ((char *)inodemap) + (i * BLOCK_SIZE))) < 0)
			return -1;

	for (int i = 0; i < B_BLOCKS; i++)
		if(write_block(objfs, STRUCT_BLOCKS + I_BLOCKS + i, ((char *)blockmap) + (i * BLOCK_SIZE))) < 0)
			return -1;

	free_y(objs, STRUCT_BLOCKS);
	free_y(inodemap, (I_BLOCKS));
	free_y(blockmap, (B_BLOCKS));

#if 0
	metadata_store = metadata_store - (OBJECT_BLOCKS);

	// Initialize
	int hashidx[MAX_OBJS];
	for (int i = 0; i < MAX_OBJS; i++)
		hashidx[i] = 0;

	// Respective bitmaps for blocks and inodes
	if(write_block(objfs, 0, (char *)inodemap) < 0)
		return -1;
	if(write_block(objfs, 1, (char *)blockmap) < 0)
		return -1;
	// if(write_block(objfs, 2, (char *)objs) < 0)
	// 	return -1;

	free_4k(inodemap);
	free_4k(blockmap);
	// free_4k(objfs);
#endif

	objfs->objstore_data = NULL;
	dprintf("Done objstore destroy\n");
	return 0;
}
