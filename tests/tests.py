import time
import multiprocessing
import sys
import importlib
import inspect

from test_runner_base import TestRunner

sys.path.append('C:/prj/Callifornia/out/build/x64-Release/clientPy')
import callsClientPy


def discover_and_run_all_tests():
    """Discovers and runs all tests from all test files"""
    test_files = [
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
        'test_screen_sharing',
        'test_camera_sharing',
        'test_logout',
        'test_end_call',
        'test_stop_outgoing_call',
        'test_state_queries',
        'test_multiple_incoming_calls',
    ]
    
    all_test_classes = []
    
    for test_file in test_files:
        try:
            module = importlib.import_module(test_file)
            for name, obj in inspect.getmembers(module):
                if inspect.isclass(obj) and issubclass(obj, TestRunner) and obj is not TestRunner:
                    all_test_classes.append((obj, test_file))
        except Exception as e:
            print(f"Failed to import {test_file}: {e}")
    
    if not all_test_classes:
        print("No test classes found!")
        return
    
    first_test_class = all_test_classes[0][0]
    runner = first_test_class("8081")
    
    if not runner.start_server():
        print("Failed to start server, exiting...")
        return
    
    print("\n" + "="*60)
    print("Running ALL tests from all test files")
    print("="*60)
    
    try:
        total_passed = 0
        total_tests = 0
        
        for test_class, module_name in all_test_classes:
            test_instance = test_class("8081")
            test_instance.server_process = runner.server_process
            test_instance.stop_event = runner.stop_event
            
            for method_name in dir(test_instance):
                if method_name.startswith('test_') and callable(getattr(test_instance, method_name)):
                    test_method = getattr(test_instance, method_name)
                    total_tests += 1
                    test_display_name = f"{module_name}.{method_name}"
                    if runner.run_test(test_method, test_display_name):
                        total_passed += 1
                    time.sleep(2)
        
        print(f"\n{'='*60}")
        print(f"FINAL RESULTS: {total_passed}/{total_tests} tests passed")
        print(f"{'='*60}")
        
    finally:
        runner.stop_server()


def run_basic_tests_only():
    """Run only basic tests (first 5)"""
    basic_test_files = [
        'test_authorization',
        'test_call_flow',
        'test_incoming_call_accept',
        'test_incoming_call_decline',
        'test_audio_controls',
    ]
    
    all_test_classes = []
    
    for test_file in basic_test_files:
        try:
            module = importlib.import_module(test_file)
            for name, obj in inspect.getmembers(module):
                if inspect.isclass(obj) and issubclass(obj, TestRunner) and obj is not TestRunner:
                    all_test_classes.append((obj, test_file))
        except Exception as e:
            print(f"Failed to import {test_file}: {e}")
    
    if not all_test_classes:
        print("No test classes found!")
        return
    
    first_test_class = all_test_classes[0][0]
    runner = first_test_class("8081")
    
    if not runner.start_server():
        print("Failed to start server, exiting...")
        return
    
    print("\n" + "="*60)
    print("Running BASIC tests")
    print("="*60)
    
    try:
        total_passed = 0
        total_tests = 0
        
        for test_class, module_name in all_test_classes:
            test_instance = test_class("8081")
            test_instance.server_process = runner.server_process
            test_instance.stop_event = runner.stop_event
            
            for method_name in dir(test_instance):
                if method_name.startswith('test_') and callable(getattr(test_instance, method_name)):
                    test_method = getattr(test_instance, method_name)
                    total_tests += 1
                    test_display_name = f"{module_name}.{method_name}"
                    if runner.run_test(test_method, test_display_name):
                        total_passed += 1
                    time.sleep(2)
        
        print(f"\n{'='*60}")
        print(f"RESULTS: {total_passed}/{total_tests} tests passed")
        print(f"{'='*60}")
        
    finally:
        runner.stop_server()


def main():
    discover_and_run_all_tests()

if __name__ == "__main__":
    main()
