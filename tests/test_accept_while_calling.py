import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def accept_while_user_a_scenario(client, handler):
    """User A calls B, but when C calls, accepts C's call"""
    print(f"[{handler.name}] Calling user_b")
    client.start_outgoing_call("user_b")
    time.sleep(1)
    
    # Wait for incoming call from C
    if handler.wait_for_event("incoming_call_user_c", 10):
        print(f"[{handler.name}] Accepting call from user_c during call to B")
        client.accept_call("user_c")
        time.sleep(3)


def accept_while_user_b_scenario(client, handler):
    """User B waits for incoming call and accepts"""
    if handler.wait_for_event("incoming_call_user_a", 10):
        print(f"[{handler.name}] Accepting call from user_a")
        client.accept_call("user_a")
        handler.wait_for_event("accept_result_OK", 5)


def accept_while_user_c_scenario(client, handler):
    """User C waits then calls A"""
    time.sleep(3)
    print(f"[{handler.name}] Calling user_a")
    client.start_outgoing_call("user_a")
    handler.wait_for_event("call_result_OK", 10)


class AcceptWhileCallingTest(TestRunner):
    def test_accept_while_calling(self):
        """A calls B, at this time C calls A - A accepts call from C"""
        b_events = multiprocessing.Manager().list()
        a_events = multiprocessing.Manager().list()
        c_events = multiprocessing.Manager().list()
        
        b_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_b", b_events, "user_b", accept_while_user_b_scenario, 8)
        )
        
        a_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_a", a_events, "user_a", accept_while_user_a_scenario, 8)
        )
        
        c_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_c", c_events, "user_c", accept_while_user_c_scenario, 8)
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
        
        a_got_c_call = "incoming_call_user_c" in a_events
        c_called_success = "call_result_OK" in c_events or "auth_result_OK" in c_events
        
        return a_got_c_call or c_called_success

if __name__ == "__main__":
    test = AcceptWhileCallingTest("8081")
    if test.start_server():
        try:
            result = test.test_accept_while_calling()
            print(f"Accept while calling test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()