#pragma once

constexpr int nBoneMatrixMemorySize = sizeof(BoneData_t) * 128;
constexpr float flLagCompensationConeCosine = 0.707107f;    // 45 degree angle


struct TimeStamp_t {


    int m_iTick{ };
    float m_flFrac{ };

    TimeStamp_t(int tick, const float frac) : m_iTick(tick), m_flFrac(frac) {
        if (m_flFrac < 1.f) {
            if (m_flFrac <= 0.f)
                m_flFrac = 0.f;
        }
        else {
            m_iTick++;
            m_flFrac = 0.f;
        }
    }

    TimeStamp_t(float time) {
        auto temp = std::fmodf(time, 0.015625f);
        m_flFrac = temp * 64;
        m_iTick = TIME_TO_TICKS(time - temp);
        Normalize();
    }

    void Normalize() {
        if (m_flFrac < 1.f) {
            if (m_flFrac <= 0.f)
                m_flFrac = 0.f;
        }
        else {
            m_iTick++;
            m_flFrac = 0.f;
        }
    }

    TimeStamp_t operator-(const TimeStamp_t& other) const {
        const auto frac_temp = m_flFrac - other.m_flFrac;
        auto frac = frac_temp;
        if (frac_temp < 0.f) {
            frac += 1.f;
        }
        auto tick = m_iTick - other.m_iTick - 1;
        if (frac_temp >= 0.f)
            tick = m_iTick - other.m_iTick;
        return { tick, frac };
    }

    TimeStamp_t operator+(const TimeStamp_t& other) const {
        const auto frac_temp = m_flFrac + other.m_flFrac;
        auto frac = frac_temp;
        if (frac_temp > 1.f) {
            frac -= 1.f;
        }
        auto tick = m_iTick + other.m_iTick + 1;
        if (frac_temp <= 1.f)
            tick = m_iTick + other.m_iTick;
        return { tick, frac };
    }

    bool operator<(const TimeStamp_t& other) const {
        if (m_iTick > other.m_iTick)
            return false;
        if (m_iTick >= other.m_iTick)
            return other.m_flFrac > m_flFrac;
        return false;
    }

    float ToTime() const {
        return static_cast<float>(m_iTick) * 0.015625f + m_flFrac * 0.015625;
    }
};


class CLagRecord
{
public:

    C_CSPlayerPawn* m_pPawn = nullptr;
    CGameSceneNode* m_pGameSceneNode = nullptr;
    CSkeletonInstance* m_pSkeletonInstance = nullptr;
    CModel* m_pModel = nullptr;
    CCollisionProperty* m_pCollisionProperty = nullptr;

    QAngle m_aRotation = {};

    BoneData_t m_Bones[128];

    Vector m_vCollisionMins = {};
    Vector m_vCollisionMaxs = {};

    Vector m_vOrigin = {};

    float m_flSimulationTime = 0.0f;
    int m_nTick = 0;

    std::vector<HitboxData_t> m_arrTargetHitboxes = {};

    bool Setup(C_CSPlayerPawn* pPawn);

