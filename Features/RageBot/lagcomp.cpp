#include "../../Precompiled.h"
#include "../threading/Threader.h"

bool CLagRecord::StoreHitboxes() {
    if (!LocalPlayerData::m_pWeaponBaseVData)
        return false;

    m_arrTargetHitboxes.clear();

    if (g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes.empty())
        return false;

    //printf("Storing hitboxes for record at tick %d\n", m_nTick);

    if (m_pModel->m_nRendermeshCount <= 0 || !m_pModel->m_pRenderMeshes)
        return false;

    //printf("Passed model checks\n");

    CRenderMesh* pMeshes = m_pModel->m_pRenderMeshes->m_pMeshes;
    if (!pMeshes)
        return false;


    CHitBoxSets* pHitBoxSets = pMeshes[0].m_pHitboxSets;
    //  printf("Passed mesh checks %llx %llx %llx %i\n", pHitBoxSets[0].m_HitBoxes, pHitBoxSets[0], pHitBoxSets[0].m_pHitBox, pHitBoxSets[0].m_nHitboxCount);

    if (!pHitBoxSets || pHitBoxSets[0].m_nHitboxCount <= 0)
        return false;

    // printf("Passed hitbox set checks\n");

    CHitBox* pHitBoxArr = pHitBoxSets[0].m_pHitBox;
    if (!pHitBoxArr)
        return false;

    //printf("Passed hitbox array checks\n");

    int nDamageThreshold = NRagebot::m_bHoldingMinOverride
        ? g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamageOverride
        : ((g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage > 100)
            ? (g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage - 100 + m_pPawn->m_iHealth())
            : g_Variables.m_Ragebot.m_ActiveConfig.m_iMinDamage);

    bool bStoredHitbox = false;

    for (const auto& hitbox : g_Variables.m_Ragebot.m_ActiveConfig.m_Hitboxes) {
        int damage = LocalPlayerData::m_pWeaponBaseVData->m_nDamage();

        switch (hitbox.m_eDefinition) {
        case HB_HEAD:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 1, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_HEAD].ConstructHitboxData(m_Bones[6], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            break;

        case HB_CHEST:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 2, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_UPPER_CHEST].ConstructHitboxData(
                        m_Bones[HitboxToBoneIndex(HITBOX_UPPER_CHEST)], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            break;

        case HB_STOMACH:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 3, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_STOMACH].ConstructHitboxData(m_Bones[1], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            break;
        case HB_ARMS:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 4, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_LEFT_UPPER_ARM].ConstructHitboxData(m_Bones[59], hitbox.m_bMultipoint));
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_LEFT_FOREARM].ConstructHitboxData(m_Bones[56], hitbox.m_bMultipoint));
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_LEFT_HAND].ConstructHitboxData(m_Bones[123], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 5, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_RIGHT_UPPER_ARM].ConstructHitboxData(m_Bones[84], hitbox.m_bMultipoint));
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_RIGHT_FOREARM].ConstructHitboxData(m_Bones[81], hitbox.m_bMultipoint));
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_RIGHT_HAND].ConstructHitboxData(m_Bones[73], hitbox.m_bMultipoint));

                bStoredHitbox = true;
            }
            break;
        case HB_PELVIS:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 3, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_PELVIS].ConstructHitboxData(m_Bones[0], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            break;

        case HB_LEGS:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 6, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_LEFT_CALF].ConstructHitboxData(m_Bones[23], hitbox.m_bMultipoint));
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_RIGHT_CALF].ConstructHitboxData(m_Bones[26], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            break;

        case HB_FEET:
            if (nDamageThreshold <= g_Autowall->MaxPossibleDamageToHitbox(m_pPawn, 6, damage)) {
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_LEFT_FOOT].ConstructHitboxData(m_Bones[24], hitbox.m_bMultipoint));
                m_arrTargetHitboxes.emplace_back(
                    pHitBoxArr[HITBOX_RIGHT_FOOT].ConstructHitboxData(m_Bones[27], hitbox.m_bMultipoint));
                bStoredHitbox = true;
            }
            break;
        }
    }

    return bStoredHitbox;
}

