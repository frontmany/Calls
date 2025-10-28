import time
import multiprocessing
import sys

# Добавляем пути к модулям
sys.path.append('C:/prj/Callifornia/build/serverUpdaterPy/Debug')
import serverUpdaterPy

if __name__ == "__main__":
    serverUpdaterPy.runServerUpdater()