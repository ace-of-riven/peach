#pragma once

#include "../interface/int_transform.h"
#include "../interface/int_path.h"
#include "../interface/int_plane_batch.h"

#include "gm_resource_manager.h"

struct GAME_tower {
	GAME_resource *source ;

	int lastAttack ;				// tick
	int tickCount ;

	gi::Transform transform ;
};

GAME_tower *GAME_tower_create ( GAME_resource *source , struct GAME_map *map , int position ) ;

void GAME_tower_tick_step ( GAME_tower  *tower ) ;

void GAME_tower_render ( GAME_tower *tower ) ;
