import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def overlapping_caller_scenario(client, handler, target_nickname):
    """Generic caller scenario for overlapping tests"""
    print(f"[{handler.name}] Waiting before call...")
    time.sleep(2)
    print(f"[{handler.name}] Calling {target_nickname}")
    client.start_outgoing_call(target_nickname)
    time.sleep(3)


class OverlappingCallsTest(TestRunner):
    def test_multiple_overlapping_calls(self):
        """Multiple overlapping calls between 4+ users"""
        from test_runner_base import generate_unique_nickname
        call_scenarios = [
            ("user_a", "user_b"),
            ("user_c", "user_d"),
            ("user_b", "user_c"),
            ("user_d", "user_a"),
        ]
        
        processes = []
        events_list = []
        nicknames = {}
        
        # Generate unique nicknames for all users
        for caller, callee in call_scenarios:
            if caller not in nicknames:
                nicknames[caller] = generate_unique_nickname(caller)
            if callee not in nicknames:
                nicknames[callee] = generate_unique_nickname(callee)
        
        for caller, callee in call_scenarios:
            caller_nick = nicknames[caller]
            callee_nick = nicknames[callee]
            events = multiprocessing.Manager().list()
            events_list.append(events)
            
            process = multiprocessing.Process(
                target=run_client_flexible,
                args=("localhost", self.port, caller_nick, events, caller, partial(overlapping_caller_scenario, target_nickname=callee_nick), 8)
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