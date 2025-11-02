"""
Validation script to check if all test files are properly configured
"""
import sys
import importlib
import inspect

def validate_test_files():
    """Validates that all test files can be imported and have correct structure"""
    
    test_files = [
        'test_runner_base',
        'tests',
        'test_authorization',
        'test_call_flow',
        'test_incoming_call_accept',
        'test_incoming_call_decline',
        'test_audio_controls',
        'test_accept_while_calling',
        'test_call_busy_user',
        'test_call_queue',
        'test_overlapping_calls',
        'test_rapid_redial',
        'test_triple_call',
    ]
    
    print("Validating test files...")
    print("=" * 60)
    
    try:
        base_module = importlib.import_module('test_runner_base')
        TestRunner = getattr(base_module, 'TestRunner', None)
        if not TestRunner:
            print("[FAIL] TestRunner not found in test_runner_base")
            return False
        print("[OK] test_runner_base.py")
    except Exception as e:
        print(f"[FAIL] test_runner_base.py: {e}")
        return False
    
    all_valid = True
    test_classes_found = []
    
    for test_file in test_files[2:]:
        try:
            module = importlib.import_module(test_file)
            
            found_test_class = False
            for name, obj in inspect.getmembers(module):
                if inspect.isclass(obj) and issubclass(obj, TestRunner) and obj is not TestRunner:
                    found_test_class = True
                    test_classes_found.append((test_file, name, obj))
                    
                    test_methods = [m for m in dir(obj) if m.startswith('test_') and callable(getattr(obj, m))]
                    if test_methods:
                        print(f"[OK] {test_file}.py (class: {name}, tests: {', '.join(test_methods)})")
                    else:
                        print(f"[WARN] {test_file}.py: No test methods found in {name}")
            
            if not found_test_class:
                print(f"[WARN] {test_file}.py: No TestRunner subclass found")
                all_valid = False
                
        except ImportError as e:
            print(f"[FAIL] {test_file}.py - IMPORT ERROR: {e}")
            all_valid = False
        except Exception as e:
            print(f"[FAIL] {test_file}.py - ERROR: {e}")
            all_valid = False
    
    print("=" * 60)
    print(f"\nFound {len(test_classes_found)} test class(es) in {len(test_files) - 2} test files (11 tests total)")
    
    if all_valid:
        print("\n[SUCCESS] All test files are properly configured!")
        print("\nYou can now run:")
        print("  python tests.py          - Run basic tests")
        print("  python tests.py --all    - Run all tests")
    else:
        print("\n[WARNING] Some test files have issues. Please review the warnings above.")
    
    return all_valid

if __name__ == "__main__":
    success = validate_test_files()
    sys.exit(0 if success else 1)

