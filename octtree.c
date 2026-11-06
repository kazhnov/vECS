#define MIN_NODE_SIZE 0.125

struct OctNode_t;
typedef OctNode_t OctNode;

typedef enum {
    OCTNODE_CHUNK,
    OCTNODE_ENTITIES,

    OCTNODE_COUNT
} OctNodeType;

typedef struct {
    union {
	OctNode* nodes;
	void* entities;
    } value;
    unsigned int count;
} OctNode;

typedef struct {
    

} OctTree;
