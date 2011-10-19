/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'analysis_config.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <util.h>
#include <enkf_types.h>
#include <analysis_config.h>
#include <enkf_defaults.h>
#include <rng.h>
#include <analysis_module.h>
#include "config_keys.h"


struct analysis_config_struct {
  hash_type            * analysis_modules;
  analysis_module_type * analysis_module;
  char                  * log_path;                    /* Points to directory with update logs. */
  bool                    merge_observations;          /* When observing from time1 to time2 - should ALL observations in between be used? */
  bool                    rerun;                       /* Should we rerun the simulator when the parameters have been updated? */
  int                     rerun_start;                 /* When rerunning - from where should we start? */

  double                  overlap_alpha;
  double                  std_cutoff;
    
  bool                    update_results;              /* Should result values like e.g. WWCT be updated? */
  bool                    single_node_update;          /* When creating the default ALL_ACTIVE local configuration. */ 
}; 








void analysis_config_set_alpha( analysis_config_type * config , double alpha) {
  config->overlap_alpha = alpha;
}

double analysis_config_get_alpha(const analysis_config_type * config) {
  return config->overlap_alpha;
}

void analysis_config_set_std_cutoff( analysis_config_type * config , double std_cutoff ) {
  config->std_cutoff = std_cutoff;
}

double analysis_config_get_std_cutoff(const analysis_config_type * config) {
  return config->std_cutoff;
}


void analysis_config_set_log_path(analysis_config_type * config , const char * log_path ) {
  config->log_path        = util_realloc_string_copy(config->log_path , log_path);
}

/**
   Will in addition create the path.
*/
const char * analysis_config_get_log_path( const analysis_config_type * config ) {
  util_make_path( config->log_path );
  return config->log_path; 
}



void analysis_config_set_rerun_start( analysis_config_type * config , int rerun_start ) {
  config->rerun_start = rerun_start;
}

void analysis_config_set_rerun(analysis_config_type * config , bool rerun) {
  config->rerun = rerun;
}

bool analysis_config_get_rerun(const analysis_config_type * config) {
  return config->rerun;
}

void analysis_config_set_update_results(analysis_config_type * config , bool update_results) {
  config->update_results = update_results;
}

bool analysis_config_get_update_results(const analysis_config_type * config) {
  return config->update_results;
}

void analysis_config_set_single_node_update(analysis_config_type * config , bool single_node_update) {
  config->single_node_update = single_node_update;
}

bool analysis_config_get_single_node_update(const analysis_config_type * config) {
  return config->single_node_update;
}


int analysis_config_get_rerun_start(const analysis_config_type * config) {
  return config->rerun_start;
}


void analysis_config_set_merge_observations( analysis_config_type * config , bool merge_observations) {
  config->merge_observations = merge_observations;
}



/*****************************************************************/

void analysis_config_load_internal_module( analysis_config_type * config , rng_type * rng , 
                                           const char * user_name , const char * symbol_table ) {
  analysis_module_type * module = analysis_module_alloc_internal( rng , user_name , symbol_table );
  if (module != NULL)
    hash_insert_hash_owned_ref( config->analysis_modules , user_name , module , analysis_module_free__ );
  else
    fprintf(stderr,"** Warning: failed to load module %s from %s.\n",user_name , symbol_table);
}


void analysis_config_load_external_module( analysis_config_type * config , rng_type * rng , 
                                           const char * user_name , const char * lib_name) {
  analysis_module_type * module = analysis_module_alloc_external( rng , user_name , lib_name );
  if (module != NULL)
    hash_insert_hash_owned_ref( config->analysis_modules , user_name , module , analysis_module_free__ );
  else
    fprintf(stderr,"** Warning: failed to load module %s from %s.\n",user_name , lib_name);
}


analysis_module_type * analysis_config_get_module( analysis_config_type * config , const char * module_name ) {
  return hash_get( config->analysis_modules , module_name );
}

bool analysis_config_has_module(analysis_config_type * config , const char * module_name) {
  return hash_has_key( config->analysis_modules , module_name );
}

void analysis_config_select_module( analysis_config_type * config , const char * module_name ) {
  if (analysis_config_has_module( config , module_name )) 
    config->analysis_module = analysis_config_get_module( config , module_name );
  else {
    if (config->analysis_module == NULL)
      util_abort("%s: sorry module:%s does not exist - and no module currently selected\n",__func__ , module_name);
    else
      fprintf(stderr , "** Warning: analysis module:%s does not exists - current selection unchanged:%s\n", 
              module_name , 
              analysis_module_get_name( config->analysis_module ));
    
  }
}

analysis_module_type * analysis_config_get_active_module( analysis_config_type * config ) {
  return config->analysis_module;
}

/*****************************************************************/