    template<typename Func>
    auto WithAppliedState(Func&& func) -> decltype(func())
    {
        struct BackupData_t {
            BoneData_t m_Bones[128];
            Vector m_vCollisionMins;
            Vector m_vCollisionMaxs;

            Vector m_vAbsOrigin; // interpolated bone origin
            Vector m_vOrigin;    // non-interpolated bone origin

            QAngle m_qAbsRotation; // idek what this shit is tbh ( interpolated ig )
            QAngle m_qRotation; // idek what this shit is tbh
        };

        BackupData_t backup = {}; // initiator essentially automatically calls memset ( all bytes 0 )

        // first lets backup the players 'current' state 

        memcpy(backup.m_Bones, this->m_pSkeletonInstance->m_pBoneCache, nBoneMatrixMemorySize); // since we have a *statically defined arr size at 128, its possible if bonecount from skeleton is higher we crash / stack corrupt

        backup.m_vCollisionMins = this->m_pCollisionProperty->m_vecMins();
        backup.m_vCollisionMaxs = this->m_pCollisionProperty->m_vecMaxs();

        backup.m_vAbsOrigin = this->m_pGameSceneNode->m_vecAbsOrigin();
        backup.m_vOrigin = this->m_pGameSceneNode->m_vecOrigin().m_vecValue;

        backup.m_qAbsRotation = this->m_pGameSceneNode->m_angAbsRotation();
        backup.m_qRotation = this->m_pGameSceneNode->m_angRotation();

        // Now lets write the wish data to the player for tracing to succeed

        memcpy(this->m_pSkeletonInstance->m_pBoneCache, this->m_Bones, nBoneMatrixMemorySize);

        this->m_pCollisionProperty->m_vecMins() = this->m_vCollisionMins;
        this->m_pCollisionProperty->m_vecMaxs() = this->m_vCollisionMaxs;

        this->m_pGameSceneNode->m_vecAbsOrigin() = this->m_vOrigin;            // set both to uninterpolated bone origin
        this->m_pGameSceneNode->m_vecOrigin().m_vecValue = this->m_vOrigin;    // set both to uninterpolated bone origin


        this->m_pGameSceneNode->m_angAbsRotation() = this->m_aRotation;
        this->m_pGameSceneNode->m_angRotation() = this->m_aRotation;

        auto ret = func(); // run whatever code we need to under this applied state and capture its return ig

        // we are done with the tracing job, we can restore the players backed up state

        memcpy(this->m_pSkeletonInstance->m_pBoneCache, backup.m_Bones, nBoneMatrixMemorySize);

        this->m_pCollisionProperty->m_vecMins() = backup.m_vCollisionMins;
        this->m_pCollisionProperty->m_vecMaxs() = backup.m_vCollisionMaxs;

        this->m_pGameSceneNode->m_vecAbsOrigin() = backup.m_vAbsOrigin;
        this->m_pGameSceneNode->m_vecOrigin().m_vecValue = backup.m_vOrigin;

        this->m_pGameSceneNode->m_angAbsRotation() = backup.m_qAbsRotation;
        this->m_pGameSceneNode->m_angRotation() = backup.m_qRotation;


        return ret;
    }

    bool StoreHitboxes();

    bool WantsLagCompensationOnEntity(Vector vLocalOrigin, QAngle angView);
};

class CRecordTrack
{
public:

    C_CSPlayerPawn* m_pPawn = nullptr;
    std::deque<CLagRecord> m_deqRecords = {};
    bool m_bRageMatters = false;

    void Reset()
    {
        this->m_pPawn = nullptr;
        this->m_bRageMatters = false;
        this->m_deqRecords.clear();
    }

    bool CheckAndSetPawn(C_CSPlayerPawn* pPawn)
    {
        if (!this->m_pPawn)
            return this->m_pPawn = pPawn;

        if (this->m_pPawn != pPawn) {
            this->Reset();
            return false;
        }

        this->m_pPawn = pPawn;
        return true;
    }

    CLagRecord* GetFirstRecord()
    {
        if (m_deqRecords.empty())
            return nullptr;

        CLagRecord* pLagRecord = &m_deqRecords.front();
        if (pLagRecord == nullptr)
            return nullptr;

        return pLagRecord;
    }

    CLagRecord* GetLastRecord()
    {
        if (m_deqRecords.empty())
            return nullptr;
        CLagRecord* pLagRecord = &m_deqRecords.back();
        if (pLagRecord == nullptr)
            return nullptr;
        return pLagRecord;
    }

    void Update(CCSPlayerController* pController);

};

class CLagCompensation
{
public:

    void Run();

    TimeStamp_t m_InterpolationTiming = { 0,0.f };
    TimeStamp_t m_UnknownTiming = { 0,0.f };

    int m_nOldestValidTick = -1;
    float m_flOldestValidTime = -1.f;

};

inline auto g_LagCompensation = std::make_unique<CLagCompensation>();