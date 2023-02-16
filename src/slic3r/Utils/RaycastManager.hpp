#ifndef slic3r_RaycastManager_hpp_
#define slic3r_RaycastManager_hpp_

#include <memory> // unique_ptr
#include <optional>
#include "libslic3r/AABBMesh.hpp" // Structure to cast rays
#include "libslic3r/Point.hpp" // Transform3d
#include "libslic3r/ObjectID.hpp"
#include "libslic3r/Model.hpp" // ModelObjectPtrs, ModelObject, ModelInstance, ModelVolume
#include "slic3r/GUI/Camera.hpp"

namespace Slic3r::GUI{

/// <summary>
/// Cast rays from camera to scene
/// Used for find hit point on model volume under mouse cursor
/// </summary>
class RaycastManager
{
// Public structures used by RaycastManager
public: 

    //                   ModelVolume.id
    using Mesh = std::pair<size_t, std::unique_ptr<AABBMesh> >;
    using Meshes = std::vector<Mesh>;
    
    // Key for transformation consist of unique volume and instance id ... ObjectId()
    //                 ModelInstance, ModelVolume
    using TrKey = std::pair<size_t, size_t>;
    using TrItem = std::pair<TrKey, Transform3d>;
    using TrItems = std::vector<TrItem>;

    /// <summary>
    /// Interface for identify allowed volumes to cast rays.
    /// </summary>
    class ISkip{
    public:
        virtual ~ISkip() = default;

        /// <summary>
        /// Condition to not process model volume
        /// </summary>
        /// <param name="model_volume_id">ObjectID of model volume to not process</param>
        /// <returns>True on skip otherwise false</returns>
        virtual bool skip(const size_t &model_volume_id) const { return false; }
    };

    // TODO: it is more general object move outside of this class
    template<typename T> 
    struct SurfacePoint {
        using Vec3 = Eigen::Matrix<T, 3, 1, Eigen::DontAlign>;
        Vec3 position = Vec3::Zero();
        Vec3 normal   = Vec3::UnitZ();
    };

    struct Hit : public SurfacePoint<double>
    {
        TrKey tr_key;
        double squared_distance;
    };    

    struct ClosePoint
    {
        TrKey tr_key;
        Vec3d  point;
        double squared_distance;
    };

// Members
private:
    // Keep structure to fast cast rays
    // meshes are sorted by volume_id for faster search
    Meshes m_meshes;

    // Keep transformation of meshes
    TrItems m_transformations;
    // Note: one mesh could have more transformations ... instances

public:

    /// <summary>
    /// Actualize raycasters + transformation
    /// Detection of removed object
    /// Detection of removed instance
    /// Detection of removed volume
    /// </summary>
    /// <param name="object">Model representation</param>
    /// <param name="skip">Condifiton for skip actualization</param>
    void actualize(const ModelObject *object, const ISkip *skip = nullptr);
    void actualize(const ModelInstance *instance, const ISkip *skip = nullptr);

    class SkipVolume: public ISkip
    {
        size_t volume_id;
    public:
        SkipVolume(size_t volume_id) : volume_id(volume_id) {}
        bool skip(const size_t &model_volume_id) const override { return model_volume_id == volume_id; }
    };

    class AllowVolumes: public ISkip
    {
        std::vector<size_t> allowed_id;
    public:
        AllowVolumes(std::vector<size_t> allowed_id) : allowed_id(allowed_id) {}
        bool skip(const size_t &model_volume_id) const override {
            auto it = std::find(allowed_id.begin(), allowed_id.end(), model_volume_id);
            return it == allowed_id.end(); 
        }
    };

    /// <summary>
    /// Unproject on mesh by Mesh raycasters
    /// </summary>
    /// <param name="mouse_pos">Position of mouse on screen</param>
    /// <param name="camera">Projection params</param>
    /// <param name="skip">Define which caster will be skipped, null mean no skip</param>
    /// <returns>Position on surface, normal direction and transformation key, which define hitted object instance</returns>
    std::optional<Hit> unproject(const Vec2d &mouse_pos, 
                                 const Camera &camera,
                                 const ISkip *skip = nullptr) const;

    /// <summary>
    /// Unproject Ray(point direction) on mesh by MeshRaycasters
    /// NOTE: It inspect also oposit direction of ray !!
    /// </summary>
    /// <param name="point">Start point for ray</param>
    /// <param name="direction">Direction of ray</param>
    /// <param name="skip">Define which caster will be skipped, null mean no skip</param>
    /// <returns>Position on surface, normal direction and transformation key, which define hitted object instance</returns>
    std::optional<Hit> unproject(const Vec3d &point, const Vec3d &direction, const ISkip *skip = nullptr) const;

    /// <summary>
    /// Search of closest point
    /// </summary>
    /// <param name="point">Point</param>
    /// <param name="skip">Define which caster will be skipped, null mean no skip</param>
    /// <returns></returns>
    std::optional<ClosePoint> closest(const Vec3d &point, const ISkip *skip = nullptr) const;

    /// <summary>
    /// Getter on transformation from hitted volume to world
    /// </summary>
    /// <param name="tr_key">Define transformation</param>
    /// <returns>Transformation for key</returns>
    Transform3d get_transformation(const TrKey &tr_key) const;
};

} // namespace Slic3r::GUI

#endif // slic3r_RaycastManager_hpp_
