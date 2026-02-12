#define _VMATH_IMPLEMENTATION_
#define _VCOLOR_IMPLEMENTATION_
#define VGFX_PATH "redist/vgfx/"
#include "redist/vgfx/include/vgfx.h"
#include "redist/vgfx/include/vmath/vmath.h"
#include "redist/vgfx/include/vcolor/vcolor.h"
#include <stdlib.h>
#include <string.h>

static uint32_t modelEnemy = 0;
static uint32_t modelLight = 0;
static bool chaseGoal = false;
static float goal_position[3] = VRGB_BLACK;

typedef enum EntityComponent {
    COMPONENT_CONTROLL,
    COMPONENT_CAMERA,
    COMPONENT_TRANSFORM,
    COMPONENT_MODEL,
    COMPONENT_LIGHT,
    COMPONENT_COLOR,
    COMPONENT_GOAL_CHASE,
    COMPONENT_GOAL_SET,
    COMPONENT_VELOCITY,
    COMPONENT_ENVIRONMENT,
    
    COMPONENT_SIZE,
} EntityComponent;

typedef struct Entity {
    uint32_t components[(COMPONENT_SIZE + 63)/64];

    float position[3];
    float rotation[3];
    float scale[3];

    float velocity[3];
    float goal[3];
    float height;
    float color[3];
    uint32_t model;
} Entity;

typedef struct {
    Entity* base;
    uint32_t count;
    uint32_t capacity;
} Entities;

Entities entities;
Entities entitiesQueue;

void GAME_EntityComponentSet(Entity* entity, EntityComponent component);
void GAME_EntityComponentsClear(Entity* entity);
bool GAME_EntityComponentGet(Entity* entity, EntityComponent component);

void     GAME_EntitiesInit(Entities* entities, uint32_t capacity);
uint32_t GAME_EntitiesNew(Entities* entities);
Entity*  GAME_EntitiesPointerGet(Entities* entities, uint32_t entity);
void     GAME_EntitiesClear(Entities* entities);
void     GAME_EntitiesDestroy(Entities* entities);
void     GAME_EntitiesUpdate();

void GAME_SystemModelRender(Entity* entity);
void GAME_SystemCameraSet(Entity* entity);
void GAME_SystemControll(Entity* entity);
void GAME_SystemGoalSet(Entity* entity);
void GAME_SystemGoalChase(Entity* entity);
void GAME_SystemLight(Entity* entity);
void GAME_SystemMove(Entity* entity);
void GAME_SystemBackgroundColorSet(Entity* entity);

void GAME_PlayerAdd(float position[], float rotation[], float height);
void GAME_EnemyAdd(float position[], float rotation[], uint32_t model);
void GAME_LightAdd(float position[], float scale[], float color[], uint32_t model);
void GAME_BackgroundColorAdd(float color[]);

int main() {
    VG_WindowOpen("ECS", (float[]){1280, 720}, 0);
    VG_VSyncSet(true);
    VG_BackgroundColorSet(VRGBA_BLACK);
    VG_LightAmbientColorSet((float[]){0.01, 0.01, 0, 1});
    uint32_t shader_default = VG_ShaderLoad(VGFX_PATH"shaders/shader.vert", VGFX_PATH"shaders/shader.frag");
    uint32_t shader_light   = VG_ShaderLoad(VGFX_PATH"shaders/shader.vert", VGFX_PATH"shaders/light.frag");
    modelEnemy = VG_ModelNew(VGFX_PATH"models/bunny.obj", shader_default);
    modelLight = VG_ModelNew(VGFX_PATH"models/teapot.obj", shader_light);
    VG_ModelColorSet(modelLight, VRGB_YELLOW);
    GAME_EntitiesInit(&entities, 8);
    GAME_EntitiesInit(&entitiesQueue, 8);
    GAME_PlayerAdd(VRGB_BLACK, VRGB_BLACK, 0.5);
    GAME_EnemyAdd(VRGB_BLUE, VRGB_BLACK, modelEnemy);
    GAME_LightAdd((float[]){.0, 2, 0}, (float[]) {0.1, 0.1, 0.1}, VRGBA_BLUE, modelLight);
    GAME_BackgroundColorAdd((float[]){.8, .0, .0});
    
    
    while(!VG_WindowShouldClose()) {
	GAME_EntitiesUpdate();
	if (VG_KeyPressed(VG_KEY_LEFT_SHIFT)) {
	    chaseGoal = !chaseGoal;
	}
	VG_DrawingBegin();	
	for (size_t i = 0; i < entities.count; i++) {
	    Entity* entity = GAME_EntitiesPointerGet(&entities, i);
	    if (GAME_EntityComponentGet(entity, COMPONENT_ENVIRONMENT)) {
		if (GAME_EntityComponentGet(entity, COMPONENT_COLOR)) {
		    GAME_SystemBackgroundColorSet(entity);
		}
	    }
	    if (GAME_EntityComponentGet(entity, COMPONENT_TRANSFORM)) {
		if (GAME_EntityComponentGet(entity, COMPONENT_CAMERA)) {
		    GAME_SystemCameraSet(entity);
		}
		
		if (GAME_EntityComponentGet(entity, COMPONENT_LIGHT)) {
		    GAME_SystemLight(entity);
		}

		if (GAME_EntityComponentGet(entity, COMPONENT_GOAL_SET)) {
		    GAME_SystemGoalSet(entity);
		}
		
		if (GAME_EntityComponentGet(entity, COMPONENT_VELOCITY)) {
		    if (GAME_EntityComponentGet(entity, COMPONENT_CONTROLL)) {
			GAME_SystemControll(entity);
		    }
		    if (GAME_EntityComponentGet(entity, COMPONENT_GOAL_CHASE)) {
			GAME_SystemGoalChase(entity);
		    }
		    GAME_SystemMove(entity);
		}
		if (GAME_EntityComponentGet(entity, COMPONENT_MODEL)) {
		    GAME_SystemModelRender(entity);
		}
	    }
	    
	}

	VG_DrawingEnd();
    }

    VG_WindowClose();
    GAME_EntitiesDestroy(&entities);
    GAME_EntitiesDestroy(&entitiesQueue);
    return 0;
}

