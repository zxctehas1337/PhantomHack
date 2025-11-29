#pragma once
class CMaterial2;

namespace cs2
{
	struct animatable_scene_object;
	struct mesh_transform_cache {
		void* m_mutex{ };
		std::uint64_t m_transform_cache{ };
		MEM_PAD(124);
	};

	struct unk_mesh_cache5 {
		MEM_PAD(232);
	};

	struct render_mesh {
		MEM_PAD(304);
		unk_mesh_cache5* m_unk_mesh_cache5{ };
		MEM_PAD(116);
	};

	struct unk_mesh_cache3 {
		render_mesh* m_render_mesh{ };
		MEM_PAD(40);
	};

	struct model_impl {
		MEM_PAD(8);
		const char* m_path{ };
		MEM_PAD(94);
		unk_mesh_cache3* m_unk_cache3{ };
		MEM_PAD(984);
	};

	struct unk_mesh_cache {
		model_impl* m_model_imp{ };
		MEM_PAD(32);
	};

	struct mesh_instance {
		MEM_PAD(8);
		unk_mesh_cache* m_unk_cache_ptr{ };
		MEM_PAD(32);
		animatable_scene_object* m_animatable_scene_object{ };
		MEM_PAD(16);
		mesh_transform_cache* m_transforms_cache{ };
		MEM_PAD(16);
	};

	struct animatable_scene_object {

		OFFSET(int, get_bone_count, 0xD0);
		OFFSET(Matrix3x4_t*, get_render_bones, 0xD8);

		MEM_PAD(16);
		mesh_instance* m_mesh_instance{ };
		CAnimatableSceneObjectDesc* m_desc{ };
		MEM_PAD(16);
		Matrix3x4_t m_translation_matrix{ };
		MEM_PAD(88);
		CHandle<C_BaseEntity> m_owner_handle{ };
		MEM_PAD(80);
		CSkeletonInstance* m_skeleton{ };
		MEM_PAD(24);

	};

	struct draw_mesh {
		mesh_instance* m_mesh_instance{ };
		MEM_PAD(16);
		animatable_scene_object* m_animatable_scene_object{ };
		CMaterial2* m_material{ };
		MEM_PAD(40);
		Color m_color{ };
		MEM_PAD(4);
		void* m_object_info{ };
		MEM_PAD(8);
	};
} 

enum EBacktrackChamType
{
	BACKTRACK_CHAM_NONE = 0,
	BACKTRACK_CHAM_LAST = 1,
	BACKTRACK_CHAM_ALL = 2,
	BACKTRACK_CHAM_RANGE = 3 
};

class CChams
{
public:
	struct ChamsMaterials_t
	{
		CStrongHandle<CMaterial2> m_Occluded;
		CStrongHandle<CMaterial2> m_Visible;
	}; ChamsMaterials_t m_Materials[5];

	std::unordered_map< cs2::mesh_instance*, std::array<cs2::mesh_instance*, 20> > m_mapMeshInstances{ };
	std::shared_mutex m_MeshInstancesMutex{ };

	std::unordered_map<C_CSPlayerPawn*, std::array<float, 20> > m_mapOnShotChams;

private:
	bool m_bInit = false;
public:
	bool Init();

	CStrongHandle<CMaterial2> CreateMaterial(const char* szMaterialName, const char szVmatBuffer[]);

	void CleanupOldModels();

	bool OnGeneratePrimitives( CAnimatableSceneObjectDesc* pDesc, CSceneAnimatableObject* pObject, void* a3, CMeshPrimitiveOutputBuffer* pRenderBuffer ) const;
};
inline const auto g_Chams = std::make_unique<CChams>();