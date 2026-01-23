import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def multiple_incoming_popular_scenario(client, handler):
    """Popular user receives multiple calls, accepts first, declines others"""
    accepted = False
    processed_callers = set()  # Track processed callers
    for i in range(50):
        # Check for incoming calls
        for event in list(handler.events):
            if event.startswith("incoming_call_"):
                caller = event.replace("incoming_call_", "")
                if caller in processed_callers:
                    continue  # Already processed this caller
                
                if not accepted:
                    print(f"[{handler.name}] Accepting call from {caller}")
                    client.accept_call(caller)
                    handler.wait_for_event("accept_result_OK", 5)
                    accepted = True
                    processed_callers.add(caller)
                    # Don't remove event - keep it for result checking
                else:
                    print(f"[{handler.name}] Declining call from {caller}")
                    client.decline_call(caller)
                    processed_callers.add(caller)
                    # Don't remove event - keep it for result checking
        time.sleep(0.2)


def multiple_incoming_caller_scenario(client, handler, target_nickname):
    """Caller scenario: call popular user"""
    time.sleep(2)
    print(f"[{handler.name}] Calling {target_nickname}")
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)


class MultipleIncomingCallsTest(TestRunner):
    def test_multiple_incoming_calls(self):
        """Test handling multiple incoming calls"""
        from test_runner_base import generate_unique_nickname
        popular_events = multiprocessing.Manager().list()
        popular_nickname = generate_unique_nickname("popular_user")
        caller_nicknames = [generate_unique_nickname(f"caller_{i}") for i in range(3)]
        
        popular_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, popular_nickname, popular_events, "popular_user", multiple_incoming_popular_scenario, 12)
        )
        popular_process.start()
        time.sleep(2)
        
        caller_processes = []
        caller_events_list = []
        
        for caller_nickname in caller_nicknames:
            events = multiprocessing.Manager().list()
            caller_events_list.append(events)
            
            process = multiprocessing.Process(
                target=run_client_flexible,
                args=("localhost", self.port, caller_nickname, events, caller_nickname, partial(multiple_incoming_caller_scenario, target_nickname=popular_nickname))
            )
            caller_processes.append(process)
            process.start()
            time.sleep(1)
        
        popular_process.join(timeout=20)
        for process in caller_processes:
            process.join(timeout=10)
        
        if popular_process.is_alive():
            popular_process.terminate()
        for process in caller_processes:
            if process.is_alive():
                process.terminate()
        
        # Check that popular user received multiple calls
        incoming_calls = [e for e in popular_events if e.startswith("incoming_call_")]
        accept_success = "accept_result_OK" in popular_events
        
        return len(incoming_calls) >= 2 and accept_success


if __name__ == "__main__":
    test = MultipleIncomingCallsTest("8081")
    if test.start_server():
        try:
            result = test.test_multiple_incoming_calls()
            print(f"Multiple incoming calls test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
