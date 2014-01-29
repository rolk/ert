/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'enkf_main_jobs.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/util/stringlist.h>
#include <ert/util/string_util.h>
#include <ert/util/int_vector.h>

#include <ert/enkf/enkf_main.h>
#include <ert/enkf/field_config.h>


static bool_vector_type * alloc_iactive_vector_from_range(const stringlist_type * range, int startindex, int ens_size) {
  bool_vector_type * iactive;
  if (stringlist_get_size(range) > startindex) {
    char * arg_string = stringlist_alloc_joined_substring( range, startindex, stringlist_get_size(range), "");
    iactive = bool_vector_alloc(ens_size, false);
    string_util_update_active_mask( arg_string, iactive );
    free ( arg_string );
  } else {
    iactive = bool_vector_alloc(ens_size, true);
  }
  return iactive;
}


void * enkf_main_exit_JOB(void * self , const stringlist_type * args ) {
  enkf_main_type  * enkf_main = enkf_main_safe_cast( self );
  enkf_main_exit( enkf_main );
  return NULL;
}


void * enkf_main_assimilation_JOB( void * self , const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  int ens_size                 = enkf_main_get_ensemble_size( enkf_main );
  bool_vector_type * iactive   = bool_vector_alloc( 0 , true );

  bool_vector_iset( iactive , ens_size - 1 , true );
  enkf_main_run_assimilation( enkf_main , iactive , 0 , 0 ,  ANALYZED );
  return NULL;
}

void * enkf_main_analysis_enkf_update_JOB( void * self , const stringlist_type * args) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  enkf_fs_type * target_fs = enkf_main_get_fs( enkf_main );
  int target_step;
  int_vector_type * step_list;

  
  // Argument 0: The number of the step to write to
  if (stringlist_get_size(args) > 1) 
    util_sscanf_int(stringlist_iget( args , 1) , &target_step);
  else
    target_step = 0;

  // Argument 1 - ??: The timesteps to use in the update
  if (stringlist_get_size( args ) > 2) {
    char * step_args = stringlist_alloc_joined_substring(args , 2 , stringlist_get_size(args) , " ");
    step_list = string_util_alloc_active_list( step_args );
    free( step_args );
  } else 
    step_list = int_vector_alloc(1,target_step);
  
  enkf_main_UPDATE( enkf_main , step_list , target_fs , target_step , SMOOTHER_UPDATE);
  
  int_vector_free( step_list );
  return NULL;
}



#define CURRENT_CASE_STRING "*"
void * enkf_main_analysis_update_JOB( void * self , const stringlist_type * args) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  enkf_fs_type * target_fs;
  int target_step;
  int_vector_type * step_list;

  // Argument 0: Which case to write to
  if (stringlist_get_size(args)) {
    const char * target_fs_name = stringlist_iget( args , 0 );
    if (strcmp( target_fs_name , CURRENT_CASE_STRING) == 0)
      target_fs = enkf_fs_get_ref( enkf_main_get_fs( enkf_main ));
    else
      target_fs = enkf_main_mount_alt_fs( enkf_main , target_fs_name , false , true);
  } else
      target_fs = enkf_fs_get_ref( enkf_main_get_fs( enkf_main ));


  // Argument 1: The number of the step to write to
  if (stringlist_get_size(args) > 1) 
    util_sscanf_int(stringlist_iget( args , 1) , &target_step);
  else
    target_step = 0;

  // Argument 2 - ??: The timesteps to use in the update
  if (stringlist_get_size( args ) > 2) {
    char * step_args = stringlist_alloc_joined_substring(args , 2 , stringlist_get_size(args) , " ");
    step_list = string_util_alloc_active_list( step_args );
    free( step_args );
  } else {
    int stride = 1;
    time_map_type * time_map = enkf_fs_get_time_map( enkf_main_get_fs( enkf_main ));
    step_list = enkf_main_update_alloc_step_list( enkf_main , 0 , time_map_get_last_step( time_map ) , stride);
  }
  
  enkf_main_UPDATE( enkf_main , step_list , target_fs , target_step , SMOOTHER_UPDATE);
  
  int_vector_free( step_list );
  enkf_fs_umount( target_fs );
  return NULL;
}
#undef CURRENT_CASE_STRING




void * enkf_main_ensemble_run_JOB( void * self , const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  int ens_size                 = enkf_main_get_ensemble_size( enkf_main );
  bool_vector_type * iactive = alloc_iactive_vector_from_range(args, 0, ens_size);

  bool_vector_iset( iactive , ens_size - 1 , true );
  enkf_main_run_exp( enkf_main , iactive , true , 0 , 0 , ANALYZED);
  bool_vector_free(iactive);
  return NULL;
}


