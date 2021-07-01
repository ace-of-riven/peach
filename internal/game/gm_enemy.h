#pragma once

#include "../interface/int_transform.h"
#include "../interface/int_path.h"
#include "../interface/int_plane_batch.h"

#include "gm_resource_manager.h"

struct GAME_enemy {
	GAME_resource *enemy_resource ;

	int animationState ;
	float pathLocation ;

	glm::vec3 offset ;

	gi::Path *path ;
	gi::Transform transform ;
};

GAME_enemy *GAME_enemy_create ( GAME_resource *source , gi::Path *path , int i ) ;

void GAME_enemy_tick_step ( GAME_enemy *enemy ) ;

void GAME_enemies_render ( ) ;
void GAME_enemies_tick_step ( ) ;

void GAME_enemy_discard ( GAME_enemy *enemy ) ;
