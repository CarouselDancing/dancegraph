
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



demodancer = ["DemoDancer.dgs"]


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
              "demo" : demodancer
              }

parser = argparse.ArgumentParser()
parser.add_argument("--filelist", type = str, default = "demo", help = "List of replay files")
parser.add_argument("--localport", type = int, default = 9000, help = "Local UDP port")
parser.add_argument("--server", nargs = 2)
parser.add_argument("--timedelay", type = float, default = 0.05, help = "Delay between avatar spinups")
parser.add_argument("--tpose", action = 'store_true', help = "Tposes only, rather than replay files")
parser.add_argument("--verbose", action = 'store_true', help = "Show output")
parser.add_argument("--preset", type = str, default = "zed4_env_replaysignal", help = "Zed Runtime Config Preset to use")
parser.add_argument("--loglevel", type = str, default = 'off', help = "Log level of the output")
parser.add_argument("instances", type = int, help = "Number of instances to spin up")
args = parser.parse_args()

files = listlookup[args.filelist]

if (args.tpose):
    preset = "zed4_env_tpose"
else:
    preset = args.preset


def cline(i, tposes = False):
    if (tposes):
        ans = ['./nettest', "--localport", str(args.localport + 10 * i),
               "--loglevel", args.loglevel, "--mode", "client", preset]
    else:
        ans = ['./nettest',  '--replay', "zed/v2.1", files[i%len(files)], "--localport", str(args.localport + 10 * i),
               "--loglevel", args.loglevel, "--mode", "client", preset]

    if (args.server is not None):
        ans.extend(["--server", args.server[0], args.server[1]])
        
    return ans

for i in range(args.instances):
    ans = cline(i, args.tpose)
    print(" ".join(ans))
    if (args.verbose):
        procs.append(subprocess.Popen(ans, stderr = subprocess.PIPE))
    else:
        procs.append(subprocess.Popen(ans, stdout = subprocess.DEVNULL, stderr = subprocess.PIPE))
    time.sleep(args.timedelay)

    
input("Press enter to kill:")

print("Killing!")
for p in procs:
    p.kill()

