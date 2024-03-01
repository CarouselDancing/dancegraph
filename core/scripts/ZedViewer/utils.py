import math
import struct

import numpy as np

def unquantize(x):
    return (x / 32767)

class Quaternion:
    def __init__(self, x, y, z, w = None):
        self.x = x
        self.y = y
        self.z = z
        if (w is None):
            self.w = math.sqrt(1.0 - self.x * self.x - self.y * self.y - self.z * self.z)
        else:
            self.w = w

        self.type = 'quat'
        
    def __mul__(self,q):

        if (q.type == 'quat'):
            nw = -self.x * q.x - self.y * q.y - self.z * q.z + self.w * q.w
            nx = self.x * q.w + self.y * q.z - self.z * q.y + self.w * q.x
            ny = -self.x * q.z + self.y * q.w + self.z * q.x + self.w * q.y
            nz = self.x * q.y - self.y * q.x + self.z * q.w + self.w * q.z
            return Quaternion(nx, ny, nz, nw)
        else:
            nx = 2*(self.w * q.z * self.y + self.x * q.z * self.z - self.w * q.y * self.z + self.x * q.y * self.y) + q.x*(self.w * self.w + self.x * self.x - self.y * self.y - self.z * self.z);
            ny = 2*(self.w * q.x * self.z + self.x * q.x * self.y - self.w * q.z * self.x + self.y * q.z * self.z) + q.y*(self.w * self.w - self.x * self.x + self.y * self.y - self.z * self.z);
            nz = 2*(self.w * q.y * self.x - self.w * q.x * self.y + self.x * q.x * self.z + self.y * q.y * self.z) + q.z*(self.w * self.w - self.x * self.x - self.y * self.y + self.z * self.z);
            return Vector(nx, ny, nz)            



# // inside quaterion class
# // quaternion defined as (r, i, j, k)
# Vector3 rotateVector(const Vector3 & _V)const
#     Vector3 vec();   // any constructor will do
#     vec.x = 2*(r*_V.z*j + i*_V.z*k - r*_V.y*k + i*_V.y*j) + _V.x*(r*r + i*i - j*j - k*k);
#     vec.y = 2*(r*_V.x*k + i*_V.x*j - r*_V.z*i + j*_V.z*k) + _V.y*(r*r - i*i + j*j - k*k);
#     vec.z = 2*(r*_V.y*i - r*_V.x*j + i*_V.x*k + j*_V.y*k) + _V.z*(r*r - i*i - j*j + k*k);
#     return vec;
# }

#     qector3 vec();   // any constructor will do
#     nx = 2*(self.w * q.z * self.y + self.x * q.z*self.z - self.w * q.y * self.z + self.x * q.y * self.y) + q.x*(self.w * self.w + self.x * self.x - self.y * self.y - self.z * self.z);
#     ny = 2*(self.w * q.x * self.z + self.x * q.x*self.y - self.w * q.z * self.x + self.y * q.z * self.z) + q.y*(self.w * self.w - self.x * self.x + self.y * self.y - self.z * self.z);
#     nz = 2*(self.w * q.y * self.x - self.w * q.x*self.y + self.x * q.x * self.z + self.y * q.y * self.z) + q.z*(self.w * self.w - self.x * self.x - self.y * self.y + self.z * self.z);
#     return vec;
# }

    


        
    def read_quant(fp):
        xv = unquantize(struct.unpack('h', fp.read(2))[0])
        yv = unquantize(struct.unpack('h', fp.read(2))[0])
        zv = unquantize(struct.unpack('h', fp.read(2))[0])


        nsq = 1 - xv*xv -yv * yv - zv * zv
        if (nsq < 0.0):
            wv = 0.0
        else:
            wv = math.sqrt(nsq)
        return Quaternion(xv, yv, zv, wv)
    
    def read(fp):
        xv = struct.unpack('h', fp.read(2))[0]
        yv = struct.unpack('h', fp.read(2))[0]
        zv = struct.unpack('h', fp.read(2))[0]
        wv = struct.unpack('h', fp.read(2))[0]       
        return Quaternion(xv, yv, zv, wv)

    def to_matrix(self):
        q0, q1, q2, q3 = [xv,yv,zv,wv]
        rotation_matrix = np.array([
            [1 - 2*(q2**2 + q3**2), 2*(q1*q2 - q0*q3), 2*(q1*q3 + q0*q2)],
            [2*(q1*q2 + q0*q3), 1 - 2*(q1**2 + q3**2), 2*(q2*q3 - q0*q1)],
            [2*(q1*q3 - q0*q2), 2*(q2*q3 + q0*q1), 1 - 2*(q1**2 + q2**2)]
        ])
        return rotation_matrix
    
    def __str__(self):
        return "quat[%f, %f, %f, %f]"%(self.x, self.y, self.z, self.w)

    def identity():
        return Quaternion(0.0, 0.0, 0.0, 1.0)

    def inv(self):
        return Quaternion(-self.x, -self.y, -self.z, self.w)
    
    
