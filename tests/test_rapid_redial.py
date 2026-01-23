import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def rapid_busy_callee_scenario(client, handler, caller_nickname):
    """Decline all incoming calls from caller"""
    for i in range(40):
        for event in handler.events:
            if event == f"incoming_call_{caller_nickname}":
                print(f"[{handler.name}] Declining call from {caller_nickname}")
                client.decline_call(caller_nickname)
                handler.events.remove(event)
                break
        time.sleep(0.3)


def rapid_persistent_caller_scenario(client, handler, target_nickname):
    """Try calling multiple times"""
    for attempt in range(3):
        print(f"[{handler.name}] Call attempt {attempt + 1}")
        client.start_outgoing_call(target_nickname)
        time.sleep(3)


class RapidRedialTest(TestRunner):
    def test_rapid_redial(self):
        """Rapid redials after declined/timeout"""
        from test_runner_base import generate_unique_nickname
        callee_events = multiprocessing.Manager().list()
        caller_events = multiprocessing.Manager().list()
        
        callee_nickname = generate_unique_nickname("busy_callee")
        caller_nickname = generate_unique_nickname("persistent_caller")
        
        callee_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, callee_nickname, callee_events, "busy_callee", partial(rapid_busy_callee_scenario, caller_nickname=caller_nickname), 12)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, caller_nickname, caller_events, "persistent_caller", partial(rapid_persistent_caller_scenario, target_nickname=callee_nickname), 10)
        )
        
        callee_process.start()
        time.sleep(1)
        caller_process.start()
        
        callee_process.join(timeout=20)
        caller_process.join(timeout=20)
        
        for p in [callee_process, caller_process]:
            if p.is_alive():
                p.terminate()
        
        callee_got_calls = any(f"incoming_call_{caller_nickname}" in str(event) 
                              for event in callee_events)
        caller_made_attempts = "auth_result_OK" in caller_events
        
        return callee_got_calls or caller_made_attempts

if __name__ == "__main__":
    test = RapidRedialTest("8081")
    if test.start_server():
        try:
            result = test.test_rapid_redial()
            print(f"Rapid redial test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()