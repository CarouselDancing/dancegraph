#pragma once

#include <array>
#include <string>
#include <magic_enum/magic_enum.hpp>
#include <core/common/utility.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <math.h>
#include <fstream>

#include <sstream>

#include <spdlog/spdlog.h>

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif


#define M_PI 3.141592653589793238462643f
#define RAD2DEG (180.0f / M_PI)


constexpr int NUM_KEYPOINTS = 34;

#define BODY_PART_TEST static_cast<int>(BODY_PARTS_POSE_38::RIGHT_ELBOW)

using json = nlohmann::json;

namespace zed {

    enum class BODY_MASTER_PARTS {
        Hips,
        LeftUpLeg,
        LeftLeg,
        LeftFoot,
        LeftToeBase,
        LeftToe_End,
        RightUpLeg,
        RightLeg,
        RightFoot,
        RightToeBase,
        RightToe_End,
        Spine,
        Spine1,
        Spine2,
        LeftShoulder,
        LeftArm,
        LeftForeArm,
        LeftHand,
        LeftHandIndex1,
        LeftHandIndex2,
        LeftHandIndex3,
        LeftHandIndex4,
        LeftHandMiddle1,
        LeftHandMiddle2,
        LeftHandMiddle3,
        LeftHandMiddle4,
        LeftHandPinky1,
        LeftHandPinky2,
        LeftHandPinky3,
        LeftHandPinky4,
        LeftHandRing1,
        LeftHandRing2,
        LeftHandRing3,
        LeftHandRing4,
        LeftHandThumb1,
        LeftHandThumb2,
        LeftHandThumb3,
        LeftHandThumb4,
        Neck,
        Head,
        HeadTop_End,
        LeftEye,
        RightEye,
        RightShoulder,
        RightArm,
        RightForeArm,
        RightHand,
        RightHandIndex1,
        RightHandIndex2,
        RightHandIndex3,
        RightHandIndex4,
        RightHandMiddle1,
        RightHandMiddle2,
        RightHandMiddle3,
        RightHandMiddle4,
        RightHandPinky1,
        RightHandPinky2,
        RightHandPinky3,
        RightHandPinky4,
        RightHandRing1,
        RightHandRing2,
        RightHandRing3,
        RightHandRing4,
        RightHandThumb1,
        RightHandThumb2,
        RightHandThumb3,
        RightHandThumb4,
        LAST
    };

    enum class BODY_34_PARTS
    {
        PELVIS = 0,
        NAVAL_SPINE = 1,
        CHEST_SPINE = 2,
        NECK = 3,
        LEFT_CLAVICLE = 4,
        LEFT_SHOULDER = 5,
        LEFT_ELBOW = 6,
        LEFT_WRIST = 7,
        LEFT_HAND = 8,
        LEFT_HANDTIP = 9,
        LEFT_THUMB = 10,
        RIGHT_CLAVICLE = 11,
        RIGHT_SHOULDER = 12,
        RIGHT_ELBOW = 13,
        RIGHT_WRIST = 14,
        RIGHT_HAND = 15,
        RIGHT_HANDTIP = 16,
        RIGHT_THUMB = 17,
        LEFT_HIP = 18,
        LEFT_KNEE = 19,
        LEFT_ANKLE = 20,
        LEFT_FOOT = 21,
        RIGHT_HIP = 22,
        RIGHT_KNEE = 23,
        RIGHT_ANKLE = 24,
        RIGHT_FOOT = 25,
        HEAD = 26,
        NOSE = 27,
        LEFT_EYE = 28,
        LEFT_EAR = 29,
        RIGHT_EYE = 30,
        RIGHT_EAR = 31,
        LEFT_HEEL = 32,
        RIGHT_HEEL = 33,

        LAST = 34
    };



    enum class BODY_38_PARTS {
        PELVIS = 0,
        SPINE_1 = 1,
        SPINE_2 = 2,
        SPINE_3 = 3,
        NECK = 4,
        NOSE = 5,
        LEFT_EYE = 6,
        RIGHT_EYE = 7,
        LEFT_EAR = 8,
        RIGHT_EAR = 9,
        LEFT_CLAVICLE = 10,
        RIGHT_CLAVICLE = 11,
        LEFT_SHOULDER = 12,
        RIGHT_SHOULDER = 13,
        LEFT_ELBOW = 14,
        RIGHT_ELBOW = 15,
        LEFT_WRIST = 16,
        RIGHT_WRIST = 17,
        LEFT_HIP = 18,
        RIGHT_HIP = 19,
        LEFT_KNEE = 20,
        RIGHT_KNEE = 21,
        LEFT_ANKLE = 22,
        RIGHT_ANKLE = 23,
        LEFT_BIG_TOE = 24,
        RIGHT_BIG_TOE = 25,
        LEFT_SMALL_TOE = 26,
        RIGHT_SMALL_TOE = 27,
        LEFT_HEEL = 28,
        RIGHT_HEEL = 29,
        // Hands
        LEFT_HAND_THUMB_4 = 30, // tip
        RIGHT_HAND_THUMB_4 = 31,
        LEFT_HAND_INDEX_1 = 32, // knuckle
        RIGHT_HAND_INDEX_1 = 33,
        LEFT_HAND_MIDDLE_4 = 34, // tip
        RIGHT_HAND_MIDDLE_4 = 35,
        LEFT_HAND_PINKY_1 = 36, // knuckle
        RIGHT_HAND_PINKY_1 = 37,
        ///@cond SHOWHIDDEN
        LAST = 38
        ///@endcond
    };


