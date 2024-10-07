import argparse
import struct
import math
import json

from scipy.spatial.transform import Rotation

usefulbones34 = [2, 3, 5, 6, 12, 13, 18, 19, 22, 23, 26]
uselessbones34 = [0,1,4,7,8,9,10,11,14,15,16,17,20,21,24,25,27,28,29,30,31,32,33]

usefulbones38 = [1,2,3,4,10,11,12,13,14,15,16,17,18,19,20,21,22,23]



body_parts34 = [
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
]


body_parts38 = [
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
    "Right_Hand_Pinky_1"]


body_tpose34 = {
    0: (0.000000, 0.966564, 0.000000),
    1: (0.000000, 1.065799, -0.012273),
    2: (0.000000, 1.315855, -0.042761),
    3: (0.000000, 1.466180, -0.034832),
    4: (-0.061058, 1.406959, -0.035706),
    5: (-0.187608, 1.404300, -0.061715),
    6: (-0.461655, 1.404300, -0.061715),
    7: (-0.737800, 1.404300, -0.061715),
    10: (-0.856083, 1.341810, -0.017511),
    11: (0.061057, 1.406960, -0.035706),
    12: (0.187608, 1.404300, -0.061715),
    13: (0.461654, 1.404301, -0.061715),
    14: (0.737799, 1.404301, -0.061714),
    17: (0.856082, 1.341811, -0.017511),
    18: (-0.091245, 0.900000, -0.000554),
    19: (-0.093691, 0.494046, -0.005710),
    20: (-0.091244, 0.073566, -0.026302),
    21: (-0.094977, -0.031356, 0.100105),
    22: (0.091245, 0.900000, -0.000554),
    23: (0.093692, 0.494046, -0.005697),
    24: (0.091245, 0.073567, -0.026302),
    25: (0.094979, -0.031355, 0.100105),
    26: (0.000000, 1.569398, -0.003408)
}

body_relpose = {}

body_tree34 = {
    "Pelvis" : ["NavalSpine", "LeftHip", "RightHip"],
    "NavalSpine" : ["ChestSpine"],
    "ChestSpine": ["LeftClavicle", "RightClavicle", "Neck"],
    
    "Neck": ["Head"],
    
    "LeftClavicle": ["LeftShoulder"],             
    "LeftShoulder": ["LeftElbow"],
    "LeftElbow": ["LeftWrist"],
    "LeftWrist": ["LeftThumb"],              

    "RightClavicle": ["RightShoulder"],
    "RightShoulder": ["RightElbow"],
    "RightElbow": ["RightWrist"],
    "RightWrist": ["RightThumb"],                           

    "LeftHip": ["LeftKnee"],
    "LeftKnee": ["LeftAnkle"],
    "LeftAnkle": ["LeftFoot"],              
    
    "RightHip": ["RightKnee"],
    "RightKnee": ["RightAnkle"],
    "RightAnkle": ["RightFoot"],              
}



rpermute = {
    'x' : "Xrotation",
    'y' : "Yrotation",
    'z' : "Zrotation"
    }



def rot_string(p):
    return " ".join([rpermute[i] for i in p])

class Quantized_Quaternion:
    # Represent a quaternion with three 16-bit fixed-point ints
    def __init__(self, ints):
        self.fixed = ints

    def ToQuaternion(self):
        floats = [f / 32767 for f in self.fixed]
        sqrs = [f * f for f in floats]
        # print("Sqrs is ", sqrs)
        # print("Sumsq is %f"%(1.0 - sum(sqrs)))
        floats.append(math.sqrt(1.0 - sum(sqrs)))
        return Quaternion(floats)


class Quaternion:
    def __init__(self, floats):
        self.rot = Rotation.from_quat(floats)

    def toEuler(self, perm = 'xyz'):
        e = self.rot.as_euler(perm, degrees = True)
        return Euler([e[0], e[1], e[2]])

    def cstr(self, sep = ","):
        q = self.rot.as_quat()
        return (sep.join([str(i) for i in q]))
    
    def __str__(self):
        q = self.rot.as_quat()
        return (" ".join([str(i) for i in q]))

    def apply(self, x):
        return Position(self.rot.apply([x.y, x.x, x.z]))
    
