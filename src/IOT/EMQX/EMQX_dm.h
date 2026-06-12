#pragma once

#include "cJSON.h"

cJSON* EMQX_Data_Upload();
void EMQX_Cmd_Handle(cJSON* root);
void EMQX_Post_Relay();