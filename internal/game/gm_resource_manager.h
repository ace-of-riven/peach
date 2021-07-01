#pragma once

#include <vector>

#include "../gpu/gpu_includes.h"

#include "../interface/int_transform.h"
#include "../interface/int_path.h"
#include "../interface/int_plane_batch.h"

enum eGAMEResourceType {
	GAME_RESOURCE_ENEMY ,
	GAME_RESOURCE_PROJECTILE ,
	GAME_RESOURCE_ALLY ,
	GAME_RESOURCE_TOWER ,
	GAME_RESOURCE_MAP ,
	GAME_RESOURCE_SPELL
};

struct GAME_resource {
	GAME_resource ( ) ;

	const char name [ 256 ] ;

	eGAMEResourceType resourceType ;

	int health ;
	int damage ;

	int range ;

	float unitSpeed ;
	int spellCoolown ;
	int animationLength ;
	std::vector<GPU_Texture *> textureArr ;

	gi::Path *path ;

	GAME_resource *parent ;
	std::vector<GAME_resource *> childern ;
};

void GAME_resource_manager_init ( const char *resource_file ) ;

GAME_resource *GAME_resource_request ( const char *resource_name ) ;

GPU_Texture *GAME_resource_import_texture ( char *file ) ;

gi::Path *GAME_resource_import_path ( FILE *path_file ) ;