void GAME_SystemModelRender(Entity* entity) {
    VG_ModelColorSet(entity->model, entity->color);
    VG_ModelDrawAt(entity->model, entity->position, entity->scale);
}

void GAME_SystemLight(Entity* entity) {
    VG_LightPositionSet(entity->position);
    VG_LightColorSet(entity->color);
}

void GAME_SystemCameraSet(Entity* entity) {
    Camera* camera = VG_CameraGet();
    VM3_Copy(camera->position, entity->position);
    camera->position[1] += entity->height;
    VM3_Copy(camera->rotation, entity->rotation);
    camera->fov = V_PI/3;
}

void GAME_SystemGoalSet(Entity* entity) {
    VM3_Copy(goal_position, entity->position);
}

void GAME_SystemGoalChase(Entity* entity) {
    if (!chaseGoal) {
	VM3_Set(entity->velocity, 0, 0, 0);
    } else {
	float direction[3];
	VM3_SubtractO(goal_position, entity->position, direction);
	VM3_Normalize(direction);
	VM3_Copy(entity->velocity, direction);
    }
}

void GAME_SystemControll(Entity* entity) {
    float dt = VG_DeltaTimeGet();

    VM3_Set(entity->velocity, 0, 0, 0);
    float forward[3];
    VM3_Set(forward, 0, 0, -1);
    VM3_RotateY(forward, entity->rotation[1]);
    
//    float backward[3];
//    VM3_Set(backward, 0, 0, dt);
//    VM3_RotateY(forward, entity->rotation[1]);

    float right[3];
    VM3_Set(right, 1, 0, 0);
    VM3_RotateY(right, entity->rotation[1]);

//    float left[3];
//    VM3_Set(left, -dt, 0, 0);
//    VM3_RotateY(left, entity->rotation[1]);

    float dvelocity[3];
    VM3_Set(dvelocity, 0, 0, 0);
    
    if(VG_KeyDown(VG_KEY_W)) {
	VM3_Add(dvelocity, forward);
    }
    if (VG_KeyDown(VG_KEY_S)) {
	VM3_Subtract(dvelocity, forward);
    }
    if (VG_KeyDown(VG_KEY_A)) {
	VM3_Subtract(dvelocity, right);		
    }
    if (VG_KeyDown(VG_KEY_D)) {
	VM3_Add(dvelocity, right);
    }

    if (VG_KeyPressed(VG_KEY_SPACE)) {
	GAME_EnemyAdd(entity->position, entity->rotation, modelEnemy);
    }
    
    float mouse_delta[2];
    VG_MouseDeltaGet(mouse_delta);
    entity->rotation[1] -= mouse_delta[0]*0.010;
    VM3_Copy(entity->velocity, dvelocity);
}

void GAME_SystemMove(Entity* entity) {
    float dt = VG_DeltaTimeGet();
    float scaled_velocity[3];
    scaled_velocity[0] += entity->velocity[0]*dt;
    scaled_velocity[1] += entity->velocity[1]*dt;
    scaled_velocity[2] += entity->velocity[2]*dt;
    VM3_Add(entity->position, scaled_velocity);
}

void GAME_SystemBackgroundColorSet(Entity* entity) {
    VG_BackgroundColorSet(entity->color);
    VG_LightAmbientColorSet(entity->color);
}