class Euler:
    def __init__(self, floats):

        self.X = floats[0]
        self.Y = floats[1]
        self.Z = floats[2]
    def cstr(self, sep = ","):
        return (sep.join([str(i) for i in [self.X, self.Y, self.Z]]))
    
    def __str__(self):
        return (" ".join([str(i) for i in [self.X, self.Y, self.Z]]))
    


class Position:
    def __init__(self, floats):
        [self.x, self.y, self.z] = floats

    def cstr(self, sep = ","):
        return (sep.join([str(i) for i in [self.x, self.y, self.z]]))
    
    def __str__(self):
        return (" ".join([str(i) for i in [self.x, self.y, self.z]]))


    def scale(self, s):
        return Position([s * self.x , s * self.y, s * self.z])

    def __add__(self, a):
        return Position([self.x + a.x, self.y + a.y, self.z + a.z])

    def __sub__(self, a):
        return Position([self.x - a.x, self.y - a.y, self.z - a.z])


    
class Transform:
    def __init__(self, pos, ori):
        self.pos = pos
        self.ori = ori

    def cstrquat(self, sep=","):
        return "%s%s%s"%(self.pos.cstr(sep),sep,self.ori.cstr(sep))
    
    def cstr(self, sep=","):
        return "%s%s%s"%(self.pos.cstr(sep),sep,self.ori.toEuler(args.convert_order).cstr(sep))
    
    def __str__(self):
        return "%s %s"%(self.pos,self.ori.toEuler(args.convert_order))

    def scale(self, x):
        return Transform(self.pos.scale(x), self.ori)

    def offset_pos(self, p):
        return Transform(self.pos + p, self.ori)

class ZedSkelHeader:
    def __init__(self):
        pass

    def read(self, ifp):
        self.numBodies = struct.unpack('b', ifp.read(1))[0]
        self.grab_delta = struct.unpack('i', ifp.read(4))[0]
        self.track_delta = struct.unpack('i', ifp.read(4))[0]
        self.process_delta = struct.unpack('i', ifp.read(4))[0]
        self.padding = struct.unpack('bbb', ifp.read(3))
    
class SkeletonReader :
    # Just a base class; don't invoke this directly
    
    def __init__(self):
        self.bones = 0
        self.usefulbones = []
        self.header = ZedSkelHeader()

    def readquantquats(self, ifp, num):
        qq = struct.unpack('h' * num * 3, ifp.read(num * 6))
        return qq

    def readvecs(self, ifp, num):
        vecs = struct.unpack('i' * num * 3, ifp.read(num * 12))
        return vecs

    # Turns a sparse list of quant_quats into a fully-fleshed bonelist
    def generate_bonelist(self, quants):
        bonelist = [Quaternion([0.0, 0.0, 0.0, 1.0]) for i in range(self.bones)]
        for iidx, bidx in enumerate(self.usefulbones):
            bonelist[bidx] = (Quantized_Quaternion(quants[3 * iidx: 3 * iidx + 3]).ToQuaternion())

        return bonelist
        
    def _read_signal_frame_2_1(self, ifp, size, frame, keypoints = False):
        skelheader = self.header.read(ifp)
        bodyList = {}

        for i in range(self.header.numBodies):
            bonelist = [Quaternion([0.0, 0.0, 0.0, 1.0]) for i in range(self.bones)]
            skelID = struct.unpack('i', ifp.read(4))[0]
            root_pos = struct.unpack('fff', ifp.read(12))
            root_ori = struct.unpack('hhh', ifp.read(6))

            root_transform = Transform(Position(root_pos).scale(args.posscale), Quantized_Quaternion(root_ori).ToQuaternion())

            if (keypoints):
                kplist = self.readvecs(ifp, self.bones)
            else:
                kplist = None
            lub = len(self.usefulbones)
            quantfloats = self.readquantquats(ifp, lub)
            bonelist = self.generate_bonelist(quantfloats)
            bodyList[skelID] = [root_transform, bonelist, kplist]
        return bodyList

