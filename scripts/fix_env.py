import os

MINGW_PATH = r"C:\msys64\mingw64\bin"

env_path = os.environ.get("PATH", "")

if MINGW_PATH not in env_path:
    os.environ["PATH"] = MINGW_PATH + ";" + env_path

print("[DEBUG] Effective PATH:")
print(os.environ["PATH"])