    const std::map<BODY_34_PARTS, BODY_MASTER_PARTS> body_34_masterlookup = std::map<BODY_34_PARTS, BODY_MASTER_PARTS>{
        {BODY_34_PARTS::PELVIS, BODY_MASTER_PARTS::Hips},
        {BODY_34_PARTS::NAVAL_SPINE, BODY_MASTER_PARTS::Spine},
        {BODY_34_PARTS::CHEST_SPINE, BODY_MASTER_PARTS::Spine2},
        {BODY_34_PARTS::NECK, BODY_MASTER_PARTS::Neck},
        {BODY_34_PARTS::LEFT_CLAVICLE, BODY_MASTER_PARTS::LeftShoulder},
        {BODY_34_PARTS::LEFT_SHOULDER, BODY_MASTER_PARTS::LeftArm},
        {BODY_34_PARTS::LEFT_ELBOW, BODY_MASTER_PARTS::LeftForeArm},
        {BODY_34_PARTS::LEFT_WRIST, BODY_MASTER_PARTS::LeftHand},
        {BODY_34_PARTS::LEFT_HAND, BODY_MASTER_PARTS::LeftHandMiddle1},
        {BODY_34_PARTS::LEFT_HANDTIP, BODY_MASTER_PARTS::LeftHandMiddle4 },
        {BODY_34_PARTS::LEFT_THUMB, BODY_MASTER_PARTS::LeftHandThumb4},
        {BODY_34_PARTS::RIGHT_CLAVICLE, BODY_MASTER_PARTS::RightShoulder},
        {BODY_34_PARTS::RIGHT_SHOULDER, BODY_MASTER_PARTS::RightArm},
        {BODY_34_PARTS::RIGHT_ELBOW, BODY_MASTER_PARTS::RightForeArm},
        {BODY_34_PARTS::RIGHT_WRIST, BODY_MASTER_PARTS::RightHand},
        {BODY_34_PARTS::RIGHT_HAND, BODY_MASTER_PARTS::RightHandMiddle1},
        {BODY_34_PARTS::RIGHT_HANDTIP, BODY_MASTER_PARTS::RightHandMiddle4},
        {BODY_34_PARTS::RIGHT_THUMB, BODY_MASTER_PARTS::RightHandThumb4},
        {BODY_34_PARTS::LEFT_HIP, BODY_MASTER_PARTS::LeftUpLeg},
        {BODY_34_PARTS::LEFT_KNEE, BODY_MASTER_PARTS::LeftLeg},
        {BODY_34_PARTS::LEFT_ANKLE, BODY_MASTER_PARTS::LeftFoot},
        {BODY_34_PARTS::LEFT_FOOT, BODY_MASTER_PARTS::LeftToe_End},
        {BODY_34_PARTS::RIGHT_HIP, BODY_MASTER_PARTS::RightUpLeg},
        {BODY_34_PARTS::RIGHT_KNEE, BODY_MASTER_PARTS::RightLeg},
        {BODY_34_PARTS::RIGHT_ANKLE, BODY_MASTER_PARTS::RightFoot},
        {BODY_34_PARTS::RIGHT_FOOT, BODY_MASTER_PARTS::RightToe_End},
        {BODY_34_PARTS::HEAD, BODY_MASTER_PARTS::Head},
        {BODY_34_PARTS::NOSE, BODY_MASTER_PARTS::Head},
        {BODY_34_PARTS::LEFT_EYE, BODY_MASTER_PARTS::LeftEye},
        {BODY_34_PARTS::LEFT_EAR, BODY_MASTER_PARTS::LeftEye},
        {BODY_34_PARTS::RIGHT_EYE, BODY_MASTER_PARTS::RightEye},
        {BODY_34_PARTS::RIGHT_EAR, BODY_MASTER_PARTS::RightEye},
        {BODY_34_PARTS::LEFT_HEEL, BODY_MASTER_PARTS::LeftToeBase},
        {BODY_34_PARTS::RIGHT_HEEL, BODY_MASTER_PARTS::RightToeBase},
        {BODY_34_PARTS::LAST, BODY_MASTER_PARTS::LAST}
    };

    const std::map<BODY_38_PARTS, BODY_MASTER_PARTS> body_38_masterlookup = std::map<BODY_38_PARTS, BODY_MASTER_PARTS>{

        {BODY_38_PARTS::PELVIS, BODY_MASTER_PARTS::Hips},
        {BODY_38_PARTS::SPINE_1, BODY_MASTER_PARTS::Spine},
        {BODY_38_PARTS::SPINE_2, BODY_MASTER_PARTS::Spine1},
        {BODY_38_PARTS::SPINE_3, BODY_MASTER_PARTS::Spine2},
        {BODY_38_PARTS::NECK, BODY_MASTER_PARTS::Neck},
        {BODY_38_PARTS::NOSE, BODY_MASTER_PARTS::Head},
        {BODY_38_PARTS::LEFT_EYE, BODY_MASTER_PARTS::LeftEye},
        {BODY_38_PARTS::RIGHT_EYE, BODY_MASTER_PARTS::RightEye},
        {BODY_38_PARTS::LEFT_EAR, BODY_MASTER_PARTS::LeftEye},
        {BODY_38_PARTS::RIGHT_EAR, BODY_MASTER_PARTS::RightEye},
        {BODY_38_PARTS::LEFT_CLAVICLE, BODY_MASTER_PARTS::LeftShoulder},
        {BODY_38_PARTS::RIGHT_CLAVICLE, BODY_MASTER_PARTS::RightShoulder},
        {BODY_38_PARTS::LEFT_SHOULDER, BODY_MASTER_PARTS::LeftArm},
        {BODY_38_PARTS::RIGHT_SHOULDER, BODY_MASTER_PARTS::RightArm},
        {BODY_38_PARTS::LEFT_ELBOW, BODY_MASTER_PARTS::LeftForeArm},
        {BODY_38_PARTS::RIGHT_ELBOW, BODY_MASTER_PARTS::RightForeArm},
        {BODY_38_PARTS::LEFT_WRIST, BODY_MASTER_PARTS::LeftHand},
        {BODY_38_PARTS::RIGHT_WRIST, BODY_MASTER_PARTS::RightHand},
        {BODY_38_PARTS::LEFT_HIP, BODY_MASTER_PARTS::LeftUpLeg},
        {BODY_38_PARTS::RIGHT_HIP, BODY_MASTER_PARTS::RightUpLeg},
        {BODY_38_PARTS::LEFT_KNEE, BODY_MASTER_PARTS::LeftLeg},
        {BODY_38_PARTS::RIGHT_KNEE, BODY_MASTER_PARTS::RightLeg},
        {BODY_38_PARTS::LEFT_ANKLE, BODY_MASTER_PARTS::LeftFoot},
        {BODY_38_PARTS::RIGHT_ANKLE, BODY_MASTER_PARTS::RightFoot},
        {BODY_38_PARTS::LEFT_BIG_TOE, BODY_MASTER_PARTS::LeftToe_End},
        {BODY_38_PARTS::RIGHT_BIG_TOE, BODY_MASTER_PARTS::RightToe_End},
        {BODY_38_PARTS::LEFT_SMALL_TOE, BODY_MASTER_PARTS::LeftToeBase },
        {BODY_38_PARTS::RIGHT_SMALL_TOE, BODY_MASTER_PARTS::RightToeBase},
        {BODY_38_PARTS::LEFT_HEEL, BODY_MASTER_PARTS::LeftFoot},
        {BODY_38_PARTS::RIGHT_HEEL, BODY_MASTER_PARTS::RightFoot},
        {BODY_38_PARTS::LEFT_HAND_THUMB_4, BODY_MASTER_PARTS::LeftHandThumb4},
        {BODY_38_PARTS::RIGHT_HAND_THUMB_4, BODY_MASTER_PARTS::RightHandThumb4},
        {BODY_38_PARTS::LEFT_HAND_INDEX_1, BODY_MASTER_PARTS::LeftHandIndex1},
        {BODY_38_PARTS::RIGHT_HAND_INDEX_1, BODY_MASTER_PARTS::RightHandIndex1},
        {BODY_38_PARTS::LEFT_HAND_MIDDLE_4, BODY_MASTER_PARTS::LeftHandMiddle4},
        {BODY_38_PARTS::RIGHT_HAND_MIDDLE_4, BODY_MASTER_PARTS::RightHandMiddle4},
        {BODY_38_PARTS::LEFT_HAND_PINKY_1, BODY_MASTER_PARTS::LeftHandPinky1},
        {BODY_38_PARTS::RIGHT_HAND_PINKY_1, BODY_MASTER_PARTS::RightHandPinky1},
        {BODY_38_PARTS::LAST, BODY_MASTER_PARTS::LAST}
    };