#define CURRENT_CASE_STRING "*"
void * enkf_main_smoother_JOB( void * self , const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  int ens_size                 = enkf_main_get_ensemble_size( enkf_main );
  bool_vector_type * iactive   = bool_vector_alloc( ens_size , true );
  bool valid                   = true;
  const char * target_case;
  enkf_fs_type * target_fs     = enkf_main_get_fs( enkf_main );


  // Argument 0: Which case to write to. Default current case.
  if (stringlist_get_size(args)) {
    target_case = stringlist_iget( args , 0 );
    if (strcmp( target_case , CURRENT_CASE_STRING) == 0)
      target_case = enkf_fs_get_case_name(target_fs);
  } else
    target_case = enkf_fs_get_case_name(target_fs);

  //Argument 1: Rerun. Default false.
  bool rerun = (stringlist_get_size(args) >= 2) ? stringlist_iget_as_bool(args, 1, &valid) : false;

  if (!valid) {
      fprintf(stderr, "** Warning: Function %s : Second argument must be a bool value. Exiting job\n", __func__);
      return NULL;
  }

  enkf_main_run_smoother( enkf_main , target_case , iactive , rerun);
  bool_vector_free( iactive );
  return NULL;
}
#undef CURRENT_CASE_STRING


void * enkf_main_iterated_smoother_JOB( void * self , const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  const analysis_config_type * analysis_config = enkf_main_get_analysis_config(enkf_main);
  analysis_iter_config_type * iter_config = analysis_config_get_iter_config(analysis_config);
  int num_iter = analysis_iter_config_get_num_iterations(iter_config);
  
  enkf_main_run_iterated_ES( enkf_main , 0 , num_iter);
  return NULL;
}


void * enkf_main_select_module_JOB( void * self , const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  analysis_config_type * analysis_config = enkf_main_get_analysis_config( enkf_main );
  
  analysis_config_select_module( analysis_config , stringlist_iget( args , 0 ));
  
  return NULL;
}



void * enkf_main_create_reports_JOB(void * self , const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  ert_report_list_type * report_list = enkf_main_get_report_list( enkf_main );
  
  ert_report_list_create( report_list , enkf_main_get_current_fs( enkf_main ) , true );
  return NULL;
}

void * enkf_main_scale_obs_std_JOB(void * self, const stringlist_type * args ) {
  enkf_main_type   * enkf_main = enkf_main_safe_cast( self );
  
  double scale_factor;
  util_sscanf_double(stringlist_iget(args, 0), &scale_factor);

  if (enkf_main_have_obs(enkf_main)) {
    enkf_obs_type * observations = enkf_main_get_obs(enkf_main);
    enkf_obs_scale_std(observations, scale_factor);
  }
  return NULL;
}

/* 
   Will create the new case if it does not exist. 
*/
void * enkf_main_select_case_JOB( void * self , const stringlist_type * args) {
  enkf_main_type * enkf_main = enkf_main_safe_cast( self );
  const char * new_case = stringlist_iget( args , 0 );
  enkf_main_select_fs( enkf_main , new_case );
  return NULL;
}


void * enkf_main_create_case_JOB( void * self , const stringlist_type * args) {
  enkf_main_type * enkf_main = enkf_main_safe_cast( self );
  const char * new_case = stringlist_iget( args , 0 );
  enkf_fs_type * fs = enkf_main_mount_alt_fs( enkf_main , new_case , false , true );
  enkf_fs_umount(fs);
  return NULL;
}


void * enkf_main_load_results_JOB( void * self , const stringlist_type * args) {
  enkf_main_type * enkf_main = enkf_main_safe_cast( self );
  bool_vector_type * iactive = alloc_iactive_vector_from_range(args, 0, enkf_main_get_ensemble_size(enkf_main));
  int ens_size = enkf_main_get_ensemble_size(enkf_main);
  stringlist_type ** realizations_msg_list = util_calloc(ens_size, sizeof * realizations_msg_list);
  int iens = 0;
  for (; iens < ens_size; ++iens)
      realizations_msg_list[iens] = stringlist_alloc_new();

  enkf_main_load_from_forward_model(enkf_main, iactive, realizations_msg_list);

  for (iens = 0; iens < ens_size; ++iens) {
      stringlist_type * msg = realizations_msg_list[iens];
      if (stringlist_get_size(msg)) {
        int msg_count = 0;
        for (; msg_count < stringlist_get_size(msg); ++msg_count)
          fprintf(stderr, "** Warning: Function %s : Load of realization number %d returned the following warning: %s\n", __func__, iens, stringlist_iget(msg, msg_count));
      }
      stringlist_free(msg);
   }

  free(realizations_msg_list);

  bool_vector_free(iactive);
  return NULL;
}


/*****************************************************************/

static void enkf_main_jobs_export_field(const enkf_main_type * enkf_main, const stringlist_type * args, field_file_format_type file_type) {
  const char *      field            = stringlist_iget(args, 0); 
  const char *      file_name        = stringlist_iget(args, 1); 
  int               report_step      = 0;
  util_sscanf_int(stringlist_iget(args,2), &report_step);
  state_enum        state            = enkf_types_get_state_enum(stringlist_iget(args, 3)); 

  if (BOTH == state) {
      fprintf(stderr,"** Field export jobs only supports state_enum ANALYZED or FORECAST, not BOTH.\n");
      return;
  }

  bool_vector_type * iactive = alloc_iactive_vector_from_range(args, 4, enkf_main_get_ensemble_size(enkf_main));
  enkf_main_export_field(enkf_main,field, file_name, iactive, file_type, report_step, state) ;
  bool_vector_free(iactive);
}



