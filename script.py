import os
import sys
import subprocess
import shutil

def generate_makefile(n):
    makefile_content = """
# compile the program  
# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g 

# Source files
all : client nm"""

    for i in range(1, n+1):
        makefile_content += f" ss{i}"
        port = 8094 + (i-1) * 2

    makefile_content += """

client : client.c client.h
\t$(CC) $(CFLAGS) client.c -o client

nm : namingserver.c trie.c requests.c lru.c log.c lru.h trie.h requests.h namingserver.h
\t$(CC) $(CFLAGS) namingserver.c trie.c requests.c lru.c log.c -o nm"""

    for i in range(1, n+1):
        makefile_content += f"""

ss{i}: storageserver.c ss_functions.c lru.c lru.h ss_functions.h storageserver.h
\t$(CC) $(CFLAGS) storageserver.c ss_functions.c lru.c -o ss{i} -DPORT={port+2*i}"""

    makefile_content += """

clean:
\trm -f client nm """
    for i in range(1, n+1):
        makefile_content += f" ss{i}"

    with open("makefile", "w") as f:
        f.write(makefile_content)

def create_folders(n):
    for i in range(1, n+1):
        folder_name = f"dir{i}"
        if os.path.exists(folder_name):
            print(f"Folder '{folder_name}' already exists, overwriting.")
            shutil.rmtree(folder_name)
        os.makedirs(os.path.join(folder_name, folder_name), exist_ok=True)
        os.makedirs(os.path.join(folder_name, f"backup{i}"), exist_ok=True)
        with open(os.path.join(folder_name, "accessible_paths.txt"), "w") as f:
            f.write(f"backup{i}\n")
            f.write(f"dir{i}\n")

def build_targets():
    try:
        subprocess.run(["make"], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error building targets: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <n>")
        sys.exit(1)

    n = int(sys.argv[1])
    if os.path.exists("makefile"):
        print("Makefile already exists, overwriting.")
        os.remove("makefile")
    generate_makefile(n)
    create_folders(n)
    build_targets()