    enum Zed4SignalType {
        Body_34_Compact = 0,
        Body_38_Compact,
        Body_34_Full,
        Body_38_Full,
        Body_34_Keypoints,
        Body_38_Keypoints,
        Body_34_KeypointsPlus,
        Body_38_KeypointsPlus
    };





    constexpr int NUM_BONES_FULL_34 = 34;
    constexpr int NUM_BONES_FULL_38 = 38;
    constexpr int NUM_BONES_COMPACT_34 = 11;
    constexpr int NUM_BONES_COMPACT_38 = 18;

    // That's right. There are only *eleven* quaternions out of the 34 that are actually being populated by something other than (0,0,0,1)!
    const std::vector<int> BONELIST_34_COMPACT = std::vector<int>{ 2,3,5,6,12,13,18,19,22,23,26 };
    const std::vector<int> BONELIST_34_FULL = std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33 };
    const std::vector<int> BONELIST_38_FULL = std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37 };
    const std::vector<int> BONELIST_38_COMPACT = std::vector<int>{ 1,2,3,4,10,11,12,13,14,15,16,17,18,19,20,21,22,23 };


    //const std::vector<int> BONELIST_38_COMPACT = BONELIST_38_FULL;

    const std::array<std::vector<int>, magic_enum::enum_count<Zed4SignalType>()> useful_bone_indices = {
        BONELIST_34_COMPACT,
        BONELIST_38_COMPACT,
        BONELIST_34_FULL,
        BONELIST_38_FULL,
        BONELIST_34_FULL,
        BONELIST_38_FULL,
        BONELIST_34_COMPACT,
        BONELIST_38_COMPACT,
    };


    // Number of bones we use for the signal (i.e. the ones that aren't just nulled-out (0, 0, 0, 1) quaternions
    const std::array<std::pair<int, int>, magic_enum::enum_count<Zed4SignalType>()> signal_bonecount = {
        std::pair<int, int> {NUM_BONES_COMPACT_34, NUM_BONES_FULL_34},
        std::pair<int, int>{NUM_BONES_COMPACT_38, NUM_BONES_FULL_38},
        std::pair<int, int>{NUM_BONES_FULL_34, NUM_BONES_FULL_34},
        std::pair<int, int> {NUM_BONES_FULL_38, NUM_BONES_FULL_38},
        std::pair<int, int> {NUM_BONES_FULL_34, NUM_BONES_FULL_34},
        std::pair<int, int>{NUM_BONES_FULL_38, NUM_BONES_FULL_38},
        std::pair<int, int> {NUM_BONES_COMPACT_34, NUM_BONES_FULL_34},
        std::pair<int, int>{NUM_BONES_COMPACT_38, NUM_BONES_FULL_38},
    };

    constexpr int MAX_SKEL_COUNT = 10;


    struct quat;
    struct xform_compact;
    struct ZedBodiesCompact;
    struct ZedSkeletonDataCompact;



    struct vec3 {
        float x, y, z;

        vec3 operator + (const vec3& v);

        vec3 operator - (const vec3& v);
        const std::string str() const;
        vec3 scale(float s);
        const quat embiggen() const;

    };


    struct quant_quat {
        int16_t x, y, z;

        const quat unquantize() const;
        const std::string str() const;
    };



    struct quat {
        float x, y, z, w;


        const vec3 toEuler() const;
        const std::string str() const;
        const vec3 ensmallen() const;

        quat inv() const;


        const quant_quat quantize() const;
    };


    vec3 operator + (const vec3& a, const  vec3& b);
    vec3 operator - (const vec3& a, const  vec3& b);
    vec3 operator * (float f, const vec3& a);
    float dot(const vec3& a, const  vec3& b);

    quat operator + (const quat& a, const quat& b);
    quat operator - (const quat& a, const quat& b);
    quat operator * (float f, const quat& a);
    float dot(const quat& a, const quat& b);

    quant_quat operator + (const quant_quat& a, const quant_quat& b);
    quant_quat operator - (const quant_quat& a, const quant_quat& b);

    vec3 lerp(const vec3& a, const vec3& b, float t);
    quat slerp(const quat& a, const quat& b, float t);
    quant_quat slerp(const quant_quat& a, const quant_quat& b, float t);

    vec3 operator *(const quat& q, const vec3 v);

    quat operator * (const quat& a, const quat& b);



    struct xform {

        vec3 pos; quat ori;

        const std::string str() const;
        xform scale(float s);
        xform_compact ensmallen() const;

    };


    struct xform_compact {
        vec3 pos; vec3 ori;

        const std::string str() const;
        xform_compact scale(float s);
        xform embiggen() const;
    };

    /*
    const std::map<int, vec3> body_tpose_34 = {
        {  0, vec3{ 0.000000f,  0.966564f,   0.000000f}},
        {  1, vec3{ 0.000000f,  1.065799f,  -0.012273f}},
        {  2, vec3{ 0.000000f,  1.315855f,  -0.042761f}},
        {  3, vec3{ 0.000000f,  1.466180f,  -0.034832f}},
        {  4, vec3{ -0.061058f, 1.406959f,  -0.035706f}},
        {  5, vec3{ -0.187608f, 1.404300f,  -0.061715f}},
        {  6, vec3{ -0.461655f, 1.404300f,  -0.061715f}},
        {  7, vec3{ -0.737800f, 1.404300f,  -0.061715f}},
        { 10, vec3{ -0.856083f, 1.341810f,  -0.017511f}},
        { 11, vec3{ 0.061057f,  1.406960f,  -0.035706f}},
        { 12, vec3{ 0.187608f,  1.404300f,  -0.061715f}},
        { 13, vec3{ 0.461654f,  1.404301f,  -0.061715f}},
        { 14, vec3{ 0.737799f,  1.404301f,  -0.061714f}},
        { 17, vec3{ 0.856082f,  1.341811f,  -0.017511f}},
        { 18, vec3{ -0.091245f, 0.900000f,  -0.000554f}},
        { 19, vec3{ -0.093691f, 0.494046f,  -0.005710f}},
        { 20, vec3{ -0.091244f, 0.073566f,  -0.026302f}},
        { 21, vec3{ -0.094977f, -0.031356f,  0.100105f}},
        { 22, vec3{ 0.091245f,  0.900000f,  -0.000554f}},
        { 23, vec3{ 0.093692f,  0.494046f,  -0.005697f}},
        { 24, vec3{ 0.091245f,  0.073567f,  -0.026302f}},
        { 25, vec3{ 0.094979f,  -0.031355f,  0.100105f}},
        { 26, vec3{ 0.000000f,  1.569398f,  -0.003408f}}
    };

    const std::map<int, vec3> body_tpose_38 = {
            {0 , vec3{0,0.9979,0}},
            {1 , vec3{0,1.0972,0.0123}},
            {2 , vec3{0,1.2136,0.0265}},
            {3 , vec3{0,1.3472,0.0428}},
            {4 , vec3{0,1.4975,0.0348}},
            {10, vec3{0.0611,1.4383,0.0357}},
            {11, vec3{-0.0611,1.4383,0.0357}},
            {12, vec3{0.1876,1.4357,0.0617}},
            {13, vec3{-0.1876,1.4357,0.0617}},
            {14, vec3{0.4617,1.4357,0.0617}},
            {15, vec3{-0.4617,1.4357,0.0617}},
            {16, vec3{0.7378,1.4357,0.0617}},
            {17, vec3{-0.7378,1.4357,0.0617}},
            {18, vec3{0.0912,0.9314,0.0006}},
            {19, vec3{-0.0912,0.9314,0.0006}},
            {20, vec3{0.0937,0.5254,0.0057}},
            {21, vec3{-0.0937,0.5254,0.0057}},
            {22, vec3{0.0912,0.1049,0.0263}},
            {23, vec3{-0.0912,0.1049,0.0263}}
    };
    */

    // We need this ugliness so that it gets picked up after transport


    const std::vector<vec3> bone_master_tpose = std::vector<vec3>{
        vec3{ 0.0000, 0.9979, 0.0000 },
        vec3{ 0.0912, 0.9314, 0.0006 },
        vec3{ 0.0937, 0.5254, 0.0057 },
        vec3{ 0.0912, 0.1049, 0.0263 },
        vec3{ 0.0950, 0.0000, -0.1001 },
        vec3{ 0.0950, 0.0000, -0.2000 },
        vec3{ -0.0912, 0.9314, 0.0006 },
        vec3{ -0.0937, 0.5254, 0.0057 },
        vec3{ -0.0912, 0.1049, 0.0263 },
        vec3{ -0.0950, 0.0000, -0.1001 },
        vec3{ -0.0950, 0.0000, -0.2000 },
        vec3{ 0.0000, 1.0972, 0.0123 },
        vec3{ 0.0000, 1.2136, 0.0265 },
        vec3{ 0.0000, 1.3472, 0.0428 },
        vec3{ 0.0611, 1.4383, 0.0357 },
        vec3{ 0.1876, 1.4357, 0.0617 },
        vec3{ 0.4617, 1.4357, 0.0617 },
        vec3{ 0.7378, 1.4357, 0.0617 },
        vec3{ 0.8605, 1.4333, 0.0335 },
        vec3{ 0.8970, 1.4200, 0.0335 },
        vec3{ 0.9181, 1.3931, 0.0335 },
        vec3{ 0.9181, 1.3623, 0.0335 },
        vec3{ 0.8656, 1.4357, 0.0617 },
        vec3{ 0.8995, 1.4233, 0.0617 },
        vec3{ 0.9208, 1.3960, 0.0617 },
        vec3{ 0.9208, 1.3592, 0.0617 },
        vec3{ 0.8469, 1.4334, 0.1090 },
        vec3{ 0.8858, 1.4192, 0.1090 },
        vec3{ 0.9017, 1.3988, 0.1090 },
        vec3{ 0.9017, 1.3696, 0.1090 },
        vec3{ 0.8593, 1.4358, 0.0839 },
        vec3{ 0.8931, 1.4234, 0.0839 },
        vec3{ 0.9135, 1.3974, 0.0839 },
        vec3{ 0.9135, 1.3608, 0.0839 },
        vec3{ 0.7757, 1.4140, 0.0317 },
        vec3{ 0.8175, 1.3928, 0.0243 },
        vec3{ 0.8561, 1.3732, 0.0175 },
        vec3{ 0.8866, 1.3577, 0.0121 },
        vec3{ 0.0000, 1.4975, 0.0348 },
        vec3{ 0.0000, 1.6008, 0.0034 },
        vec3{ 0.0000, 1.7855, -0.0630 },
        vec3{ 0.0295, 1.6776, -0.1294 },
        vec3{ -0.0294, 1.6776, -0.1294 },
        vec3{ -0.0611, 1.4383, 0.0357 },
        vec3{ -0.1876, 1.4357, 0.0617 },
        vec3{ -0.4617, 1.4357, 0.0617 },
        vec3{ -0.7378, 1.4357, 0.0617 },
        vec3{ -0.8605, 1.4319, 0.0336 },
        vec3{ -0.8970, 1.4186, 0.0343 },
        vec3{ -0.9181, 1.3917, 0.0357 },
        vec3{ -0.9181, 1.3610, 0.0373 },
        vec3{ -0.8656, 1.4357, 0.0617 },
        vec3{ -0.8995, 1.4233, 0.0623 },
        vec3{ -0.9208, 1.3961, 0.0637 },
        vec3{ -0.9208, 1.3593, 0.0656 },
        vec3{ -0.8469, 1.4358, 0.1090 },
        vec3{ -0.8858, 1.4217, 0.1097 },
        vec3{ -0.9017, 1.4013, 0.1108 },
        vec3{ -0.9017, 1.3721, 0.1123 },
        vec3{ -0.8593, 1.4369, 0.0838 },
        vec3{ -0.8931, 1.4246, 0.0845 },
        vec3{ -0.9135, 1.3986, 0.0858 },
        vec3{ -0.9135, 1.3620, 0.0877 },
        vec3{ -0.7757, 1.4125, 0.0328 },
        vec3{ -0.8175, 1.3909, 0.0266 },
        vec3{ -0.8561, 1.3710, 0.0208 },
        vec3{ -0.8866, 1.3553, 0.0162 }
    };



