import paramiko

ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect('192.168.1.150', username='hoanglong', password='258456')

def get(cmd):
    _, out, _ = ssh.exec_command(cmd)
    return out.read().decode().strip()

print("CPU:", get("lscpu | grep 'Model name' | head -n 1"))
print("Cores:", get("nproc"))
print("RAM:", get("free -h | grep Mem"))
print("OS:", get("lsb_release -d"))
print("Kernel:", get("uname -r"))
print("GCC:", get("gcc --version | head -n 1"))
print("Go:", get("go version"))
print("Rust:", get("rustc --version"))
ssh.close()
