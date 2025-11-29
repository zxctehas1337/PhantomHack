//#include "../../Precompiled.h"
// 
//int Doubletap::GetPlayerTick()
//{
//    if (!Globals::m_pLocalPlayerPawn || !Globals::m_pLocalPlayerPawn->m_pWeaponServices())
//        return 0;
//
//    C_CSWeaponBase* pWeapon = Globals::m_pLocalPlayerPawn->m_pWeaponServices()->m_hActiveWeapon().Get();
//    if (!pWeapon)
//        return 0;
//
//    double intPart;
//    float fractionalPart = static_cast<float>(modf(pWeapon->m_flWatTickOffset(), &intPart));
//
//    int iPlayerTick = pWeapon->m_nNextPrimaryAttackTick();
//
//    if (fractionalPart >= 0.5f)
//        iPlayerTick += 1;
//    else if (fractionalPart < -0.5f)
//        iPlayerTick -= 1;
//
//    return iPlayerTick;
//}
//
//void Doubletap::SetHistoryTick(int iHistoryTick) {
//	for (int i = 0; i < Globals::m_pCmd->m_csgoUserCmd.input_history_size(); i++) {
//		CSGOInputHistoryEntryPB* input_history = Globals::m_pCmd->m_csgoUserCmd.mutable_input_history(i);
//		if (!input_history)
//			continue;
//
//		input_history->set_player_tick_count(iHistoryTick);
//		input_history->set_player_tick_fraction(0.f);
//	}
//}
//
//void Doubletap::Process() {
//    static int iShotCount{};
//    
//    int iPlayerTick = GetPlayerTick();
//    bool bSetPlayerTick = iShotCount % 2;;
//
//    if (Globals::m_pCmd->m_csgoUserCmd.attack1_start_history_index() > -1)
//        iShotCount++;
//
//    SetHistoryTick(bSetPlayerTick ? iPlayerTick - 1 : 0);
//
//    Globals::m_pCmd->m_csgoUserCmd.set_attack1_start_history_index(-1);
//}
//
//void Doubletap::SetupInputHistory() {
//    if (!Globals::m_pLocalPlayerPawn || Globals::m_pLocalPlayerPawn->m_iHealth() <= 0 || Globals::m_pCmd->m_csgoUserCmd.input_history_size() < 0)
//        return;
//
//    int asd = Globals::m_pCmd->m_csgoUserCmd.input_history_size();
//
//    auto input_history = Globals::m_pCmd->m_csgoUserCmd.mutable_input_history(0);
//
//    CClientNetworkData info;
//    Interfaces::m_pEngine->GetClientNetworkData(&info);
//
//    auto tickbase = Globals::m_pLocalPlayerController->m_nTickBase();
//
//    if (Globals::m_pBaseCmd->has_viewangles()) {
//        input_history->mutable_view_angles()->set_x(Globals::m_pBaseCmd->viewangles().x());
//        input_history->mutable_view_angles()->set_y(Globals::m_pBaseCmd->viewangles().y());
//        input_history->mutable_view_angles()->set_z(Globals::m_pBaseCmd->viewangles().z());
//    }
//
//    if (info.m_pLocalData) {
//        if (!input_history->has_shoot_position()) {
//            CMsgVector* shoot_pos = new CMsgVector;
//            shoot_pos->set_x(info.m_pLocalData->m_vecShootPostion.x);
//            shoot_pos->set_y(info.m_pLocalData->m_vecShootPostion.y);
//            shoot_pos->set_z(info.m_pLocalData->m_vecShootPostion.z);
//
//            input_history->set_allocated_shoot_position(shoot_pos);
//        }
//        else {
//            CMsgVector* shoot_pos = input_history->mutable_shoot_position();
//            shoot_pos->set_x(info.m_pLocalData->m_vecShootPostion.x);
//            shoot_pos->set_y(info.m_pLocalData->m_vecShootPostion.y);
//            shoot_pos->set_z(info.m_pLocalData->m_vecShootPostion.z);
//        }
//    }
//
//    input_history->set_render_tick_count(info.m_nRenderTickCount);
//    input_history->set_render_tick_fraction(info.m_flRenderTickFraction);
//
//    input_history->set_player_tick_count(tickbase);
//    input_history->set_player_tick_fraction(Interfaces::m_pGlobalVariables->m_flTickFraction);
//}