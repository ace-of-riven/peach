#include "gm_tower.h"
#include "gm_map.h"

GAME_tower *GAME_tower_create ( GAME_resource *source , GAME_map *map , int position ) {
	GAME_tower *tower = new GAME_tower ( ) ;
	tower->source = source ;
	tower->transform.set_translation ( map->tower_locations [ position ] ) ;
	tower->lastAttack = -1 ;
	tower->tickCount = 0 ;
	return tower ;
}

void GAME_tower_tick_step ( GAME_tower *tower ) {
	tower->tickCount++ ;
	if ( tower->lastAttack + tower->source->unitSpeed <= tower->tickCount ) {
		// GAME_enemy *enemy = GAME_enemy_closest ( tower->transform.position ( ) ) ;
		tower->lastAttack = tower->tickCount ;			// if attack was successfull
	}
}

void GAME_tower_render ( GAME_tower *tower ) {
	if ( tower->tickCount - tower->lastAttack > tower->source->animationLength ) {		// idling
		tower->transform.set_scalation ( glm::vec3 ( glm::normalize ( glm::vec2 ( tower->source->textureArr [ 0 ]->w , tower->source->textureArr [ 0 ]->h ) ) * 10.0f , 1.0f ) ) ;
		gi::PlaneBatch::PushPlane ( tower->transform , tower->source->textureArr [ 0 ] ) ;
	}
	else {											// still in attack animation
		tower->transform.set_scalation ( glm::vec3 ( glm::normalize ( glm::vec2 ( tower->source->textureArr [ 1 ]->w , tower->source->textureArr [ 1 ]->h ) ) * 10.0f , 1.0f ) ) ;
		gi::PlaneBatch::PushPlane ( tower->transform , tower->source->textureArr [ 1 ] ) ;
	}
}
