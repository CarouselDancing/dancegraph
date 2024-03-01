import os
import subprocess
import time

def run_cmd( client_idx, adapter_tick_ms, dancegraph_tick_ms ):
    cmd = ['..\\build\\bin\\client_server_telemetry.exe', str(client_idx), str(adapter_tick_ms), str(dancegraph_tick_ms)]
    cmd1 = " ".join(cmd)
    print("running command " + cmd1)
    #return subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    return subprocess.Popen(cmd, shell=True)


def run(adapter_tick_ms, dancegraph_tick_ms):
    pids = []
    pids.append( run_cmd( -1, adapter_tick_ms, dancegraph_tick_ms))
    time.sleep(2) # Allow NTP capture
    for i in range(4):
        pids.append(run_cmd(i,adapter_tick_ms, dancegraph_tick_ms))
        time.sleep(1)
        
    print("Done and waiting for closure")
    
    running_pids = list(range(5))
    while running_pids:
        time.sleep(1)
        running_pids = []
        i = 0
        for pid in pids:
            if pid.poll() is None:
                running_pids.append(str(i))
            i += 1
        print("running pids: " + ", ".join(running_pids))
    print("Done and all are closed")
        
def run_all( decoupled = False):
    dancegraph_ms_all = [0,1,2,4,8] if decoupled else [0]
    for adapter_ms in [16,32,64]:
        for dancegraph_ms in dancegraph_ms_all:
            run(adapter_ms, dancegraph_ms)
            time.sleep(1)

if __name__ == "__main__":
    
    run_all(False)