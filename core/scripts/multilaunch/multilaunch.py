
# Test script for automatic launching of multiple instances

import subprocess
import time
import argparse


# Unzip the testtracks to use


singlefiles = ["Record_zedv2_SingleTrack_141832_00.dgs",
               "Record_zedv2_SingleTrack_141832_03.dgs",
               "Record_zedv2_SingleTrack_143411_01.dgs",
               "Record_zedv2_SingleTrack_144904_00.dgs",
               "Record_zedv2_SingleTrack_150349_00.dgs",
               "Record_zedv2_SingleTrack_150349_03.dgs",
               "Record_zedv2_SingleTrack_151545_00.dgs"]






extrafiles = [         
    "Record_zedv2_SingleTrack_140006_00.dgs",
    "Record_zedv2_SingleTrack_140006_02.dgs",
    "Record_zedv2_SingleTrack_141858_00.dgs",
    "Record_zedv2_SingleTrack_143415_01.dgs",
    "Record_zedv2_SingleTrack_144915_00.dgs",
    "Record_zedv2_SingleTrack_151553_01.dgs"]


procs = []

listlookup = {"single" : singlefiles, # Long tracks containing a single person
              "extra" : extrafiles,   # Short tracks with only a single person in each
              }

parser = argparse.ArgumentParser()
parser.add_argument("--filelist", type = str, default = "single", help = "List of replay files")
parser.add_argument("--localport", type = int, help = "Local UDP port")
parser.add_argument("--server", nargs = 2)
parser.add_argument("--tpose", action = 'store_true', help = "Tposes only, rather than replay files")
parser.add_argument("instances", type = int, help = "Number of instances to spin up")
args = parser.parse_args()

files = listlookup[args.filelist]

if (args.localport is not None):
    baseport = args.localport
else:
    baseport = 9000
    
def cline(i, tposes = False):
    if (tposes):
        ans = ['./nettest', "--localport", str(baseport + 10 * i),
               "--loglevel", "off", "--mode", "client", "zed4_tpose"]
    else:
        ans = ['./nettest',  '--replay', "zed/v2.1", files[i%len(files)], "--localport", str(9000 + 10 * i),
               "--loglevel", "off", "--mode", "client", "zed_testreplay"]

    if (args.server is not None):
        ans.extend(["--server", args.server[0], args.server[1]])
        
    return ans

for i in range(args.instances):
    ans = cline(i, args.tpose)
    print(" ".join(ans))
    procs.append(subprocess.Popen(ans, stdout = subprocess.DEVNULL, stderr = subprocess.PIPE))
    time.sleep(1)

    
input("Press enter to kill:")

print("Killing!")
for p in procs:
    p.kill()

