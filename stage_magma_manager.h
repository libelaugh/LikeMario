/*==============================================================================

　　  ステージmagma管理[stage_magma_manager.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_MAGMA_MANAGER_H
#define STAGE_MAGMA_MANAGER_H

void StageMagmaManager_Initialize();
void StageMagmaManager_Finalize();
void StageMagmaManager_Update(double elapsedTime);
void StageMagmaManager_Draw();

#endif//STAGE_MAGMA_MANAGER_H