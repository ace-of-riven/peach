#include "gm_map.h"

#include "../usr/usr_strings.h"

GAME_map_wave *GAME_map_wave_import ( FILE *file ) {
	GAME_map_wave *result = new GAME_map_wave ( ) ;

	char name [ 256 ] ;
	int time , enemies , count ;

	fscanf_s ( file , " %d %d" , &time , &enemies ) ;

	result->spawn_tick = time ;

	for ( int enemy = 0 ; enemy < enemies ; enemy++ ) {
		fscanf_s ( file , " %d %s" , &count , name , 256 ) ;
		result->enemyCount.push_back ( count ) ;
		result->enemyType.push_back ( GAME_resource_request ( name ) ) ;
	}

	return result ;
}

GAME_map *GAME_map_import_from_file ( const char *file ) {
	FILE *in = fopen ( RESOURCE_LOCATION_BY_NAME ( file ).c_str ( ) , "r" ) ;

	char texture_location [ 256 ] ;
	GAME_map *result = new GAME_map ( ) ;

	result->path = GAME_resource_import_path ( in ) ;
	fscanf_s ( in , " %s" , texture_location , 256 ) ;
	result->texture = GAME_resource_import_texture ( texture_location ) ;

	int tower_count ;
	fscanf_s ( in , "%d" , &tower_count ) ;

	for ( int tower = 0 ; tower < tower_count ; tower++ ) {
		float x , y , z ;
		fscanf_s ( in , " %f %f %f" , &x , &y , &z ) ;
		result->tower_locations.push_back ( glm::vec3 ( x , y , z ) ) ;
		result->towers.push_back ( NULL ) ;
	}

	int wave_count ;
	fscanf_s ( in , " %d" , &wave_count ) ;

	for ( int wave = 0 ; wave < wave_count ; wave++ )
		result->waves.push_back ( GAME_map_wave_import ( in ) ) ;

	result->current_wave = 0 ;
	result->current_tick = 0 ;

	fclose ( in ) ;

	return result ;
}

void GAME_map_spawn_wave ( GAME_map *map , GAME_map_wave *wave ) {
	for ( int i = 0 ; i < wave->enemyType.size ( ) ; i++ ) {
		for ( int j = 0 ; j < wave->enemyCount [ i ] ; j++ )
			GAME_enemy_create ( wave->enemyType [ i ] , map->path , j ) ;
	}
}

void GAME_map_tick_step ( GAME_map *map ) {
	map->current_tick++ ;
	if ( map->current_wave < map->waves.size ( ) and map->waves [ map->current_wave ]->spawn_tick < map->current_tick ) {
		GAME_map_spawn_wave ( map , map->waves [ map->current_wave++ ] ) ;
	}
	for ( int i = 0 ; i < map->towers.size ( ) ; i++ )
		if ( map->towers [ i ] )
			GAME_tower_tick_step ( map->towers [ i ] ) ;
	GAME_enemies_tick_step ( ) ;
}

void GAME_map_render ( GAME_map *map ) {
	gi::PlaneBatch::PushPlane ( glm::translate ( glm::vec3 ( 50.0f , 50.0f , 0.0f ) ) * glm::scale ( glm::vec3 ( 50.0f , 50.0f , 1.0f ) ) , map->texture ) ;
	map->path->sample_render ( ) ;
	GAME_enemies_render ( ) ;
	for ( int i = 0 ; i < map->towers.size ( ) ; i++ ) {
		if ( map->towers [ i ] )
			GAME_tower_render ( map->towers [ i ] ) ;
	}
}

bool GAME_map_build ( GAME_map *map , GAME_resource *source , int position ) {
	if ( map->towers [ position ] )
		return false ;
	map->towers [ position ] = GAME_tower_create ( source , map , position ) ;
	return true ;
}