class SkeletonReaderCompact34(SkeletonReader):
    # id
    # xform_quant root_transform
    # std::array<quant_quat, NUM_BONES_COMPACT_34> bone_rotations
    def __init__(self):

        super(SkeletonReaderCompact34, self).__init__()
        
        self.bones = 34
        self.body_parts = body_parts34
        self.tpose = body_tpose34
        self.body_tree = body_tree34
        self.usefulbones = usefulbones34

    def read_signal_frame_2_1(self, ifp, size, frame):
        return self._read_signal_frame_2_1(ifp, size, frame, keypoints = False)

        
class SkeletonReaderCompact38(SkeletonReader):
    # id
    # xform_quant root_transform
    # std::array<quant_quat, NUM_BONES_COMPACT_38> bone_rotations

    def __init__(self):
        super(SkeletonReaderCompact38, self).__init__()        
        self.bones = 38
        self.body_parts = body_parts38
        self.tpose = body_tpose38
        self.body_tree = body_tree38
        self.usefulbones = usefulbones38


    def read_signal_frame_2_1(self, ifp, size, frame):
        return self._read_signal_frame_2_1(ifp, size, frame, keypoints = False)

    
class SkeletonReaderKPPlus34(SkeletonReader):
    # id
    # xform_quant root_transform
    # std::array<vec3, NUM_BONES_FULL_34> bone_keypoints
    # std::array<quant_quat, NUM_BONES_COMPACT_34> bone_rotations

    def __init__(self):
        super(SkeletonReaderKPPlus34, self).__init__()                
        self.bones = 34
        self.body_parts = body_parts34

        self.body_tree = body_tree34
        self.usefulbones = usefulbones34

        print("Warning, tpose not calculated from keypoints, using default")
        self.tpose = body_tpose34
        #self.tpose = self.calc_tpose()        

    def read_signal_frame_2_1(self, ifp, size, frame):
        return self._read_signal_frame_2_1(ifp, size, frame, keypoints = True)


class SkeletonReaderKPPlus38(SkeletonReader):
    # id
    # xform_quant root_transform
    # std::array<vec3, NUM_BONES_FULL_38> bone_keypoints
    # std::array<quant_quat, NUM_BONES_COMPACT_38> bone_rotations

    def __init__(self):
        super(SkeletonReaderKPPlus38, self).__init__()                        
        self.bones = 38
        self.body_parts = body_parts38

        self.body_tree = body_tree38
        self.usefulbones = usefulbones38

        print("Warning, tpose not calculated from keypoints, using default")
        self.tpose = body_tpose38
        #self.tpose = self.calc_tpose()        

    def read_signal_frame_2_1(self, ifp, size, frame):
        return self._read_signal_frame_2_1(ifp, size, frame, keypoints = True)

body_signals = {
    'Body_34Compact' : SkeletonReaderCompact34,
    'Body_38Compact' : SkeletonReaderCompact38,
    'Body_34_KeypointsPlus' : SkeletonReaderKPPlus34,
    'Body_38_KeypointsPlus' : SkeletonReaderKPPlus38,
    }

BODY_ZERO_POS = Position([0.0, 100.0, 0.0])

def fwrite(fp, x):
    fp.write(x)
    fp.write("\n")

def tabs(i):
    return '\t' * i

def outbvh(template, bodynum, csvtype = None):
    if (template[-4:] == '.bvh'):
        prefix = template[:-4]
    else:
        prefix = template

    if (csvtype == "euler"):
        return "%s_%03d_euler.csv"%(prefix, bodynum)
    elif(csvtype == "quat"):
        return "%s_%03d_quat.csv"%(prefix, bodynum)
    else:
        return "%s_%03d.bvh"%(prefix, bodynum)
    
def get_root_transform(floats):
    pos = Position(floats[:3])
    ori = Quaternion(floats[3:])
    return Transform(pos, ori)
        

def get_bodydata(floats):
    exes = floats[::4]
    ayes = floats[1::4]
    zeds = floats[2::4]
    wubs = floats[3::4]
    bdata = []
    for i in range(len(exes)):
        bdata.append(Quaternion([exes[i], ayes[i], zeds[i], wubs[i]]))
    return bdata

class BodyTrackFrame:
    def __init__(self, frame, root, body):
        self.frame = frame
        self.rootTransform = root
        self.quaternions = body