void GAME_PlayerAdd(float position[], float rotation[], float height) {
    uint32_t player_handle = GAME_EntitiesNew(&entitiesQueue);
    Entity* player = GAME_EntitiesPointerGet(&entitiesQueue, player_handle);
    GAME_EntityComponentsClear(player);
    
    GAME_EntityComponentSet(player, COMPONENT_TRANSFORM);    
    VM3_Copy(player->position, position);
    VM3_Copy(player->rotation, rotation);
    VM3_Set(player->scale, 1, 1, 1);
    
    GAME_EntityComponentSet(player, COMPONENT_CONTROLL);
    GAME_EntityComponentSet(player, COMPONENT_CAMERA);
    GAME_EntityComponentSet(player, COMPONENT_GOAL_SET);
    player->height = height;

    GAME_EntityComponentSet(player, COMPONENT_VELOCITY);
    VM3_Set(player->velocity, 0, 0, 0);
}

void GAME_EnemyAdd(float position[], float rotation[], uint32_t model) {
    uint32_t enemy_handle = GAME_EntitiesNew(&entitiesQueue);
    Entity* enemy = GAME_EntitiesPointerGet(&entitiesQueue, enemy_handle);
    GAME_EntityComponentsClear(enemy);
    
    GAME_EntityComponentSet(enemy, COMPONENT_TRANSFORM);
    VM3_Copy(enemy->position, position);
    VM3_Copy(enemy->rotation, rotation);
    VM3_Set(enemy->scale, 1, 1, 1);

    GAME_EntityComponentSet(enemy, COMPONENT_MODEL);
    enemy->model = model;

    GAME_EntityComponentSet(enemy, COMPONENT_VELOCITY);
    VM3_Set(enemy->velocity, 0, 0, 0);
    
    GAME_EntityComponentSet(enemy, COMPONENT_GOAL_CHASE);
}

void GAME_LightAdd(float position[], float scale[], float color[], uint32_t model) {
    uint32_t light_handle = GAME_EntitiesNew(&entitiesQueue);
    Entity* light = GAME_EntitiesPointerGet(&entitiesQueue, light_handle);
    GAME_EntityComponentsClear(light);
    
    GAME_EntityComponentSet(light, COMPONENT_TRANSFORM);
    VM3_Copy(light->position, position);
    VM3_Copy(light->rotation, VRGB_BLACK);
    VM3_Copy(light->scale, scale);

    GAME_EntityComponentSet(light, COMPONENT_MODEL);
    light->model = model;

    GAME_EntityComponentSet(light, COMPONENT_LIGHT);
    GAME_EntityComponentSet(light, COMPONENT_COLOR);
    VM3_Copy(light->color, color);
}

void GAME_BackgroundColorAdd(float color[]) {
    uint32_t light_handle = GAME_EntitiesNew(&entitiesQueue);
    Entity* light = GAME_EntitiesPointerGet(&entitiesQueue, light_handle);
    GAME_EntityComponentsClear(light);

    GAME_EntityComponentSet(light, COMPONENT_ENVIRONMENT);
    
    GAME_EntityComponentSet(light, COMPONENT_COLOR);
    VM3_Copy(light->color, color);
}

void GAME_EntityComponentSet(Entity* entity, EntityComponent component) {
    entity->components[0] |= (1 <<component);
}

void GAME_EntityComponentsClear(Entity* entity) {
    entity->components[0] = 0;
}

bool GAME_EntityComponentGet(Entity* entity, EntityComponent component) {
    return entity->components[0] & (1 << component);
}

void GAME_EntitiesInit(Entities* entities, uint32_t capacity) {
    entities->count = 0;
    if (capacity < 1) {
	entities->capacity = 1;
    } else {
	entities->capacity = capacity;
    }
    entities->base = malloc(sizeof(Entity)*entities->capacity);
}

uint32_t GAME_EntitiesNew(Entities* entities) {
    if (entities->count + 1 >= entities->capacity) {
	entities->capacity *= 2;
	entities->base = realloc(entities->base, sizeof(Entity)*entities->capacity);
    }
    return entities->count++;
}

Entity*  GAME_EntitiesPointerGet(Entities* entities, uint32_t entity) {
    if (entity >= entities->count) {
	return NULL;
    }
    return entities->base + entity;
}

void GAME_EntitiesClear(Entities* entities) {
    entities->count = 0;
}

void GAME_EntitiesDestroy(Entities* entities) {
    entities->count = 0;
    entities->capacity = 0;
    free(entities->base);
}

void GAME_EntitiesUpdate() {
    for(size_t i = 0; i < entitiesQueue.count; i++) {
	uint32_t to_id = GAME_EntitiesNew(&entities);
	Entity* to = GAME_EntitiesPointerGet(&entities, to_id);
	
	Entity* from = GAME_EntitiesPointerGet(&entitiesQueue, i);
	
	memcpy(to, from, sizeof(Entity));
    }
    GAME_EntitiesClear(&entitiesQueue);
}
