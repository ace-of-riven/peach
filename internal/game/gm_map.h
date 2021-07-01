#pragma once

#include "../interface/int_transform.h"
#include "../interface/int_path.h"
#include "../interface/int_plane_batch.h"

#include "gm_resource_manager.h"
#include "gm_enemy.h"
#include "gm_tower.h"

struct GAME_map_wave {
	int spawn_tick ;
	
	std::vector<int> enemyCount ;
	std::vector<GAME_resource *> enemyType ;
};

struct GAME_map {
	gi::Path *path ;

	GPU_Texture *texture ;

	int current_wave ;
	int current_tick ;

	std::vector<GAME_map_wave *> waves ;

	std::vector<glm::vec3> tower_locations ;
	std::vector<GAME_tower *> towers ;
};

GAME_map *GAME_map_import_from_file ( const char *file ) ;

void GAME_map_tick_step ( GAME_map * ) ;

void GAME_map_render ( GAME_map * ) ;

bool GAME_map_build ( GAME_map *map , GAME_resource *source , int position ) ;
