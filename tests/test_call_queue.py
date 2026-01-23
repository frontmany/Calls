import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def queue_popular_user_scenario(client, handler):
    """Accept first call, decline others"""
    accepted = False
    processed_callers = set()  # Track processed callers
    for i in range(50):
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
        time.sleep(0.25)


def queue_caller_scenario(client, handler, target_nickname):
    """Caller scenario for queue test"""
    time.sleep(2)
    print(f"[{handler.name}] Calling {target_nickname}")
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)


class CallQueueTest(TestRunner):
    def test_incoming_call_queue(self):
        """Multiple users call one user, testing queue"""
        from test_runner_base import generate_unique_nickname
        popular_events = multiprocessing.Manager().list()
        popular_nickname = generate_unique_nickname("popular_user")
        
        popular_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, popular_nickname, popular_events, "popular_user", queue_popular_user_scenario, 10)
        )
        popular_process.start()
        time.sleep(2)
        
        caller_processes = []
        caller_events_list = []
        caller_nicknames = []
        
        for i in range(3):
            caller_nickname = generate_unique_nickname(f"caller_{i}")
            caller_nicknames.append(caller_nickname)
            events = multiprocessing.Manager().list()
            caller_events_list.append(events)
            
            process = multiprocessing.Process(
                target=run_client_flexible,
                args=("localhost", self.port, caller_nickname, events, caller_nickname, partial(queue_caller_scenario, target_nickname=popular_nickname))
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
        
        popular_got_calls = any(event.startswith("incoming_call_") 
                               for event in popular_events)
        call_attempts = sum(1 for events in caller_events_list 
                          if "call_result_OK" in events or "auth_result_OK" in events)
        
        return popular_got_calls and call_attempts >= 2

if __name__ == "__main__":
    test = CallQueueTest("8081")
    if test.start_server():
        try:
            result = test.test_incoming_call_queue()
            print(f"Call queue test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()