class BodyTrack:

    def __init__(self, bodyID, body_tpose, body_parts, body_tree, fps = 30):
        self.framedata = []
        self.ID = bodyID
        self.fps = fps
        self.body_tpose = body_tpose
        self.body_parts = body_parts
        self.body_tree = body_tree
        
    def add_frame(self, frame, roottrans, bdata):
        self.framedata.append(BodyTrackFrame(frame, roottrans, bdata))

    def write_header(self, fp):
        self.body_ordering = []
        root = "Pelvis"
        fwrite(fp, "HIERARCHY")
        self.walk_tree(fp, root, depth = 0, parentoffset = Position([0.0, 0.0, 0.0]))


    def walk_tree(self, fp, branch, depth = 0, parentoffset = None):

        if (depth == 0): 
            fwrite(fp, "ROOT %s"%branch)
        else:
            try:
                self.body_tree[branch]
                fwrite(fp, "%sJOINT %s"%(tabs(depth), branch))
            except(KeyError):
                try:
                    os_t = Position(self.body_tpose[self.body_parts.index(branch)])
                    offset = os_t.scale(args.bodyscale)
                    noffs = offset - parentoffset
                    
                    fwrite(fp, "%sEnd Site"%tabs(depth))
                    fwrite(fp, "%s{"%(tabs(depth)))
                    fwrite(fp, "%sOFFSET %s"%(tabs(depth + 1), noffs))
                    fwrite(fp, "%s}"%(tabs(depth)))
                    body_relpose[self.body_parts.index(branch)] = noffs
                    return
                except(KeyError):
                    pass
        fwrite(fp, "%s{"%(tabs(depth)))

        os_t = Position(self.body_tpose[self.body_parts.index(branch)])
        offset = os_t.scale(args.bodyscale)

        if (depth == 0):
            noffs = Position([0.0, 0.0, 0.0])
        else:
            noffs = offset - parentoffset

        fwrite(fp, "%sOFFSET %s"%(tabs(depth + 1), noffs))
        #fwrite(fp,  "%sOFFSET %s"%(tabs(depth + 1), parentoffset))
        body_relpose[self.body_parts.index(branch)] = noffs
        if (depth == 0):
            fwrite(fp, "%sCHANNELS 6 Xposition Yposition Zposition %s"%(tabs(depth + 1), rot_string(args.tree_order)))
        elif(args.keypoints):
            fwrite(fp, "%sCHANNELS 6 Xposition Yposition Zposition %s"%(tabs(depth + 1), rot_string(args.tree_order)))
            self.body_ordering.append(self.body_parts.index(branch))
        else:
            fwrite(fp, "%sCHANNELS 3 Zrotation Yrotation Xrotation"%tabs(depth + 1))
            self.body_ordering.append(self.body_parts.index(branch))

        for subbranch in self.body_tree[branch]:

            self.walk_tree(fp, subbranch, depth + 1, parentoffset = offset)
            #self.walk_tree(fp, subbranch, depth + 1, parentoffset = offset)

        fwrite(fp, "%s}"%(tabs(depth)))


    def rotpluskeypoint(self, q, frame):
        rot = frame.quaternions[q]
        return [rot.toEuler(perm = args.convert_order), body_relpose[q]]
        #return [rot.toEuler(), rot.apply(body_relpose[q])]
        
    def write(self, outfile, reposition = False):

        ofname = outbvh(outfile, self.ID)


        print("Writing body %d to %s"%(self.ID, ofname))
        ofp = open(ofname, "w")
        self.write_header(ofp)

        framecount = len(self.framedata)
        frametime = 1.0 / self.fps

        fwrite(ofp, "MOTION")
        fwrite(ofp, "Frames: %d"%framecount)
        fwrite(ofp, "Frame Time: %f"%frametime)

        if (args.normalize_position):
            position_offset = BODY_ZERO_POS - self.framedata[0].rootTransform.pos            
        else:
            position_offset = Position([0.0, 0.0, 0.0])

        for frame in self.framedata:
            if (args.keypoints):
                eulerdata = []
                for q in self.body_ordering:
                    reuler, rpos = self.rotpluskeypoint(q, frame)
                    eulerdata.append(body_relpose[q])
                    eulerdata.append(reuler)

            else:
                eulerdata = [frame.quaternions[q].toEuler(perm = args.convert_order) for q in self.body_ordering]
            fwrite(ofp, "%s %s"%(frame.rootTransform.offset_pos(position_offset), " ".join([str(e) for e in eulerdata])))
            
    def writecsvquat(self, outfile):

        headings = []

        for i in range(3):
            headings.append("RootPos_%s"%(['x', 'y', 'z'][i]))

        for i in range(4):
            headings.append("RootOri_%s"%(['x', 'y', 'z', 'w'][i]))

        #for i in range(34):
            
        for i in self.body_ordering:
            for j in range(4):
                headings.append("%s_%s"%(body_parts[i],['x', 'y', 'z', 'w'][j]))
                
        csvname = outbvh(outfile, self.ID, csvtype = "quat")
        ofp = open(csvname, "w")
        ofp.write(",".join(headings))
        ofp.write("\n")
        for frame in self.framedata:
            # Write the orientation in quaternion form
            ofp.write("%s,"%(frame.rootTransform.cstrquat(sep =",")))
            siftedquats = [frame.quaternions[q] for q in self.body_ordering]
            qst = [q.cstr(",") for q in siftedquats]
            ofp.write(",".join(qst))
            ofp.write("\n")

    def writecsveuler(self, outfile):
        headings = []
        for i in range(3):
            headings.append("RootPos_%s"%(['x', 'y', 'z'][i]))

        for i in range(3):
            headings.append("RootOri_%s"%(['x', 'y', 'z'][i]))
        
            #for i in range(34):
        for i in self.body_ordering:            
            for j in range(3):
                headings.append("%s_%s"%(body_parts[i],['x', 'y', 'z'][j]))

        csvname = outbvh(outfile, self.ID, csvtype = "euler")
        ofp = open(csvname, "w")
        ofp.write(",".join(headings))
        ofp.write("\n")
        for frame in self.framedata:
            ofp.write("%s,"%(frame.rootTransform.cstr(",")))
            eulers = [frame.quaternions[q].toEuler(perm = args.convert_order) for q in self.body_ordering]
            qst = [e.cstr(",") for e in eulers]
            ofp.write(",".join(qst))
            ofp.write("\n")

