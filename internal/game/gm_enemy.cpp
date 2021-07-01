#include "gm_enemy.h"

#include <set>

std::set<GAME_enemy *> enemies_list ;

float random ( ) {
	return 1.0f - ( float ( rand ( ) ) / RAND_MAX ) * 2.0f ;
}

GAME_enemy *GAME_enemy_create ( GAME_resource *source , gi::Path *path , int i ) {
	GAME_enemy *result = new GAME_enemy ( ) ;
	result->animationState = 0 ;
	result->pathLocation = -i * source->unitSpeed * 16.0f ;
	result->enemy_resource = source ;
	result->path = path ;
	result->offset = glm::vec3 ( random ( ) * 2.0f , random ( ) * 2.0f , 0.0f ) ;

	enemies_list.insert ( result ) ;

	return result ;
}

void GAME_enemy_discard ( GAME_enemy *enemy ) {
	std::set<GAME_enemy *>::iterator itr ;
	if ( ( itr = std::find ( enemies_list.begin ( ) , enemies_list.end ( ) , enemy ) ) != enemies_list.begin ( ) )
		enemies_list.erase ( itr ) ;
}

void GAME_enemy_tick_step ( GAME_enemy *enemy ) {
	enemy->animationState++;
	enemy->pathLocation += enemy->enemy_resource->unitSpeed ;
	enemy->transform.set_translation ( enemy->path->pos_in_path ( enemy->pathLocation , 0.0f , 1.0f ) + enemy->offset ) ;

	glm::vec3 direction = enemy->path->dir_in_path ( enemy->pathLocation , 0.0f , 1.0f )  ;

	if ( direction.x > 0.0f )
		enemy->transform.set_rotation ( 0.0f , 0.0f , 0.0f ) ;
	else
		enemy->transform.set_rotation ( 0.0f , 180.0f , 0.0f ) ;

	if ( enemy->pathLocation > 1.0f ) {
		GAME_enemy_discard ( enemy ) ;
	}
}

void GAME_enemies_tick_step ( ) {
	for ( std::set<GAME_enemy *>::iterator itr = enemies_list.begin ( ) ; itr != enemies_list.end ( ) ; ) {
		std::set<GAME_enemy *>::iterator next = itr ; next++;
		GAME_enemy_tick_step ( ( *itr ) ) ;
		itr = next ;
	}
}

void GAME_enemies_render ( ) {
	for ( std::set<GAME_enemy *>::iterator itr = enemies_list.begin ( ) ; itr != enemies_list.end ( ) ; itr++ ) {
		int anim_state = ( ( *itr )->animationState / ( *itr )->enemy_resource->animationLength ) % ( *itr )->enemy_resource->textureArr.size ( ) ;
		int w = ( *itr )->enemy_resource->textureArr [ anim_state ]->w;
		int h = ( *itr )->enemy_resource->textureArr [ anim_state ]->h;
		( *itr )->transform.set_scalation ( glm::vec3 ( glm::normalize ( glm::vec2 ( w , h ) ) * 2.0f , 1.0f ) ) ;
		gi::PlaneBatch::PushPlane ( ( *itr )->transform , ( *itr )->enemy_resource->textureArr [ anim_state ] ) ;
	}
}
