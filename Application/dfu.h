#pragma once

#include "cl_common.h"

void JumpToBootloader(void);
void JumpToApplication(void);

CL_Result_t EraseAppFlash(void);
CL_Result_t EraseBakFlash(void);
CL_Result_t WriteFlash(uint32_t addr, const uint8_t *buff, uint32_t length);
CL_Result_t LoadAppInfo(void);
CL_Result_t SaveAppInfo(uint32_t size, uint32_t hash);
bool IsAppValid(void);
bool IsOtaBakValid(void);
CL_Result_t CopyOtaBakToApp(void);
CL_Result_t EraseFlash(uint32_t addr, uint32_t pages);