def old_convert():
    ifp = open(args.infile, 'rb')

    bodylist = {}
    framecounter = 0

    q = Quaternion([0.5, 0.0, 0.0, 0.866025])
    try:
        while(True):
            numbodies = struct.unpack('i', ifp.read(4))[0]
            align = struct.unpack('i', ifp.read(4))[0]

            if (align != 0):
                print("Warning, alignment byte is nonzero(%d); possible file corruption"%align)

            timestamp = struct.unpack('d', ifp.read(8))[0] # Double, but subject to change

            for i in range(numbodies):
                bodyID = struct.unpack('i', ifp.read(4))[0]
                rootTransform = get_root_transform(struct.unpack('f' * 7, ifp.read(4 * 7))).scale(args.posscale)
                bodydata = get_bodydata(struct.unpack('f' * 34 * 4, ifp.read(34 * 4 * 4)))

                try:
                    bodylist[bodyID].add_frame(framecounter, rootTransform, bodydata)
                except(KeyError):
                    bodylist[bodyID] = BodyTrack(bodyID, body_tpose34, body_parts34, body_tree34)
                    bodylist[bodyID].add_frame(framecounter, rootTransform, bodydata)

            ifp.seek( (1 + 7 + 34 * 4) * 4 * (args.maxbodies - numbodies), 1)
            framecounter += 1
    except(struct.error):
        pass
    ifp.close()
    return bodylist


def read_header(fp):
    # A magic string 'DGSAV'
    # followed by the length of the json string
    # followed by the json string
    # The json should be { "zed" : { ... some stuff ...} }
    
    magic = fp.read(5).decode('utf-8')
    if (magic) != "DGSAV":
        print("Error: not a Dancegraph .dgs file")
        return False
    jsonlen = struct.unpack('i', fp.read(4))[0]
    jsonopts = fp.read(jsonlen).decode('utf-8')
    iobj = json.loads(jsonopts)


    ff = [i for i in iobj.keys() if i[:3] == 'zed']
    
    if (len(ff) == 0):
        print("Error: not a zedcam .dgs file")
        return False    
    try:
        sigver = iobj['save-version']
        print("Version %s found"%sigver)
        return [save_versions[sigver], iobj]
    except(KeyError):
        print("No Save version, falling back to 0.1")
        return [save_versions["0.1"], iobj]
    return True



