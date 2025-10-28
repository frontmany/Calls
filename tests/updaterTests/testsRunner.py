import subprocess
import time
import sys
import os

def run_multiple_files():
    sys.path.append('C:/prj/Callifornia/build/serverUpdaterPy/Release')
    sys.path.append('C:/prj/Callifornia/build/clientUpdaterPy/Release')

    server_process = subprocess.Popen([
        sys.executable, 
        "C:/prj/Callifornia/tests/updaterTests/serverUpdater/serverUpdater.py"
    ])
    
    time.sleep(2)

    client_process = subprocess.Popen([
        sys.executable, 
        "C:/prj/Callifornia/tests/updaterTests/clientUpdater/clientUpdater.py"
    ])
    
    print("✅ Оба процесса запущены")
    print("Сервер PID:", server_process.pid)
    print("Клиент PID:", client_process.pid)
    try:
        server_process.wait()
        client_process.wait()
    except KeyboardInterrupt:
        print("\n🛑 Остановка процессов...")
        server_process.terminate()
        client_process.terminate()

if __name__ == "__main__":
    run_multiple_files()