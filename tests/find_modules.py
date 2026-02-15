"""
Helper script to find Python binding modules
"""
import os
import sys

def find_modules():
    """Find where Python binding modules are located"""
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_release = os.path.join(base_dir, 'out', 'build', 'x64-Release')
    out_debug = os.path.join(base_dir, 'out', 'build', 'x64-Debug')

    possible_client_paths = [
        os.path.join(out_release, 'clientCorePythonWrapper'),
        os.path.join(out_debug, 'clientCorePythonWrapper'),
    ]

    possible_server_paths = [
        os.path.join(out_release, 'serverPythonWrapper'),
        os.path.join(out_debug, 'serverPythonWrapper'),
    ]
    
    print("Searching for callsClientPy module...")
    print("=" * 60)
    found_client = False
    for path in possible_client_paths:
        abs_path = os.path.abspath(path)
        exists = os.path.exists(abs_path)
        print(f"{'[FOUND]' if exists else '[NOT FOUND]'} {abs_path}")
        if exists:
            files = os.listdir(abs_path)
            module_files = [f for f in files if f.startswith('callsClientPy')]
            if module_files:
                print(f"  -> Module files: {', '.join(module_files)}")
                found_client = True
                # Try to import
                if abs_path not in sys.path:
                    sys.path.insert(0, abs_path)
                try:
                    import callsClientPy
                    print(f"  -> ✓ Successfully imported!")
                except Exception as e:
                    print(f"  -> ✗ Import failed: {e}")
    
    print("\nSearching for callsServerPy module...")
    print("=" * 60)
    found_server = False
    for path in possible_server_paths:
        abs_path = os.path.abspath(path)
        exists = os.path.exists(abs_path)
        print(f"{'[FOUND]' if exists else '[NOT FOUND]'} {abs_path}")
        if exists:
            files = os.listdir(abs_path)
            module_files = [f for f in files if f.startswith('callsServerPy')]
            if module_files:
                print(f"  -> Module files: {', '.join(module_files)}")
                found_server = True
                # Try to import
                if abs_path not in sys.path:
                    sys.path.insert(0, abs_path)
                try:
                    import callsServerPy
                    print(f"  -> ✓ Successfully imported!")
                except Exception as e:
                    print(f"  -> ✗ Import failed: {e}")
    
    print("\n" + "=" * 60)
    if found_client and found_server:
        print("✓ Both modules found!")
    elif found_client:
        print("⚠ Only callsClientPy found")
    elif found_server:
        print("⚠ Only callsServerPy found")
    else:
        print("✗ Modules not found. Please build the project first.")
        print("\nTo build Python bindings, run CMake and build the project.")
        print("The modules should be generated in one of the paths above.")

if __name__ == "__main__":
    find_modules()
