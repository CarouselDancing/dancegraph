
#include <string>
#include <sstream>

#include "zed_common.h"

namespace zed
{

    vec3 vec3::operator + (const vec3& v) {
        return vec3{ x + v.x, y + v.y, z + v.z };
    }


    vec3 vec3::operator - (const vec3& v) {
        return vec3{ x - v.x, y - v.y, z - v.z };
    }


    vec3 operator - (const vec3& a, const  vec3& b) {
        return vec3{ a.x - b.x, a.y - b.y, a.z - b.z };

    }

    const std::string vec3::str() const {
        std::stringstream ss;
        ss << std::setprecision(6) << x << " " << y << " " << z;
        return ss.str();
    }

    vec3 vec3::scale(float s) {
        return vec3{ s * x, s * y, s * z };
    }


    const quat vec3::embiggen() const {
        return quat{ x, y, z, sqrt(1.0f - x * x - y * y - z * z) };
    }



    const vec3 quat::toEuler() const {
        // Euler rotation values in a format suitable for BVH export
        float heading = atan2(2 * (w * y + x * z), 1 - 2 * (y * y + z * z));

        // Clamp needed in case floating point error tips the value out of bounds of asin
        float attitude = asin(std::clamp(2 * (w * z - x * y), -1.0f, 1.0f));
        float bank = atan2(2 * (w * x + y * z), 1 - 2 * (z * z + x * x));
        return vec3{ attitude * RAD2DEG, heading * RAD2DEG, bank * RAD2DEG };
    };

    const std::string quat::str() const {
        std::stringstream ss;
        ss << std::setprecision(6) << x << " " << y << " " << z << " " << w;
        return ss.str();
    }


    const vec3 quat::ensmallen() const {
        // Typically the 'w' coordinate is the biggest, so lets ditch that one to keep precision
        if (w > 0)
            return vec3{ x, y, z };
        else
            return vec3{ -x, -y, -z };
    }


    // Note that the orientation is stored as a quaternion but string output is in Euler angles
    const std::string xform::str() const {
        std::stringstream ss;
        ss << pos.str() << " " << ori.toEuler().str();
        return ss.str();
    }

    xform xform::scale(float s) {
        return xform{ pos.scale(s), ori };
    }

    xform_compact xform::ensmallen() const {
        return xform_compact{ pos, ori.ensmallen() };
    }


    // Note that the orientation is stored as a quaternion but string output is in Euler angles
    const std::string xform_compact::str() const {
        std::stringstream ss;
        ss << pos.str() << " " << ori.embiggen().toEuler().str();
        return ss.str();
    }

    xform_compact xform_compact::scale(float s) {
        return xform_compact{ pos.scale(s), ori };
    }

    xform xform_compact::embiggen() const {
        return xform{ pos, ori.embiggen() };
    }


    const quat quant_quat::unquantize() const {
        float qx, qy, qz, qw;
        qx = ((float)x) / 32767.0;
        qy = ((float)y) / 32767.0;
        qz = ((float)z) / 32767.0;

        float qws = 1.0f - qx * qx - qy * qy - qz * qz;
        if (qws < 0.0)
            qws = 0.0;
        qw = sqrt(qws);
        return quat{ qx, qy, qz, qw };
    }

    const std::string quant_quat::str() const {
        return unquantize().str();
    }



    const quant_quat quat::quantize() const {
        int16_t qx, qy, qz;
        if (w >= 0) {
            qx = (int16_t)(32767 * x);
            qy = (int16_t)(32767 * y);
            qz = (int16_t)(32767 * z);
        }
        else {
            qx = (int16_t)-(32767 * x);
            qy = (int16_t)-(32767 * y);
            qz = (int16_t)-(32767 * z);
        }

        return quant_quat{ qx, qy, qz };
    }

    const std::string xform_quant::str() const {
        std::stringstream ss;
        ss << pos.str() << " " << ori.unquantize().str();
        return ss.str();

    }
    xform_quant xform_quant::scale(float s) {
        return xform_quant{ pos.scale(s), ori };
    }

