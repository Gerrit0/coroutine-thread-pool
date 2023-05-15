import subprocess

proc = subprocess.Popen(["./bin/ws_debug"])
top = subprocess.Popen(["/usr/bin/top", "-b", "-p", str(proc.pid)], stdout=subprocess.PIPE)
proc.wait()
top.kill()

for line in top.stdout:
    print(line)