def read_signal_header(fp):
    # int userID
    # long long timestamp
    # int packetID
    # int dataSize
    
    userID = struct.unpack('i', fp.read(4))[0]
    blah = struct.unpack('i', fp.read(4))[0]
    timestamp = struct.unpack('q', fp.read(8))[0]

    packetID = struct.unpack('i', fp.read(4))[0]
    dataSize = struct.unpack('i', fp.read(4))[0]

    
def convert_0_1(ifp, jsopts = {}):
    bodylist = {}
    try:
        while(True):
            read_signal_header(ifp)

            numbodies = struct.unpack('i', ifp.read(4))[0]
            align = struct.unpack('i', ifp.read(4))[0]

            if (align != 0):
                print("Warning, alignment byte is nonzero(%d); possible file corruption"%align)

            timestamp = struct.unpack('d', ifp.read(8))[0] # Double, but subject to change

            for i in range(numbodies):
                bodyID = struct.unpack('i', ifp.read(4))[0]
                rootTransform = get_root_transform(struct.unpack('f' * 7, ifp.read(4 * 7))).scale(args.posscale)
                bodydata = get_bodydata(struct.unpack('f' * 34 * 4, ifp.read(34 * 4 * 4)))

                try:
                    bodylist[bodyID].add_frame(framecounter, rootTransform, bodydata)
                except(KeyError):
                    bodylist[bodyID] = BodyTrack(bodyID, body_tpose34, body_parts34, body_tree34)
                    bodylist[bodyID].add_frame(framecounter, rootTransform, bodydata)

            ifp.seek( (1 + 7 + 34 * 4) * 4 * (args.maxbodies - numbodies), 1)
            framecounter += 1
    except(struct.error):
        pass

    return bodylist

DGSAV_FRAME_MARKER = 1448298308

def read_frame_marker_0_2(ifp):
    if (args.verbose):
        print("Looking for frame marker at %x"%(ifp.tell()))
    magic = struct.unpack('I', ifp.read(4))
    if (magic[0] != DGSAV_FRAME_MARKER):
        print("Failed Frame marker at %x: %x"%(ifp.tell(), magic[0]))
        return None
    size = struct.unpack('i', ifp.read(4))
    return size

def read_signal_header_0_2(fp):

    timestamp = struct.unpack('q', fp.read(8))[0]    
    packetID = struct.unpack('i', fp.read(4))[0]
    userID = struct.unpack('h', fp.read(2))[0]
    sigIdx = struct.unpack('B', fp.read(1))[0]
    sigType = struct.unpack('B', fp.read(1))[0]
    #print("UserID: %d, timestamp: %d, packetID: %d, sigIdx: %d, sigType: %d"%(userID, timestamp, packetID, sigIdx, sigType))
    return 8 + 4 + 2 + 1 + 1

def read_signal_frame_0_2(ifp, size, frame):

    bodyList = {}
    numbodies = struct.unpack('b', ifp.read(1))[0]    
    padding1 = struct.unpack('b' * 7, ifp.read(7))[0]
    timestamp = struct.unpack('Q', ifp.read(8))[0]

    #print("BodyCount: %d, TS: %d"%(numbodies, timestamp))
    if (args.verbose):
        print("SigFrame: %x: %d bodies, time: %d "%(ifp.tell(), numbodies, timestamp))
    for i in range(numbodies):
        
        bonelist = [Quaternion([0.0, 0.0, 0.0, 1.0]) for i in range(34)]
    
        skelID = struct.unpack('i', ifp.read(4))[0]
        root_pos = struct.unpack('fff', ifp.read(12))
        root_ori = struct.unpack('hhh', ifp.read(6))
        
        root_transform = Transform(Position(root_pos).scale(args.posscale), Quantized_Quaternion(root_ori).ToQuaternion())

        lub = len(usefulbones)

        quantfloats = struct.unpack('h' * lub * 3, ifp.read(lub * 6))

        for iidx, bidx in enumerate(usefulbones):

            bonelist[bidx] = (Quantized_Quaternion(quantfloats[3 * iidx:3 * iidx + 3]).ToQuaternion())

        bodyList[skelID] = [root_transform, bonelist]
    return bodyList
            