void analysis_config_load_internal_modules( analysis_config_type * config , rng_type * rng) {
  analysis_config_load_internal_module( config , rng , "STD_ENKF"       , "std_enkf_symbol_table");
  analysis_config_load_internal_module( config , rng , "SQRT_ENKF"      , "sqrt_enkf_symbol_table");
  analysis_config_load_internal_module( config , rng , "CV_ENKF"        , "cv_enkf_symbol_table");
  analysis_config_load_internal_module( config , rng , "BOOTSTRAP_ENKF" , "bootstrap_enkf_symbol_table");
  analysis_config_select_module( config , DEFAULT_ANALYSIS_MODULE);
}

/**
   The analysis_config object is instantiated with the default values
   for enkf_defaults.h
*/

void analysis_config_init( analysis_config_type * analysis , const config_type * config , rng_type * rng) {
  if (config_item_set( config , UPDATE_LOG_PATH_KEY ))
    analysis_config_set_log_path( analysis , config_get_value( config , UPDATE_LOG_PATH_KEY ));
  
  if (config_item_set( config , STD_CUTOFF_KEY ))
    analysis_config_set_std_cutoff( analysis , config_get_value_as_double( config , STD_CUTOFF_KEY ));

  if (config_item_set( config , ENKF_ALPHA_KEY ))
    analysis_config_set_alpha( analysis , config_get_value_as_double( config , ENKF_ALPHA_KEY ));

  if (config_item_set( config , ENKF_MERGE_OBSERVATIONS_KEY ))
    analysis_config_set_merge_observations( analysis , config_get_value_as_bool( config , ENKF_MERGE_OBSERVATIONS_KEY ));

  if (config_item_set( config , ENKF_RERUN_KEY ))
    analysis_config_set_rerun( analysis , config_get_value_as_bool( config , ENKF_RERUN_KEY ));

  if (config_item_set( config , UPDATE_RESULTS_KEY ))
    analysis_config_set_update_results( analysis , config_get_value_as_bool( config , UPDATE_RESULTS_KEY ));

  if (config_item_set( config , SINGLE_NODE_UPDATE_KEY ))
    analysis_config_set_single_node_update( analysis , config_get_value_as_bool( config , SINGLE_NODE_UPDATE_KEY ));
  
  if (config_item_set( config , RERUN_START_KEY ))
    analysis_config_set_rerun_start( analysis , config_get_value_as_int( config , RERUN_START_KEY ));
  
  /* Loading external modules */
  {
    for (int i=0; i < config_get_occurences( config , ANALYSIS_LOAD_KEY ); i++) {
      const stringlist_type * tokens = config_iget_stringlist_ref( config , ANALYSIS_SET_VAR_KEY , i);
      const char * user_name = stringlist_iget( tokens , 0 );
      const char * lib_name  = stringlist_iget( tokens , 1 );
      
      analysis_config_load_external_module( analysis , rng , user_name , lib_name);
    }
  }

  /* Setting variables for analysis modules */
  {
    for (int i=0; i < config_get_occurences( config , ANALYSIS_SET_VAR_KEY ); i++) {
      const stringlist_type * tokens = config_iget_stringlist_ref( config , ANALYSIS_SET_VAR_KEY , i);
      const char * module_name = stringlist_iget( tokens , 0 );
      const char * var_name    = stringlist_iget( tokens , 1 );
      char       * value       = stringlist_alloc_joined_segment_string( tokens , 2 , stringlist_get_size( tokens ) , " " );
      analysis_module_type * module = analysis_config_get_module( analysis , module_name );
      
      analysis_module_set_var( module , var_name , value );
      
      free( value );
    }
  }

  if (config_item_set( config, ANALYSIS_SELECT_KEY )) 
    analysis_config_select_module( analysis , config_get_value( config , ANALYSIS_SELECT_KEY ));
  
}



bool analysis_config_get_merge_observations(const analysis_config_type * config) {
  return config->merge_observations;
}







void analysis_config_free(analysis_config_type * config) {
  hash_free( config->analysis_modules );
  free(config->log_path);
  free(config);
}



analysis_config_type * analysis_config_alloc_default( ) {
  analysis_config_type * config = util_malloc( sizeof * config , __func__);
  
  config->log_path                  = NULL;

  analysis_config_set_alpha( config                    , DEFAULT_ENKF_ALPHA );
  analysis_config_set_std_cutoff( config               , DEFAULT_ENKF_STD_CUTOFF );
  analysis_config_set_merge_observations( config       , DEFAULT_MERGE_OBSERVATIONS );
  analysis_config_set_rerun( config                    , DEFAULT_RERUN );
  analysis_config_set_rerun_start( config              , DEFAULT_RERUN_START );
  analysis_config_set_update_results( config           , DEFAULT_UPDATE_RESULTS);
  analysis_config_set_single_node_update( config       , DEFAULT_SINGLE_NODE_UPDATE );

  config->analysis_module  = NULL;
  config->analysis_modules = hash_alloc();
  return config;
}



/*****************************************************************/
/*
  Keywords for the analysis - all optional. The analysis_config object
  is instantiated with defaults from enkf_defaults.h
*/

