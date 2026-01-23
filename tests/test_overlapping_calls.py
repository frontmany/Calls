import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def overlapping_caller_scenario(client, handler):
    """Generic caller scenario for overlapping tests"""
    # Target will be determined by client nickname context
    print(f"[{handler.name}] Waiting before call...")
    time.sleep(2)
    
    # Determine target based on handler name
    targets = {
        "user_a": "user_b",
        "user_c": "user_d",
        "user_b": "user_c",
        "user_d": "user_a"
    }
    target = targets.get(handler.name, "user_b")
    
    print(f"[{handler.name}] Calling {target}")
    client.start_outgoing_call(target)
    time.sleep(3)


class OverlappingCallsTest(TestRunner):
    def test_multiple_overlapping_calls(self):
        """Multiple overlapping calls between 4+ users"""
        call_scenarios = [
            ("user_a", "user_b"),
            ("user_c", "user_d"),
            ("user_b", "user_c"),
            ("user_d", "user_a"),
        ]
        
        processes = []
        events_list = []
        
        for caller, callee in call_scenarios:
            events = multiprocessing.Manager().list()
            events_list.append(events)
            
            process = multiprocessing.Process(
                target=run_client_flexible,
                args=("localhost", self.port, caller, events, caller, overlapping_caller_scenario, 8)
            )
            processes.append(process)
        
        for i, process in enumerate(processes):
            process.start()
            time.sleep(1.5)
        
        for process in processes:
            process.join(timeout=20)
        
        for process in processes:
            if process.is_alive():
                process.terminate()
        
        return True

if __name__ == "__main__":
    test = OverlappingCallsTest("8081")
    if test.start_server():
        try:
            result = test.test_multiple_overlapping_calls()
            print(f"Overlapping calls test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()