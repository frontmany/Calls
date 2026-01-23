import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def busy_user_b_scenario(client, handler, caller_c_nickname):
    """User B waits for call from C and accepts"""
    if handler.wait_for_event(f"incoming_call_{caller_c_nickname}", 10):
        print(f"[{handler.name}] Accepting call from {caller_c_nickname}")
        client.accept_call(caller_c_nickname)
        handler.wait_for_event("accept_result_OK", 5)


def busy_user_c_scenario(client, handler, target_b_nickname):
    """User C calls B"""
    print(f"[{handler.name}] Calling {target_b_nickname}")
    client.start_outgoing_call(target_b_nickname)
    handler.wait_for_event("call_result_OK", 10)


def busy_user_a_scenario(client, handler, target_b_nickname):
    """User A waits and then calls B who should be busy"""
    time.sleep(4)
    print(f"[{handler.name}] Calling {target_b_nickname} (should be busy)")
    client.start_outgoing_call(target_b_nickname)
    time.sleep(2)


class CallBusyUserTest(TestRunner):
    def test_call_busy_user(self):
        """A calls B who is already in call with C"""
        from test_runner_base import generate_unique_nickname
        b_events = multiprocessing.Manager().list()
        c_events = multiprocessing.Manager().list()
        a_events = multiprocessing.Manager().list()
        
        user_a_nickname = generate_unique_nickname("user_a")
        user_b_nickname = generate_unique_nickname("user_b")
        user_c_nickname = generate_unique_nickname("user_c")
        
        b_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, user_b_nickname, b_events, "user_b", partial(busy_user_b_scenario, caller_c_nickname=user_c_nickname), 8)
        )
        
        c_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, user_c_nickname, c_events, "user_c", partial(busy_user_c_scenario, target_b_nickname=user_b_nickname))
        )
        
        a_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, user_a_nickname, a_events, "user_a", partial(busy_user_a_scenario, target_b_nickname=user_b_nickname), 8)
        )
        
        b_process.start()
        time.sleep(1)
        c_process.start()
        time.sleep(2)
        a_process.start()
        
        a_process.join(timeout=15)
        b_process.join(timeout=15)
        c_process.join(timeout=15)
        
        for p in [a_process, b_process, c_process]:
            if p.is_alive():
                p.terminate()
        
        bc_success = ("call_result_OK" in c_events and
                     f"incoming_call_{user_c_nickname}" in b_events)
        
        return bc_success

if __name__ == "__main__":
    test = CallBusyUserTest("8081")
    if test.start_server():
        try:
            result = test.test_call_busy_user()
            print(f"Call busy user test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()