#if 0

    const std::map<vec3> body_part_positionss = std::map<vec3>{
        Hips, vec3{ 0.0000, 0.9979, 0.0000 },
        LeftUpLeg, vec3{ 0.0912, 0.9314, 0.0006 },
        LeftLeg, vec3{ 0.0937, 0.5254, 0.0057 },
        LeftFoot, vec3{ 0.0912, 0.1049, 0.0263 },
        LeftToeBase, vec3{ 0.0950, 0.0000, -0.1001 },
        LeftToe_End, vec3{ 0.0950, 0.0000, -0.2000 },
        RightUpLeg, vec3{ -0.0912, 0.9314, 0.0006 },
        RightLeg, vec3{ -0.0937, 0.5254, 0.0057 },
        RightFoot, vec3{ -0.0912, 0.1049, 0.0263 },
        RightToeBase, vec3{ -0.0950, 0.0000, -0.1001 },
        RightToe_End, vec3{ -0.0950, 0.0000, -0.2000 },
        Spine, vec3{ 0.0000, 1.0972, 0.0123 },
        Spine1, vec3{ 0.0000, 1.2136, 0.0265 },
        Spine2, vec3{ 0.0000, 1.3472, 0.0428 },
        LeftShoulder, vec3{ 0.0611, 1.4383, 0.0357 },
        LeftArm, vec3{ 0.1876, 1.4357, 0.0617 },
        LeftForeArm, vec3{ 0.4617, 1.4357, 0.0617 },
        LeftHand, vec3{ 0.7378, 1.4357, 0.0617 },
        LeftHandIndex1, vec3{ 0.8605, 1.4333, 0.0335 },
        LeftHandIndex2, vec3{ 0.8970, 1.4200, 0.0335 },
        LeftHandIndex3, vec3{ 0.9181, 1.3931, 0.0335 },
        LeftHandIndex4, vec3{ 0.9181, 1.3623, 0.0335 },
        LeftHandMiddle1, vec3{ 0.8656, 1.4357, 0.0617 },
        LeftHandMiddle2, vec3{ 0.8995, 1.4233, 0.0617 },
        LeftHandMiddle3, vec3{ 0.9208, 1.3960, 0.0617 },
        LeftHandMiddle4, vec3{ 0.9208, 1.3592, 0.0617 },
        LeftHandPinky1, vec3{ 0.8469, 1.4334, 0.1090 },
        LeftHandPinky2, vec3{ 0.8858, 1.4192, 0.1090 },
        LeftHandPinky3, vec3{ 0.9017, 1.3988, 0.1090 },
        LeftHandPinky4, vec3{ 0.9017, 1.3696, 0.1090 },
        LeftHandRing1, vec3{ 0.8593, 1.4358, 0.0839 },
        LeftHandRing2, vec3{ 0.8931, 1.4234, 0.0839 },
        LeftHandRing3, vec3{ 0.9135, 1.3974, 0.0839 },
        LeftHandRing4, vec3{ 0.9135, 1.3608, 0.0839 },
        LeftHandThumb1, vec3{ 0.7757, 1.4140, 0.0317 },
        LeftHandThumb2, vec3{ 0.8175, 1.3928, 0.0243 },
        LeftHandThumb3, vec3{ 0.8561, 1.3732, 0.0175 },
        LeftHandThumb4, vec3{ 0.8866, 1.3577, 0.0121 },
        Neck, vec3{ 0.0000, 1.4975, 0.0348 },
        Head, vec3{ 0.0000, 1.6008, 0.0034 },
        HeadTop_End, vec3{ 0.0000, 1.7855, -0.0630 },
        LeftEye, vec3{ 0.0295, 1.6776, -0.1294 },
        RightEye, vec3{ -0.0294, 1.6776, -0.1294 },
        RightShoulder, vec3{ -0.0611, 1.4383, 0.0357 },
        RightArm, vec3{ -0.1876, 1.4357, 0.0617 },
        RightForeArm, vec3{ -0.4617, 1.4357, 0.0617 },
        RightHand, vec3{ -0.7378, 1.4357, 0.0617 },
        RightHandIndex1, vec3{ -0.8605, 1.4319, 0.0336 },
        RightHandIndex2, vec3{ -0.8970, 1.4186, 0.0343 },
        RightHandIndex3, vec3{ -0.9181, 1.3917, 0.0357 },
        RightHandIndex4, vec3{ -0.9181, 1.3610, 0.0373 },
        RightHandMiddle1, vec3{ -0.8656, 1.4357, 0.0617 },
        RightHandMiddle2, vec3{ -0.8995, 1.4233, 0.0623 },
        RightHandMiddle3, vec3{ -0.9208, 1.3961, 0.0637 },
        RightHandMiddle4, vec3{ -0.9208, 1.3593, 0.0656 },
        RightHandPinky1, vec3{ -0.8469, 1.4358, 0.1090 },
        RightHandPinky2, vec3{ -0.8858, 1.4217, 0.1097 },
        RightHandPinky3, vec3{ -0.9017, 1.4013, 0.1108 },
        RightHandPinky4, vec3{ -0.9017, 1.3721, 0.1123 },
        RightHandRing1, vec3{ -0.8593, 1.4369, 0.0838 },
        RightHandRing2, vec3{ -0.8931, 1.4246, 0.0845 },
        RightHandRing3, vec3{ -0.9135, 1.3986, 0.0858 },
        RightHandRing4, vec3{ -0.9135, 1.3620, 0.0877 },
        RightHandThumb1, vec3{ -0.7757, 1.4125, 0.0328 },
        RightHandThumb2, vec3{ -0.8175, 1.3909, 0.0266 },
        RightHandThumb3, vec3{ -0.8561, 1.3710, 0.0208 },
        RightHandThumb4, vec3{ -0.8866, 1.3553, 0.0162 }
    };