    const xform xform_quant::embiggen() const {
        return xform{ pos, ori.unquantize() };
    }

    vec3 operator + (const vec3& a, const vec3& b) {
        return vec3{ a.x + b.x, a.y + b.y, a.z + b.z };
    }

    vec3 operator * (float f, const vec3& a) {
        return vec3{ f * a.x , f * a.y, f * a.z };
    }


    quat operator + (const quat & a, const quat & b) {
        return (quat{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w });
    }

    quat operator - (const quat &a, const quat &b) {
        return (quat{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w });
    }

    quat operator * (float f, const quat& a) {
        return (quat{ f * a.x, f * a.y, f * a.z, f * a.w });
    }

    float dot(const vec3& a, const vec3& b) {
        return (a.x * b.x + a.y * b.y + a.z * b.z);
    }

    float dot(const quat& a, const quat& b) {
        return (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
    }

    quat operator * (const quat& a, const quat& b) {
        float nw = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;
        float nx = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
        float ny = -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y;
        float nz = a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
        return quat{ nx, ny, nz, nw };
    }


    quant_quat operator + (const quant_quat& a, const quant_quat& b) {
        quat c = (a.unquantize() + b.unquantize());
        return c.quantize();
    }

    quant_quat operator - (const quant_quat& a, const quant_quat& b) {
        quat c = (a.unquantize() - b.unquantize());
        return c.quantize();
    }

    vec3 operator *(const quat& q, const vec3 v) {

        float nx = 2 * (q.w * v.z * q.y + q.x * v.z * q.z - q.w * v.y * q.z + q.x * v.y * q.y) + v.x * (q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
        float ny = 2 * (q.w * v.x * q.z + q.x * v.x * q.y - q.w * v.z * q.x + q.y * v.z * q.z) + v.y * (q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z);
        float nz = 2 * (q.w * v.y * q.x - q.w * v.x * q.y + q.x * v.x * q.z + q.y * v.y * q.z) + v.z * (q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);

        return vec3{ nx, ny, nz };
    }

    quat quat::inv() const {
        return quat{ -x, -y, -z, w };
    }


    vec3 lerp(const vec3& a, const vec3& b, float t) {
        vec3 c = b - a;
        return a + t * c;
    }

    quat slerp(const quat& a, const  quat& b, float t) {
        // Spherical linear interpolation of two quaternions
        // Ken Shoemake, Animating Rotation with Quaternion Curves, Siggraph 1985

        // Slerp(a, b, t) = sin (1-t)*theta / sin(theta) * a + sin (t * theta) / sin(theta) * b
        // where cos(theta) = a . b

        float theta = acos(dot(a, b));
        float st = sin(theta);
        if (st == 0.0) {
            return a;
        }

        float p0 = sin((1 - t) * theta) / st;
        float p1 = sin(t * theta) / st;
        quat c = (p0 * a + p1 * b);

        return c;
    }


    quant_quat slerp(const quant_quat& a, const quant_quat& b, float t) {
        quat qa = a.unquantize();
        quat qb = b.unquantize();
        auto c = slerp(qa, qb, t);
        return c.quantize();
    }

    
    int BufferProperties::signalSize() {
        int sigSize;
        switch (bodySignalType) {

        case zed::Zed4SignalType::Body_38_Compact:
            sigSize = ZedBodies<ZedSkeletonCompact_38>::size(maxBodyCount); break;
        case zed::Zed4SignalType::Body_34_Full:
            sigSize = ZedBodies<ZedSkeletonFull_34>::size(maxBodyCount); break;
        case zed::Zed4SignalType::Body_38_Full:
            sigSize = ZedBodies<ZedSkeletonFull_38>::size(maxBodyCount); break;
        case zed::Zed4SignalType::Body_38_KeypointsPlus:
            sigSize = ZedBodies<ZedSkeletonKPRot_38>::size(maxBodyCount); break;
        case zed::Zed4SignalType::Body_34_KeypointsPlus:
            sigSize = ZedBodies<ZedSkeletonKPRot_34>::size(maxBodyCount); break;
        case zed::Zed4SignalType::Body_34_Compact: default:
            sigSize = ZedBodies<ZedSkeletonCompact_34>::size(maxBodyCount); break;
        }
        return sigSize;
    }

    // TODO: Refactor this tree recursion so that it's only written once!
    /*
    void ZedSkeletonKPRot_34::recurse_tree(BODY_34_PARTS curBone,
        quat rotation,
        BODY_34_PARTS parBone,
        std::array<vec3, NUM_BONES_FULL_34> &keypoints_in,
        std::array<quat, NUM_BONES_FULL_34> &antirots,
        std::array<quat, NUM_BONES_FULL_34> &rots) {

        int bIdx = (int)curBone;
        int pIdx = (int)parBone;

        quat newRot;        
        if (parBone == BODY_34_PARTS::LAST) {
            // Root bone with no parent
            newRot = rotation;
            spdlog::trace("Bone {} has no parent. Passthrough rotation {}, keypoints: from: {}, to: {}", bIdx, newRot.str(), bone_keypoints[bIdx].str(), keypoints_in[bIdx].str());
            bone_keypoints[bIdx] = root_transform.pos;

        }
            
        else {
            newRot = rotation * antirots[pIdx];
            bone_keypoints[bIdx] = bone_keypoints[pIdx] + rots[bIdx] * newRot.inv() * (keypoints_in[bIdx] - keypoints_in[pIdx]);
        }

        for (auto& childbone : body_34_tree[bIdx]) {
            recurse_tree(childbone, newRot, curBone, keypoints_in, antirots, rots);
        }
    }
    */


    void ZedSkeletonKPRot_34::recurse_tree(BODY_34_PARTS curBone, quat rot, quat unrot,
        BODY_34_PARTS parBone,
        const std::array<vec3, NUM_BONES_FULL_34>& keypoints_in,
        const std::array<quat, NUM_BONES_FULL_34>& antirots,
        const std::array<quat, NUM_BONES_FULL_34>& rots) {
        int bIdx = (int)curBone;
        int pIdx = (int)parBone;

        quat newUnrot;
        quat newRot;

        if (parBone == BODY_34_PARTS::LAST) {
            newRot = rot;
            newUnrot = unrot;
            bone_keypoints[bIdx] = root_transform.pos;

        }
        else {
            newRot = rot * rots[pIdx];
            newUnrot = unrot * antirots[pIdx];
            bone_keypoints[bIdx] = bone_keypoints[pIdx] + newRot * newUnrot.inv() * (keypoints_in[bIdx] - keypoints_in[pIdx]);
        }

        for (auto& childbone : body_34_tree[bIdx]) {
            recurse_tree(childbone, newRot, newUnrot, curBone, keypoints_in, antirots, rots);
        }
    }
    
    // Takes an existing skeleton's keypoints and rotations, and the new set of rotations and calculates a new set of keypoint positions
    // By unrolling the old rotations and rerolling the new ones

    void ZedSkeletonKPRot_34::calculate_keypoints(const ZedSkeletonKPRot_34 & old_skel,
        const std::array<quant_quat, NUM_BONES_COMPACT_34>& new_rotations) {

        static std::array<quat, NUM_BONES_FULL_34> urots; // New rotations
        static std::array<quat, NUM_BONES_FULL_34> antirots; // Unpack the current rotations, so we can redo them

        for (int i = 0; i < NUM_BONES_COMPACT_34; i++) {
            antirots[BONELIST_34_COMPACT[i]] = bone_rotations[i].unquantize();
        }

        for (int i = 0; i < NUM_BONES_FULL_34; i++) {
            urots[i] = quat{ 0, 0, 0, 1 };
            antirots[i] = quat{ 0, 0, 0, 1 };
        }
        
        for (int i = 0; i < NUM_BONES_COMPACT_34; i++) {
            urots[BONELIST_34_COMPACT[i]] = new_rotations[i].unquantize();
        }

        recurse_tree(BODY_34_PARTS::PELVIS,
            old_skel.root_transform.ori.unquantize(),
            old_skel.root_transform.ori.unquantize(),
            BODY_34_PARTS::LAST,
            old_skel.bone_keypoints,
            antirots,
            urots);
   }




    void ZedSkeletonKPRot_34::calculate_keypoints(std::array<quant_quat, NUM_BONES_COMPACT_34>& rotations, bool reset) {

        // Unpacked rotations into full quaternions
        // TODO: Check if we can initialize these to {0,0,0,1} and exploit the staticity to shave off a fraction of a microsecond
        static std::array<quat, NUM_BONES_FULL_34> urots; // New rotations
        static std::array<quat, NUM_BONES_FULL_34> antirots; // Unpack the current rotations, so we can redo them
        static std::array<vec3, NUM_BONES_FULL_34> keypoints_in; // A temporary copy of the input keypoints

        for (int i = 0; i < NUM_BONES_FULL_34; i++) {
            urots[i] = quat{ 0, 0, 0, 1 };
            antirots[i] = quat{ 0, 0, 0, 1 };
        }


        if (!reset) {
            for (int i = 0; i < NUM_BONES_COMPACT_34; i++) {
                urots[BONELIST_34_COMPACT[i]] = rotations[i].unquantize();
            }
        }
        else {
            for (int i = 0; i < NUM_BONES_COMPACT_34; i++)
            urots[i] = quat{ 0, 0, 0, 1 };

        }
            
        for (int i = 0; i < NUM_BONES_COMPACT_34; i++) {
            antirots[BONELIST_34_COMPACT[i]] = bone_rotations[i].unquantize();
        }

        for (int i = 0; i < NUM_BONES_FULL_34; i++) {
            keypoints_in[i] = bone_keypoints[i];
        }

        spdlog::info("Root transform quaternion: {}", root_transform.ori.unquantize().str());        
        std::stringstream ss = std::stringstream();
        for (int i = 0; i < NUM_BONES_FULL_34; i++) {
            ss << "[" << antirots[i].str() << "] ";
        }

        spdlog::info("Rots before recurse, (rval {}), = : {}", (reset ? std::string("true") : std::string("false")), ss.str());

        ss.str("");
        for (int i = 0; i < NUM_BONES_FULL_34; i++) {
            ss << "[" << keypoints_in[i].str() << "] ";
        }
        
        spdlog::info("Before recurse: {}", ss.str());
        recurse_tree(BODY_34_PARTS::PELVIS, root_transform.ori.unquantize(), root_transform.ori.unquantize(), BODY_34_PARTS::LAST, keypoints_in, antirots, urots);
        //recurse_tree(BODY_34_PARTS::PELVIS, quat{0, 0, 0, 1}, BODY_34_PARTS::LAST, keypoints_in, antirots, urots);

        ss.str("");
        for (int i = 0; i < NUM_BONES_FULL_34; i++) {
            ss << "[" << bone_keypoints[i].str() << "] ";
        }
        spdlog::info("After recurse: {}", ss.str());

    }


    void ZedSkeletonKPRot_34::reset_keypoints() {
        calculate_keypoints(bone_rotations, true);
    }

    void ZedSkeletonKPRot_38::recurse_tree(BODY_38_PARTS curBone,
        quat rotation,
        BODY_38_PARTS parBone,
        std::array<vec3, NUM_BONES_FULL_38>& keypoints_in,
        std::array<quat, NUM_BONES_FULL_38>& antirots,
        std::array<quat, NUM_BONES_FULL_38>& rots) {

        int bIdx = (int)curBone;
        int pIdx = (int)parBone;

        quat newRot;
        if (parBone == BODY_38_PARTS::LAST) {
            // Root bone with no parent
            newRot = rotation;
            //spdlog::info("Bone {} has no parent. Passthrough rotation {}, keypoints: from: {}, to: {}", bIdx, newRot.str(), bone_keypoints[bIdx].str(), keypoints_in[bIdx].str());
            bone_keypoints[bIdx] = root_transform.pos;
        }

        else {
            newRot = rotation * antirots[pIdx];
            bone_keypoints[bIdx] = bone_keypoints[pIdx] + rots[bIdx] * newRot.inv() * (keypoints_in[bIdx] - keypoints_in[pIdx]);
        }

        for (auto& childbone : body_38_tree[bIdx]) {
            recurse_tree(childbone, newRot, curBone, keypoints_in, antirots, rots);
        }
    }



    void ZedSkeletonKPRot_38::calculate_keypoints(std::array<quant_quat, NUM_BONES_COMPACT_38>& rotations, bool reset) {

        // Unpacked rotations into full quaternions
        // TODO: Check if we can initialize these to {0,0,0,1} and exploit the staticity to shave off a fraction of a microsecond
        static std::array<quat, NUM_BONES_FULL_38> urots; // New rotations
        static std::array<quat, NUM_BONES_FULL_38> antirots; // Unpack the current rotations, so we can redo them

        static std::array<vec3, NUM_BONES_FULL_38> keypoints_in; // A temporary copy of the input keypoints

        for (int i = 0; i < NUM_BONES_FULL_38; i++) {
            urots[i] = quat{ 0, 0, 0, 1 };
            antirots[i] = quat{ 0, 0, 0, 1 };
        }


        if (!reset) {
            for (int i = 0; i < NUM_BONES_COMPACT_38; i++) {
                urots[BONELIST_38_COMPACT[i]] = rotations[i].unquantize();
            }
        }

        for (int i = 0; i < NUM_BONES_COMPACT_38; i++) {
            antirots[BONELIST_38_COMPACT[i]] = bone_rotations[i].unquantize();
        }

        for (int i = 0; i < NUM_BONES_FULL_38; i++) {
            keypoints_in[i] = bone_keypoints[i];
        }
        spdlog::trace("Root transform quaternion: {}", root_transform.ori.unquantize().str());
        std::stringstream ss = std::stringstream();
        for (int i = 0; i < NUM_BONES_FULL_38; i++) {
            ss << "[" << antirots[i].str() << "] ";
        }

        spdlog::trace("Rots before recurse, (rval {}), = : {}", (reset ? std::string("true") : std::string("false")), ss.str());

        ss.str("");
        for (int i = 0; i < NUM_BONES_FULL_38; i++) {
            ss << "[" << bone_keypoints[i].str() << "] ";
        }

        spdlog::trace("Before recurse: {}", ss.str());
        recurse_tree(BODY_38_PARTS::PELVIS, root_transform.ori.unquantize(), BODY_38_PARTS::LAST, keypoints_in, antirots, urots);
        

        ss.str("");
        for (int i = 0; i < NUM_BONES_FULL_38; i++) {
            ss << "[" << bone_keypoints[i].str() << "] ";
        }
        spdlog::trace("After recurse: {}", ss.str());

    }


    void ZedSkeletonKPRot_38::reset_keypoints() {
        calculate_keypoints(bone_rotations, true);
    }

    void ZedSkeletonKPRot_38::calculate_keypoints(const ZedSkeletonKPRot_38& old_skel,
        const std::array<quant_quat, NUM_BONES_COMPACT_38>& new_rotations) {

        static std::array<quat, NUM_BONES_FULL_38> urots; // New rotations
        static std::array<quat, NUM_BONES_FULL_38> antirots; // Unpack the current rotations, so we can redo them

        for (int i = 0; i < NUM_BONES_COMPACT_38; i++) {
            antirots[BONELIST_38_COMPACT[i]] = bone_rotations[i].unquantize();
        }

        for (int i = 0; i < NUM_BONES_FULL_38; i++) {
            urots[i] = quat{ 0, 0, 0, 1 };
            antirots[i] = quat{ 0, 0, 0, 1 };
        }

        for (int i = 0; i < NUM_BONES_COMPACT_38; i++) {
            urots[BONELIST_38_COMPACT[i]] = new_rotations[i].unquantize();
        }

        /*
        recurse_tree(BODY_38_PARTS::PELVIS,
            old_skel.root_transform.ori.unquantize(),
            old_skel.root_transform.ori.unquantize(),
            BODY_38_PARTS::LAST,
            old_skel.bone_keypoints,
            antirots,
            urots);
            */
    }



}