void * enkf_main_export_field_JOB(void * self, const stringlist_type * args) {
  const char * file_name = stringlist_iget(args, 1); 
  field_file_format_type file_type = field_config_default_export_format(file_name); 
  
  if ((RMS_ROFF_FILE == file_type) || (ECL_GRDECL_FILE == file_type)) {
    enkf_main_type * enkf_main = enkf_main_safe_cast( self );
    enkf_main_jobs_export_field(enkf_main, args, file_type);
  } else
    printf("EXPORT_FIELD filename argument: File extension must be either .roff or .grdecl\n"); 
    
  return NULL; 
}

void * enkf_main_export_field_to_RMS_JOB(void * self, const stringlist_type * args) {
  enkf_main_type * enkf_main = enkf_main_safe_cast( self );
  enkf_main_jobs_export_field(enkf_main, args, RMS_ROFF_FILE);
  return NULL; 
}

void * enkf_main_export_field_to_ECL_JOB(void * self, const stringlist_type * args) {
  enkf_main_type * enkf_main = enkf_main_safe_cast( self );
  enkf_main_jobs_export_field(enkf_main, args, ECL_GRDECL_FILE);
  return NULL; 
}


void * enkf_main_rank_on_observations_JOB(void * self, const stringlist_type * args) {
  enkf_main_type * enkf_main  = enkf_main_safe_cast( self );
  const char * ranking_name   = stringlist_iget(args, 0);
  const char * ranking_file   =  stringlist_iget(args, 1);

  bool step_arguments = false;
  int step1           = 0;
  int step2           = enkf_main_get_history_length(enkf_main);

  int pipe_index      = stringlist_find_first(args, "|");
  int first_observation_index = 0;
  int num_args        = stringlist_get_size(args);

  if ((-1 == pipe_index) && (num_args > 2)) {
    step_arguments = true;
  } else if ((2 == pipe_index) &&  (num_args > pipe_index+1)) {
    first_observation_index = pipe_index + 1;
  } else if (4 == pipe_index) {
      step_arguments = true;
      if (num_args > pipe_index+1) {
        first_observation_index = pipe_index + 1;
      }
  }


  if (step_arguments) {
    bool step1_valid = true;
    bool step2_valid = true;
    step1 = stringlist_iget_as_int(args, 2, &step1_valid);
    step2 = stringlist_iget_as_int(args, 3, &step2_valid);

    if (!step1_valid) {
      fprintf(stderr,"** Third argument \"step1\" not recognized as integer value, job not started\n");
      return NULL;
    }

    if (!step2_valid) {
      fprintf(stderr,"** Fourth argument \"step2\" not recognized as integer value, job not started\n");
      return NULL;
    }
  }

  char * obs_key_char = NULL;
  if (first_observation_index > 0)
    obs_key_char = stringlist_alloc_joined_substring( args , first_observation_index , stringlist_get_size(args) , " ");

  enkf_obs_type * enkf_obs = enkf_main_get_obs(enkf_main);
  stringlist_type * obs_ranking_keys = enkf_obs_alloc_matching_keylist( enkf_obs , obs_key_char );

  if ((first_observation_index > 0) && (stringlist_get_size(obs_ranking_keys) == 0)) {
    fprintf(stderr,"The input string : \"%s\" did not resolve to any valid observation keys. Job not started\n", obs_key_char);
    return NULL;
  }

  enkf_main_rank_on_observations(enkf_main, obs_ranking_keys, ranking_name, ranking_file, step1, step2);

  stringlist_free(obs_ranking_keys);

  if (first_observation_index > 0)
    free(obs_key_char);

  return NULL;
}




void * enkf_main_rank_on_data_JOB(void * self, const stringlist_type * args) {
  enkf_main_type * enkf_main = enkf_main_safe_cast( self );
  const char * ranking_name  = stringlist_iget(args, 0);
  const char * ranking_file  = stringlist_iget(args, 1);
  const char * data_key      = stringlist_iget(args, 2);
  bool valid = true;
  bool sort_increasing       = stringlist_iget_as_bool(args, 3, &valid);

  if (!valid) {
    fprintf(stderr,"** Fourth argument \"sort increasing\" not recognized as bool value, job not started\n");
    return NULL;
  }

  int report_step = (stringlist_get_size(args) > 4) ? stringlist_iget_as_int(args, 4, &valid) : enkf_main_get_history_length(enkf_main) ;
  if (!valid) {
    fprintf(stderr,"** Fifth argument \"step\" not recognized as integer value, job not started\n");
    return NULL;
  }

  if (report_step < 0) {
    fprintf(stderr,"** Negative report step, job not started\n");
    return NULL;
  }

  enkf_main_rank_on_data(enkf_main, data_key, sort_increasing, ranking_name, ranking_file, report_step);
  return NULL;
}