void analysis_config_add_config_items( config_type * config ) {
  config_item_type * item;
  
  config_add_key_value( config , ENKF_ALPHA_KEY              , false , CONFIG_FLOAT);
  config_add_key_value( config , ENKF_MERGE_OBSERVATIONS_KEY , false , CONFIG_BOOLEAN);
  config_add_key_value( config , UPDATE_RESULTS_KEY          , false , CONFIG_BOOLEAN);
  config_add_key_value( config , SINGLE_NODE_UPDATE_KEY      , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_CROSS_VALIDATION_KEY   , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_LOCAL_CV_KEY           , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_PEN_PRESS_KEY          , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_SCALING_KEY            , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_KERNEL_REG_KEY         , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_KERNEL_FUNC_KEY        , false , CONFIG_INT);
  config_add_key_value( config , ENKF_KERNEL_PARAM_KEY       , false , CONFIG_INT);
  config_add_key_value( config , ENKF_FORCE_NCOMP_KEY        , false , CONFIG_BOOLEAN);
  config_add_key_value( config , ENKF_NCOMP_KEY              , false , CONFIG_INT);
  config_add_key_value( config , ENKF_CV_FOLDS_KEY           , false , CONFIG_INT);
  config_add_key_value( config , ENKF_RERUN_KEY              , false , CONFIG_BOOLEAN);
  config_add_key_value( config , RERUN_START_KEY             , false , CONFIG_INT);
  config_add_key_value( config , UPDATE_LOG_PATH_KEY         , false , CONFIG_STRING);

  config_add_key_value( config , ANALYSIS_SELECT_KEY         , false , CONFIG_STRING);

  item = config_add_item( config , ANALYSIS_LOAD_KEY , false , true );
  config_item_set_argc_minmax( item , 2 , 2 , 0 , NULL );  
  
  item = config_add_item( config , ANALYSIS_SET_VAR_KEY , false , true );
  config_item_set_argc_minmax( item , 3 , -1 , 0 , NULL );
}



void analysis_config_fprintf_config( analysis_config_type * config , FILE * stream) {
  fprintf( stream , CONFIG_COMMENTLINE_FORMAT );
  fprintf( stream , CONFIG_COMMENT_FORMAT , "Here comes configuration information related to the EnKF analysis.");
  
  
  if (config->merge_observations != DEFAULT_MERGE_OBSERVATIONS) {
    fprintf( stream , CONFIG_KEY_FORMAT        , ENKF_MERGE_OBSERVATIONS_KEY);
    fprintf( stream , CONFIG_ENDVALUE_FORMAT   , CONFIG_BOOL_STRING( config->merge_observations ));
  }

  if (config->update_results != DEFAULT_UPDATE_RESULTS) {
    fprintf( stream , CONFIG_KEY_FORMAT        , UPDATE_RESULTS_KEY);
    fprintf( stream , CONFIG_ENDVALUE_FORMAT   , CONFIG_BOOL_STRING( config->update_results ));
  }

  if (config->std_cutoff != DEFAULT_ENKF_STD_CUTOFF) {
    fprintf( stream , CONFIG_KEY_FORMAT   , STD_CUTOFF_KEY );
    fprintf( stream , CONFIG_FLOAT_FORMAT , config->std_cutoff );
    fprintf( stream , "\n");
  }

  if (config->overlap_alpha != DEFAULT_ENKF_ALPHA ) {
    fprintf( stream , CONFIG_KEY_FORMAT   , ENKF_TRUNCATION_KEY );
    fprintf( stream , CONFIG_FLOAT_FORMAT , config->overlap_alpha );
    fprintf( stream , "\n");
  }
  
  if (config->update_results != DEFAULT_SINGLE_NODE_UPDATE) {
    fprintf( stream , CONFIG_KEY_FORMAT        , SINGLE_NODE_UPDATE_KEY);
    fprintf( stream , CONFIG_ENDVALUE_FORMAT   , CONFIG_BOOL_STRING( config->single_node_update ));
  }
  
  if (config->rerun) {
    fprintf( stream , CONFIG_KEY_FORMAT        , ENKF_RERUN_KEY);
    fprintf( stream , CONFIG_ENDVALUE_FORMAT   , CONFIG_BOOL_STRING( config->rerun ));
  }
  
  if (config->rerun_start != DEFAULT_RERUN_START) {
    fprintf( stream , CONFIG_KEY_FORMAT   , RERUN_START_KEY);
    fprintf( stream , CONFIG_INT_FORMAT   , config->rerun_start );
    fprintf( stream , "\n");
  }

  if (config->log_path != NULL) {
    fprintf( stream , CONFIG_KEY_FORMAT      , UPDATE_LOG_PATH_KEY);
    fprintf( stream , CONFIG_ENDVALUE_FORMAT , config->log_path );
  }
  
  fprintf(stream , "\n\n");
}



