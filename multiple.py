import paramiko

codespaces = [
    {"hostname": "codespace1", "username": "user", "password": "pass"},
    {"hostname": "codespace2", "username": "user", "password": "pass"},
    # Add more Codespaces here
]

command = "your_command_here"

for cs in codespaces:
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(cs['hostname'], username=cs['username'], password=cs['password'])
    
    stdin, stdout, stderr = ssh.exec_command(command)
    print(f"Output from {cs['hostname']}: {stdout.read().decode()}")
    ssh.close()