bool CLagRecord::Setup(C_CSPlayerPawn* pPawn) {

    m_pPawn = pPawn;

    if (!(m_pGameSceneNode = pPawn->m_pGameSceneNode()))
        return false;

    if (!(m_pSkeletonInstance = m_pGameSceneNode->GetSkeletonInstance()))
        return false;

    if (!(m_pModel = m_pSkeletonInstance->m_modelState().m_modelHandle))
        return false;

    if (!(m_pCollisionProperty = m_pPawn->m_pCollision()))
        return false;

    Vector vAbsOrigin = this->m_pGameSceneNode->m_vecAbsOrigin();
    QAngle qAbsRotation = this->m_pGameSceneNode->m_angAbsRotation();

    this->m_vCollisionMins = this->m_pCollisionProperty->m_vecMins();
    this->m_vCollisionMaxs = this->m_pCollisionProperty->m_vecMaxs();

    this->m_aRotation = m_pGameSceneNode->m_angRotation();
    this->m_pGameSceneNode->m_angAbsRotation() = this->m_aRotation;

    this->m_vOrigin = m_pGameSceneNode->m_vecOrigin().m_vecValue;
    this->m_pGameSceneNode->m_vecAbsOrigin() = this->m_vOrigin;

    m_flSimulationTime = pPawn->m_flSimulationTime();
    m_nTick = TIME_TO_TICKS(m_flSimulationTime);

    int nBackupSimTick = pPawn->m_nSimulationTick();

    pPawn->m_nSimulationTick() = Interfaces::m_pGlobalVariables->m_nTickCount;
    pPawn->m_flSimulationTime() = Interfaces::m_pGlobalVariables->m_flCurrentTime;

    m_pSkeletonInstance->CalculateWorldSpaceBones(FLAG_ALL_BONE_FLAGS);
    m_pSkeletonInstance->CalculateAnimationState(FLAG_HITBOX);

    pPawn->m_nSimulationTick() = nBackupSimTick;
    pPawn->m_flSimulationTime() = m_flSimulationTime;

    memcpy(this->m_Bones, m_pSkeletonInstance->m_pBoneCache, nBoneMatrixMemorySize);

    this->m_pGameSceneNode->m_vecAbsOrigin() = vAbsOrigin; // restore previous interpolated things
    this->m_pGameSceneNode->m_angAbsRotation() = qAbsRotation;

    return true;
}

bool CLagRecord::WantsLagCompensationOnEntity(Vector vLocalOrigin, QAngle angView) {

    Vector vDiff = this->m_vOrigin - vLocalOrigin;
    vDiff.NormalizeInPlace();

    Vector vForward, vL, vU;
    angView.ToDirections(&vForward, &vL, &vU);

    if (vForward.DotProduct(vDiff) < flLagCompensationConeCosine)
        return false;

    return true;
}

void CRecordTrack::Update(CCSPlayerController* pController) {
    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(Interfaces::m_pGameResourceService->pGameEntitySystem->Get(pController->m_hPawn()));
    if (!pPawn || !Globals::m_pLocalPlayerPawn->IsEnemy(pPawn) || pPawn->m_bGunGameImmunity() || pPawn->m_iHealth() <= 0)
        return this->Reset();

    if (!this->CheckAndSetPawn(pPawn))
        return;

    bool bAddNewRecord = false;
    if (this->m_deqRecords.empty())
        bAddNewRecord = true;
    else
        bAddNewRecord = TIME_TO_TICKS(pPawn->m_flSimulationTime()) > this->GetFirstRecord()->m_nTick;

    if (bAddNewRecord)
    {
        auto& newRecord = this->m_deqRecords.emplace_front();
        if (!newRecord.Setup(pPawn))
            return this->Reset();

        this->m_bRageMatters = newRecord.StoreHitboxes();
        //  printf("Stored hitboxes: %d\n", this->m_bRageMatters);
    }

    while (!this->m_deqRecords.empty())
    {
        auto& backRecord = this->m_deqRecords.back();

        if (backRecord.m_nTick < g_LagCompensation->m_nOldestValidTick && this->m_deqRecords.size())
        {
            this->m_deqRecords.pop_back();
        }
        else
            break;
    }
}

void CLagCompensation::Run()
{
    auto pLocal = Globals::m_pLocalPlayerPawn;
    if (!pLocal)
        return;

    float flInterpolationTime = pLocal->GetSomeTiming(0, 1);
    this->m_InterpolationTiming = TimeStamp_t(flInterpolationTime);

    float flUnkTime = pLocal->GetSomeTiming(1, 1);
    this->m_UnknownTiming = TimeStamp_t(flUnkTime);

    auto pNetChannel = Interfaces::m_pEngine->GetNetChannelInfo();
    if (!pNetChannel)
        return;

    CONVAR(sv_maxunlag);

    float flClientRoundedMaxUnlag = sv_maxunlag->GetFloat() * 0.9499999f;
    float flLatency = (pNetChannel->GetNetLatency() / 2.f) + this->m_InterpolationTiming.ToTime();
    flClientRoundedMaxUnlag -= flLatency;
    flClientRoundedMaxUnlag = std::clamp(flClientRoundedMaxUnlag, 0.f, sv_maxunlag->GetFloat());
    this->m_flOldestValidTime = Interfaces::m_pGlobalVariables->m_flCurrentTime - flClientRoundedMaxUnlag;
    this->m_nOldestValidTick = TIME_TO_TICKS(this->m_flOldestValidTime);

    for (EntityObject_t& object : g_Entities->m_vecPlayersOnly)
    {
        CRecordTrack* pRecordTrack = object.GetRecordTrack();
        CCSPlayerController* pController = static_cast<CCSPlayerController*>(object.m_pEntity);

        g_Threader->EnqueueJob([=]() {
            pRecordTrack->Update(pController);
            });
    }

    g_Threader->WaitUntilFinished();
}