class Vector:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        self.w = 0
        self.type = 'vec'        

    def scalmul(self, s):
        return Vector(s * self.x, s * self.y, s * self.z)
        
    def __str__(self):
        return "[%f, %f, %f]"%(self.x, self.y, self.z)

    def normalize(self):
        magsq = self.x * self.x + self.y * self.y + self.z * self.z
        if (magsq != 0.0):
            mag = math.sqrt(mag)
            return Vector(self.x / mag, self.y / mag, self.z/mag)
        else:
            return self

    def __add__(self, nv):
        return Vector(self.x + nv.x, self.y + nv.y, self.z + nv.z)

    def __sub__(self, nv):
        return Vector(self.x - nv.x, self.y - nv.y, self.z - nv.z)

    def mag(self):
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)        

    def zero():
        return Vector(0.0, 0.0, 0.0)
    
    def read(fp):
        xv = struct.unpack('f', fp.read(4))[0]
        yv = struct.unpack('f', fp.read(4))[0]
        zv = struct.unpack('f', fp.read(4))[0]
        return Vector(xv, yv, zv)

    def np(self):
        return np.array([self.x, self.y, self.z])
    
# For optimization, the tree is assumed that a bone always appears as a parent after all it's child references in the list
def traverse_tree(tree, root, fn):
    print("Traversing %s"%root)
    try:
        children = tree[root]
        for child in children:
            yield fn(child, root)        
            for t in traverse_tree(tree, child, fn):
                yield t
    except(KeyError):
        pass


## ChatGPT follows    

import numpy as np

def quaternion_to_rotation_matrix(q):
    """
    Convert a quaternion to a 3x3 rotation matrix.
    """
    q0, q1, q2, q3 = q
    rotation_matrix = np.array([
        [1 - 2*(q2**2 + q3**2), 2*(q1*q2 - q0*q3), 2*(q1*q3 + q0*q2)],
        [2*(q1*q2 + q0*q3), 1 - 2*(q1**2 + q3**2), 2*(q2*q3 - q0*q1)],
        [2*(q1*q3 - q0*q2), 2*(q2*q3 + q0*q1), 1 - 2*(q1**2 + q2**2)]
    ])
    return rotation_matrix

def apply_quaternions_to_body(quaternions, keypoints_local):
    """
    Recursively apply quaternions to a body's joints in local space to generate new keypoints in world space.
    Args:
        quaternions (list of numpy arrays): List of quaternions for each joint.
        keypoints_local (list of numpy arrays): List of keypoints in local space.
    Returns:
        keypoints_world (list of numpy arrays): List of keypoints in world space.
    """
    if len(quaternions) != len(keypoints_local):
        raise ValueError("Length of quaternions list must match length of keypoints_local list.")

    # Initialize the root transformation matrix to the identity matrix
    root_matrix = np.identity(3)

    def recursive_transform(joint_index):
        nonlocal root_matrix
        joint_local = keypoints_local[joint_index]

        # Apply the local joint transformation
        rotation_matrix = quaternion_to_rotation_matrix(quaternions[joint_index])
        joint_world = np.dot(root_matrix, joint_local)  # Transform to world space

        # Calculate the new root matrix for the next level of recursion
        root_matrix = np.dot(root_matrix, rotation_matrix)

        # Recursively transform child joints
        if joint_index + 1 < len(quaternions):
            child_world = recursive_transform(joint_index + 1)
            joint_world += child_world  # Add child's contribution

        return joint_world

    # Start the recursive transformation from the root joint (index 0)
    keypoints_world = [recursive_transform(0)]

    return keypoints_world

# Example usage:
if __name__ == "__main__":
    # Sample quaternions and local keypoints for a body with 3 joints
    quaternions = [
        np.array([1.0, 0.0, 0.0, 0.0]),  # Identity quaternion for the root joint
        np.array([0.7071, 0.0, 0.0, 0.7071]),  # Example quaternion for joint 1
        np.array([0.8660, 0.0, 0.0, 0.5])  # Example quaternion for joint 2
    ]
    keypoints_local = [
        np.array([0.0, 0.0, 0.0]),
        np.array([1.0, 0.0, 0.0]),
        np.array([1.0, 0.0, 0.0])
    ]

    # Apply quaternions to generate new keypoints in world space
    keypoints_world = apply_quaternions_to_body(quaternions, keypoints_local)

    # Print the resulting keypoints in world space
    for i, keypoint in enumerate(keypoints_world):
        print(f"Keypoint {i} (World Space): {keypoint}")        