#endif


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        
        xform_quant {
        vec3 pos;
        quant_quat ori;

        const std::string str() const;
        xform_quant scale(float s);

        const xform embiggen() const;

    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonCompact_34 {
        int id;
        xform_quant root_transform;
        std::array < quant_quat, NUM_BONES_COMPACT_34> bone_rotations;
        static int bones_transmitted() {
            return NUM_BONES_COMPACT_34;
        }
        static int bones_skeleton() {
            return NUM_BONES_FULL_34;
        }
        void reset_keypoints() {
        }
        void calculate_keypoints(const ZedSkeletonCompact_34 & old_skel,
            const std::array<quant_quat, NUM_BONES_COMPACT_34>& new_rotations) {}

    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonCompact_38 {
        int id;
        xform_quant root_transform;
        std::array<quant_quat, NUM_BONES_COMPACT_38> bone_rotations;
        static int bones_transmitted() {
            return NUM_BONES_FULL_38;
        }
        static int bones_skeleton() {
            return NUM_BONES_COMPACT_34;
        }
        void reset_keypoints() {
        }
        void calculate_keypoints(const ZedSkeletonCompact_38& old_skel,
            const std::array<quant_quat, NUM_BONES_COMPACT_38>& new_rotations) {}

    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonFull_34 {
        int id;
        xform_quant root_transform;
        std::array<quant_quat, NUM_BONES_FULL_34> bone_rotations;
        static int bones_transmitted() {
            return NUM_BONES_FULL_34;
        }
        static int bones_skeleton() {
            return NUM_BONES_FULL_34;
        }
        void reset_keypoints() {
        }
        void calculate_keypoints(const ZedSkeletonFull_34& old_skel,
            const std::array<quant_quat, NUM_BONES_FULL_34>& new_rotations) {}

    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif



#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonFull_38 {
        int id;
        xform_quant root_transform;
        std::array<quant_quat, NUM_BONES_FULL_38> bone_rotations;
        static int bones_transmitted() {
            return NUM_BONES_FULL_38;
        }
        static int bones_skeleton() {
            return NUM_BONES_FULL_38;
        }
        void reset_keypoints() {
        }

        void calculate_keypoints(const ZedSkeletonFull_38& old_skel,
            const std::array<quant_quat, NUM_BONES_FULL_38>& new_rotations) {}

    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonKP_38 {
        int id;
        xform_quant root_transform;
        std::array<vec3, NUM_BONES_FULL_38> bone_keypoints;
        static int bones_transmitted() {
            return NUM_BONES_FULL_38;
        }
        static int bones_skeleton() {
            return NUM_BONES_FULL_38;
        }
        void reset_keypoints() {
        }
        void calculate_keypoints(const ZedSkeletonKP_38& old_skel,
            const std::array<quant_quat, NUM_BONES_FULL_38>& new_rotations) {}

    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonKP_34 {
        int id;
        xform_quant root_transform;
        std::array<vec3, NUM_BONES_FULL_34> bone_keypoints;
        static int bones_transmitted() {
            return NUM_BONES_FULL_34;
        }
        static int bones_skeleton() {
            return NUM_BONES_FULL_34;
        }
        void calculate_keypoints(const ZedSkeletonKP_34& old_skel,
            const std::array<quant_quat, NUM_BONES_FULL_34>& new_rotations) {}
    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonKPRot_34 {
        int id;
        xform_quant root_transform;
        std::array<vec3, NUM_BONES_FULL_34> bone_keypoints;

        std::array<quant_quat, NUM_BONES_COMPACT_34> bone_rotations;

        static int bones_transmitted() {
            return NUM_BONES_COMPACT_34;
        }
        static int bones_skeleton() {
            return NUM_BONES_FULL_34;
        }
        
        static vec3 tpose_position(int idx) {
            return bone_master_tpose[(int)body_34_masterlookup.at((BODY_34_PARTS)idx)];
        }


        // Recalculates the keypoints based on the bone rotations
        void reset_keypoints();

        // Recalculates the keypoints based on a given set of bone rotations
        void calculate_keypoints(std::array<quant_quat, NUM_BONES_COMPACT_34>& rotations, bool reset = false);
        void calculate_keypoints(const ZedSkeletonKPRot_34& old_skel,
            const std::array<quant_quat, NUM_BONES_COMPACT_34>& new_rotations);

    protected:
        void recurse_tree(BODY_34_PARTS bIdx,
            quat rotation,
            quat unrotation,
            BODY_34_PARTS pIdx,            
            const std::array<vec3, NUM_BONES_FULL_34>& bones_in,
            const std::array<quat, NUM_BONES_FULL_34> & antirots,
            const std::array<quat, NUM_BONES_FULL_34> & rots);
        
    };


#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif    
    struct
#ifdef __GNUC__
        __attribute__((__packed))
#endif        

        ZedSkeletonKPRot_38 {
        int id;
        xform_quant root_transform;
        std::array<vec3, NUM_BONES_FULL_38> bone_keypoints;
        std::array<quant_quat, NUM_BONES_COMPACT_38> bone_rotations;

        static int bones_transmitted() {
            return NUM_BONES_FULL_38;
        }
        static int bones_skeleton() {
            return NUM_BONES_COMPACT_38;
        }

        static vec3 tpose_position(int idx) {
            return bone_master_tpose[(int)body_38_masterlookup.at((BODY_38_PARTS)idx)];
        }

        // Calculates the keypoints based on a given set of bone rotations
        void reset_keypoints();

        // Recalculates the keypoints based on the bone rotations of the current object
        void calculate_keypoints(std::array<quant_quat, NUM_BONES_COMPACT_38>& rotations, bool reset = false);
        void calculate_keypoints(const ZedSkeletonKPRot_38& old_skel,
            const std::array<quant_quat, NUM_BONES_COMPACT_38>& new_rotations);

        
    protected:
        void recurse_tree(BODY_38_PARTS bIdx,
            quat rotation, BODY_38_PARTS pIdx,
            std::array<vec3, NUM_BONES_FULL_38>& bones_in,
            std::array<quat, NUM_BONES_FULL_38>& antirots,
            std::array<quat, NUM_BONES_FULL_38> & rots);


    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif

    struct Zed4Header
    {
        uint8_t num_skeletons = 0;

        // The deltas are all offset from the value in SignalMetadata
        int32_t grab_delta; // Time to grab the frames (will be negative)
        int32_t tracking_delta; // Time to track the body
        int32_t processing_delta; // Time to process the tracked body
        ZedSkeletonKP_38 skeletons[0];
    };


    template
        <typename S>
        struct ZedBodies {

        uint8_t num_skeletons = 0;
        //std::chrono::time_point<std::chrono::system_clock> elapsed;
        int32_t grab_delta;
        int32_t track_delta;
        int32_t process_delta;

        S skeletons[0];

        static size_t size(int numskels) {
            return (sizeof(Zed4Header) + numskels * sizeof(S));
        }

        static ZedBodies<S>* allocate(int numskels) {
            ZedBodies<S>* zp = (ZedBodies<S> *) malloc(ZedBodies<S>::size(numskels));
            return zp;
        }

    };

    struct UnityZedData
    {
        int numBodies;
        //long int elapsed;
        double elapsed;
        quat skeletonData[NUM_BONES_COMPACT_38 * 10];
        int skeletonID[10];
        xform rootTransform[10];
    };



    class BufferProperties {
    public:
        // Whether this has been populated already; needed because there doesn't seem to be an 'init' function or similar for the config dll
        bool populated = false;

        int maxBodyCount; // Maximum number of bodies the zedcam will see. If more people are tracked, it takes the oldest ones. Should probably be 1 or 2

        //std::string name; // Name of the ringbuffer
        //int numEntries; // Number of copies of the bufferlist are in the ringbuffer; Suggested value 3

        float writeRate; // (provisional) how often the buffer is written to, in fps
        float readRate; // (provisional) how often the buffer is read from, in fps

        float confidence; //Object detection confidence threshold

        float playbackRate; // How fast to play back the .dat files

        int keypointsThreshold; // Minimum number of keypoints to return

        int fps; // Really we'd prefer this to be a string, but it'll confuse things when writing the config file
        std::string depth;
        std::string resolution;
        std::string coordsystem;

        std::string videoFile;
        std::string trackRecording; // TODO: Get a better interface for this

        std::string trackingModel; // FAST, MEDIUM or ACCURATE

        std::string svoInput = std::string(""); // Input SVO file for testing

        Zed4SignalType bodySignalType;
        float skeletonSmoothing;
        bool timeTracking;
        bool staticCamera;
        bool allowReducedPrecision;

        int signalSize();

        BufferProperties() = default;

        // TODO: fix the below. Cannot be optional
        void populate_from_json(const json& opts = {}) {
            maxBodyCount = opts.at("zed/v2.1").at("zedMaxBodyCount");
            fps = opts.at("zed/v2.1").at("zedFPS");
            depth = opts.at("zed/v2.1").at("zedDepth");
            resolution = opts.at("zed/v2.1").at("zedResolution");
            confidence = opts.at("zed/v2.1").at("zedConfidenceThreshold");
            coordsystem = opts.at("zed/v2.1").at("zedCoordinateSystem");
            videoFile = opts.at("zed/v2.1").at("zedRecordVideo");
            std::string bst = opts.at("zed/v2.1").at("zedBodySignalType");
            trackingModel = opts.at("zed/v2.1").at("zedTrackingModel");
            skeletonSmoothing = opts.at("zed/v2.1").at("zedSkeletonSmoothing");
            timeTracking = opts.at("zed/v2.1").at("zedTimeTracking");
            svoInput = opts.at("zed/v2.1").at("zedSVOInput");
            keypointsThreshold = opts.at("zed/v2.1").at("zedKeypointsThreshold");

            staticCamera = opts.at("zed/v2.1").at("zedStaticCamera");
            allowReducedPrecision = opts.at("zed/v2.1").at("zedAllowReducedPrecision");

            spdlog::info("Coord system is {}, bodySignalType is {}", coordsystem, bst);

            try {
                bodySignalType = magic_enum::enum_cast<Zed4SignalType>(bst).value();
                spdlog::info("Body signal type {} found", bst);
            }
            catch (std::out_of_range e) {
                spdlog::info("No proper body sig type, switching to default");
                bodySignalType = Zed4SignalType::Body_34_Compact;
            }

            populated = true;
        }
    };





    const std::vector<std::string> body_parts_34 = {
    "Pelvis",
    "NavalSpine",
    "ChestSpine",
    "Neck",
    "LeftClavicle",
    "LeftShoulder",
    "LeftElbow",
    "LeftWrist",
    "LeftHand",
    "LeftHandtip",
    "LeftThumb",
    "RightClavicle",
    "RightShoulder",
    "RightElbow",
    "RightWrist",
    "RightHand",
    "RightHandtip",
    "RightThumb",
    "LeftHip",
    "LeftKnee",
    "LeftAnkle",
    "LeftFoot",
    "RightHip",
    "RightKnee",
    "RightAnkle",
    "RightFoot",
    "Head",
    "Nose",
    "LeftEye",
    "LeftEar",
    "RightEye",
    "RightEar",
    "LeftHeel",
    "RightHeel"
    };

    const std::vector<std::string> body_parts_38 = {
        "Pelvis",
        "Spine_1",
        "Spine_2",
        "Spine_3",
        "Neck",
        "Nose",
        "Left_Eye",
        "Right_Eye",
        "Left_Ear",
        "Right_Ear",
        "Left_Clavicle",
        "Right_Clavicle",
        "Left_Shoulder",
        "Right_Shoulder",
        "Left_Elbow",
        "Right_Elbow",
        "Left_Wrist",
        "Right_Wrist",
        "Left_Hip",
        "Right_Hip",
        "Left_Knee",
        "Right_Knee",
        "Left_Ankle",
        "Right_Ankle",
        "Left_Big_Toe",
        "Right_Big_Toe",
        "Left_Small_Toe",
        "Right_Small_Toe",
        "Left_Heel",
        "Right_Heel",
        "Left_Hand_Thumb_4",
        "Right_Hand_Thumb_4",
        "Left_Hand_Index_1",
        "Right_Hand_Index_1",
        "Left_Hand_Middle_4",
        "Right_Hand_Middle_4",
        "Left_Hand_Pinky_1",
        "Right_Hand_Pinky_1"
    };

    const std::vector<std::vector<BODY_34_PARTS>> body_34_tree =
        std::vector<std::vector<BODY_34_PARTS>>{
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::NAVAL_SPINE, BODY_34_PARTS::LEFT_HIP, BODY_34_PARTS::RIGHT_HIP},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::CHEST_SPINE},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_CLAVICLE, BODY_34_PARTS::RIGHT_CLAVICLE, BODY_34_PARTS::NECK},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::HEAD, BODY_34_PARTS::LEFT_EYE, BODY_34_PARTS::RIGHT_EYE},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_SHOULDER},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_ELBOW},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_WRIST},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_HAND, BODY_34_PARTS::LEFT_THUMB},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_HANDTIP},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_SHOULDER},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_ELBOW},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_WRIST},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_HAND, BODY_34_PARTS::RIGHT_THUMB},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_HANDTIP},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_KNEE},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_ANKLE},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_FOOT, BODY_34_PARTS::LEFT_HEEL},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_KNEE},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_ANKLE},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_FOOT, BODY_34_PARTS::RIGHT_HEEL},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::NOSE},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_EAR},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_EAR},
        std::vector<BODY_34_PARTS>{},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::LEFT_FOOT},
        std::vector<BODY_34_PARTS>{BODY_34_PARTS::RIGHT_FOOT}
    };

    
    const std::vector<std::vector<BODY_38_PARTS>> body_38_tree =
        std::vector<std::vector<BODY_38_PARTS>>{
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::SPINE_1, BODY_38_PARTS::LEFT_HIP, BODY_38_PARTS::RIGHT_HIP},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::SPINE_2},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::SPINE_3},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::NECK, BODY_38_PARTS::LEFT_CLAVICLE, BODY_38_PARTS::RIGHT_CLAVICLE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::NOSE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_EYE, BODY_38_PARTS::RIGHT_EYE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_EAR},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_EAR},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_SHOULDER},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_SHOULDER},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_ELBOW},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_ELBOW},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_WRIST},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_WRIST},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_HAND_THUMB_4, BODY_38_PARTS::LEFT_HAND_INDEX_1, BODY_38_PARTS::LEFT_HAND_MIDDLE_4, BODY_38_PARTS::LEFT_HAND_PINKY_1},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_HAND_THUMB_4, BODY_38_PARTS::RIGHT_HAND_INDEX_1, BODY_38_PARTS::RIGHT_HAND_MIDDLE_4, BODY_38_PARTS::RIGHT_HAND_PINKY_1},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_KNEE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_KNEE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_ANKLE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_ANKLE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::LEFT_HEEL, BODY_38_PARTS::LEFT_BIG_TOE, BODY_38_PARTS::LEFT_SMALL_TOE},
        std::vector<BODY_38_PARTS>{BODY_38_PARTS::RIGHT_HEEL, BODY_38_PARTS::RIGHT_BIG_TOE, BODY_38_PARTS::RIGHT_SMALL_TOE},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{},
        std::vector<BODY_38_PARTS>{}
    };
    
}
