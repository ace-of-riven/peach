#include "gm_resource_manager.h"
#include "../usr/usr_strings.h"

#include <map>

std::map<std::string , GAME_resource *> GAME_resources ;

GAME_resource::GAME_resource ( ) : name ( "empty" ) {
	this->parent = nullptr ;
}

GPU_Texture *GAME_resource_import_texture ( char *file ) {
	unsigned int bindcode = SOIL_load_OGL_texture ( TEXTURE_LOCATION_BY_NAME ( file ).c_str ( ) , SOIL_LOAD_RGBA , 0 , 0 ) ;
	if ( !bindcode )
		printf ( "Error loading texture image. (%s)\n" , file ) ;
	GPU_Texture *texture = GPU_texture_from_bindcode ( GL_TEXTURE_2D , bindcode ) ;

	return texture ;
}

gi::Path *GAME_resource_import_path ( FILE *path_file ) {
	int curve_count ;
	fscanf_s ( path_file , " %d" , &curve_count ) ;

	gi::Path *result = new gi::Path ( ) ;

	for ( int curve = 0 ; curve < curve_count ; curve++ ) {
		int curve_point_count ;
		fscanf_s ( path_file , " %d" , &curve_point_count ) ;

		result->begin_curve ( ) ;
		for ( int curve_point = 0 ; curve_point < curve_point_count ; curve_point++ ) {
			float x , y , z ;
			fscanf_s ( path_file , " %f %f %f" , &x , &y , &z ) ;
			result->add_curve_point ( glm::vec3 ( x , y , z ) ) ;
		}
		result->end_curve ( ) ;
	}

	return result ;
}

bool GAME_resource_handle_input ( GAME_resource *resource , const char *type , FILE *file ) {
	char data [ 256 ] ;

	if ( strcmp ( type , "texture" ) == 0 ) {
		fscanf_s ( file , " %s" , data , 256 ) ;

		resource->textureArr.push_back ( GAME_resource_import_texture ( data ) ) ;
		return true ;
	}
	else if ( strcmp ( type , "name" ) == 0 ) {
		fscanf_s ( file , " %s" , resource->name , 256 ) ;
		return true ;
	}
	else if ( strcmp ( type , "anim" ) == 0 ) {
		fscanf_s ( file , " %d" , &resource->animationLength ) ;
		return true ;
	}
	else if ( strcmp ( type , "speed" ) == 0 ) {
		fscanf_s ( file , " %f" , &resource->unitSpeed ) ;
		return true ;
	}
	else if ( strcmp ( type , "cooldown" ) == 0 ) {
		fscanf_s ( file , " %d" , &resource->spellCoolown ) ;
		return true ;
	}
	else if ( strcmp ( type , "damage" ) == 0 ) {
		fscanf_s ( file , " %d" , &resource->damage ) ;
		return true ;
	}
	else if ( strcmp ( type , "health" ) == 0 ) {
		fscanf_s ( file , " %d" , &resource->health ) ;
		return true ;
	}
	else if ( strcmp ( type , "range" ) == 0 ) {
		fscanf_s ( file , " %d" , &resource->range ) ;
		return true ;
	}
	else if ( strcmp ( type , "type" ) == 0 ) {
		fscanf_s ( file , " %s" , data , 256 ) ;

		switch ( data [ 0 ] ) {
			case 'E': {			// enemy
				resource->resourceType = GAME_RESOURCE_ENEMY ;
			}break;
			case 'A': {			// ally
				resource->resourceType = GAME_RESOURCE_ALLY ;
			}break;
			case 'S': {			// spell
				resource->resourceType = GAME_RESOURCE_SPELL ;
			}break;
			case 'P': {			// projectile
				resource->resourceType = GAME_RESOURCE_PROJECTILE ;
			}break;
			case 'M': {			// map
				resource->resourceType = GAME_RESOURCE_MAP ;
			}break;
			case 'T': {			// map
				resource->resourceType = GAME_RESOURCE_TOWER ;
			}break;
		}
		return true ;
	}
	else if ( strcmp ( type , "path" ) == 0 ) {
		fscanf_s ( file , " %s" , data , 256 ) ;

		FILE *in = fopen ( data , "r" ) ;
		resource->path = GAME_resource_import_path ( in ) ;
		fclose ( in ) ;
		return true ;
	}
	else if ( strcmp ( type , "{" ) == 0 ) {
		return true ;
	}

	return false ;
}

GAME_resource *GAME_resource_load ( FILE *file ) {
	GAME_resource *resource = new GAME_resource ( ) ;

	char read_header [ 256 ] ;

	while ( fscanf_s ( file , " %s" , read_header , 256 ) != EOF )
		if ( !GAME_resource_handle_input ( resource , read_header , file ) )
			break ;

	printf ( "Resource `%s` was loaded successfully with %d textures.\n" , resource->name , resource->textureArr.size ( ) ) ;

	return resource ;
}

void GAME_resource_manager_init ( const char *resource_file ) {
	FILE *file = fopen ( RESOURCE_LOCATION_BY_NAME ( resource_file ).c_str ( ) , "r" ) ;

	char data [ 256 ] ;

	std::vector<GAME_resource *> resources ;

	while ( fscanf ( file , " package {" ) != EOF ) {
		fscanf ( file , " %s" , data ) ;
		if ( strcmp ( data , "resource" ) == 0 ) {
			resources.push_back ( GAME_resource_load ( file ) ) ;
		}
		else {
			int parent ;
			GAME_resource *root = NULL ;
			for ( int tree = 0 ; tree < resources.size ( ) ; tree++ ) {
				fscanf_s ( file , " %d" , &parent ) ;
				if ( parent == -1 ) {
					resources [ tree ]->parent = NULL ;
					assert ( root == NULL ) ;
					root = resources [ tree ] ;
				}
				else {
					resources [ tree ]->parent = resources [ parent ] ;
					resources [ parent ]->childern.push_back ( resources [ tree ] ) ;
				}
			}

			assert ( root ) ;
			assert ( GAME_resources.find ( root->name ) == GAME_resources.end ( ) ) ;
			GAME_resources [ root->name ] = root ;

			printf ( "Package `%s` was loaded successfully imported.\n" , root->name ) ;

			resources.clear ( ) ;
		}
	}

	fclose ( file ) ;
}

GAME_resource *GAME_resource_request ( const char *resource_name ) {
	auto itr = GAME_resources.find ( resource_name ) ;
	if ( itr == GAME_resources.end ( ) )
		return NULL ;
	return itr->second ;
}