def convert_0_2(ifp, jsopts = {}):

    bodyList = {}
    frame = 0
    print(jsopts.keys())
    skelreader = body_signals[jsopts['zedBodySignalType']]()
    
    try:
        while(True):
            try:
                itell = ifp.tell()
                size = read_frame_marker_0_2(ifp)[0]
                if(args.verbose):
                    print("Read frame from file@%x, size %d"%(itell, size))                
                if (size is None):
                    print("Frame Marker Read fail")
            except(struct.error):
                return bodyList
            else:
                headerSize = read_signal_header_0_2(ifp)
                #frameList = read_signal_frame_0_2(ifp, size - headerSize - 4, frame)
                frameList = skelreader.read_signal_frame_2_1(ifp, size - headerSize - 4, frame)
                
                for k in frameList:
                    roottrans, bodydata, bodykeypoints = frameList[k]
                    if k not in bodyList:
                        bodyList[k] = BodyTrack(k, skelreader.tpose, skelreader.body_parts, skelreader.body_tree)
                    bodyList[k].add_frame(frame, roottrans, bodydata)                        
                
                # try:
                #     for k in frameList.keys():
                #         roottrans, bodydata = frameList[k]
                #         bodyList[k].add_frame(frame, roottrans, bodydata)
                # except(KeyError):
                #     bodyList[k] = BodyTrack(k)
                #     roottrans, bodydata = frameList[k]
                #     bodyList[k].add_frame(frame, roottrans, bodydata)
            frame += 1
    except(KeyError):
        pass
    print("Bodylist has size %d"%len(bodyList))
    return bodyList
            
save_versions = {
    "0.1" : convert_0_1,
    "0.2" : convert_0_2
}

def convert(input_file):
    ifp = open(input_file, 'rb')
    hval, jsopts = read_header(ifp)
    if (not hval):
        return False

    toplevelkeys = [k for k in jsopts if k[:3] == 'zed']
    if (len(toplevelkeys) == 0):
        print("Can't find zed signal among json keys, quitting")
        exit(0)
    blist = hval(ifp, jsopts[toplevelkeys[0]])
    ifp.close()
    return blist

parser = argparse.ArgumentParser()

parser.add_argument("--csvquat", action = 'store_true', help = "Produce debug csv file of quaternions")
parser.add_argument("--csveuler", action = 'store_true', help = "Produce debug csv file of euler angles")
parser.add_argument("--bodyscale", type = float, default = 100.0, help = "Body scale")
parser.add_argument("--keypoints", action = "store_true", help = "Include position keypoint data in the bvh frame data")
parser.add_argument("--legacy", action = 'store_true', help = "Old style .dat file")
parser.add_argument("--posscale", type = float, default = 0.1, help = "Body scale")
parser.add_argument("--maxbodies", type = int, default = 10, help = "Maximum bodies in the .dat file")

# parser.add_argument("--tree_order", type = str, default = 'yzx', help = "Ordering of the rotation parts")
# parser.add_argument("--convert_order", type = str, default = 'yzx', help = "Ordering of the quaternion/rotation order")
parser.add_argument("--tree_order", type = str, default = 'zyx', help = "Ordering of the rotation parts")
parser.add_argument("--convert_order", type = str, default = 'zyx', help = "Ordering of the quaternion/rotation order")

parser.add_argument("--normalize_position", action = 'store_true', help = "Set the start position to be roughly (0.0, 0.0, 1.0)")

parser.add_argument("--verbose", action = 'store_true', help = "Spam more info")

parser.add_argument("infile", type = str, help = "Input file")
parser.add_argument("outfile", type = str, help = "Output file")
args = parser.parse_args()


if (args.legacy):
    bodylist = old_convert()

else:
    bodylist = convert(args.infile)

for bID in bodylist:

    bodylist[bID].write(args.outfile, reposition = args.normalize_position)
    
# The bvh file needs to be exported, because the body_ordering array needs to be built
if (args.csvquat):
    for bID in bodylist:
        bodylist[bID].writecsvquat(args.outfile)
    
if (args.csveuler):
    for bID in bodylist:
        bodylist[bID].writecsveuler(args.outfile)
    
