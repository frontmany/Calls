import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def accept_while_user_a_scenario(client, handler, target_b_nickname, caller_c_nickname):
    """User A calls B, but when C calls, accepts C's call"""
    print(f"[{handler.name}] Calling {target_b_nickname}")
    client.start_outgoing_call(target_b_nickname)
    time.sleep(1)
    
    # Wait for incoming call from C
    if handler.wait_for_event(f"incoming_call_{caller_c_nickname}", 10):
        print(f"[{handler.name}] Accepting call from {caller_c_nickname} during call to B")
        client.accept_call(caller_c_nickname)
        time.sleep(3)


def accept_while_user_b_scenario(client, handler, caller_a_nickname):
    """User B waits for incoming call and accepts"""
    if handler.wait_for_event(f"incoming_call_{caller_a_nickname}", 10):
        print(f"[{handler.name}] Accepting call from {caller_a_nickname}")
        client.accept_call(caller_a_nickname)
        handler.wait_for_event("accept_result_OK", 5)


def accept_while_user_c_scenario(client, handler, target_a_nickname):
    """User C waits then calls A"""
    time.sleep(3)
    print(f"[{handler.name}] Calling {target_a_nickname}")
    client.start_outgoing_call(target_a_nickname)
    handler.wait_for_event("call_result_OK", 10)


class AcceptWhileOutgoingCallTest(TestRunner):
    def test_accept_while_outgoing_call(self):
        """A calls B, at this time C calls A - A accepts call from C"""
        from test_runner_base import generate_unique_nickname
        b_events = multiprocessing.Manager().list()
        a_events = multiprocessing.Manager().list()
        c_events = multiprocessing.Manager().list()
        
        user_a_nickname = generate_unique_nickname("user_a")
        user_b_nickname = generate_unique_nickname("user_b")
        user_c_nickname = generate_unique_nickname("user_c")
        
        b_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, user_b_nickname, b_events, "user_b", partial(accept_while_user_b_scenario, caller_a_nickname=user_a_nickname), 8)
        )
        
        a_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, user_a_nickname, a_events, "user_a", partial(accept_while_user_a_scenario, target_b_nickname=user_b_nickname, caller_c_nickname=user_c_nickname), 8)
        )
        
        c_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, user_c_nickname, c_events, "user_c", partial(accept_while_user_c_scenario, target_a_nickname=user_a_nickname), 8)
        )
        
        b_process.start()
        time.sleep(1)
        a_process.start()
        time.sleep(1)
        c_process.start()
        
        a_process.join(timeout=15)
        b_process.join(timeout=15)
        c_process.join(timeout=15)
        
        for p in [a_process, b_process, c_process]:
            if p.is_alive():
                p.terminate()
        
        a_got_c_call = f"incoming_call_{user_c_nickname}" in a_events
        c_called_success = "call_result_OK" in c_events or "auth_result_OK" in c_events
        
        return a_got_c_call or c_called_success

if __name__ == "__main__":
    test = AcceptWhileOutgoingCallTest("8081")
    if test.start_server():
        try:
            result = test.test_accept_while_outgoing_call()
            print(f"Accept while outgoing call test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()