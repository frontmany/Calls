import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def triple_user_a_scenario(client, handler):
    """User A calls B"""
    print(f"[{handler.name}] Calling user_b")
    client.start_outgoing_call("user_b")
    handler.wait_for_event("call_result_OK", 10)


def triple_user_b_scenario(client, handler):
    """User B receives call from A, then calls C"""
    if handler.wait_for_event("incoming_call_user_a", 10):
        print(f"[{handler.name}] Received call from user_a")
        print(f"[{handler.name}] Now calling user_c")
        client.start_outgoing_call("user_c")
        handler.wait_for_event("call_result_OK", 10)


def triple_user_c_scenario(client, handler):
    """User C waits for incoming call and accepts"""
    print(f"[{handler.name}] Waiting for incoming call...")
    time.sleep(0.5)
    
    # Wait for call from either user_a or user_b
    for i in range(30):
        for event in handler.events:
            if event.startswith("incoming_call_"):
                caller = event.replace("incoming_call_", "")
                print(f"[{handler.name}] Accepting call from {caller}")
                client.accept_call(caller)
                handler.wait_for_event("accept_result_OK", 5)
                return
        time.sleep(0.3)


class TripleCallTest(TestRunner):
    def test_triple_call_chain(self):
        """A calls B, B calls C, establishes A-C connection"""
        a_events = multiprocessing.Manager().list()
        b_events = multiprocessing.Manager().list()
        c_events = multiprocessing.Manager().list()
        
        c_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_c", c_events, "user_c", triple_user_c_scenario, 8)
        )
        
        b_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_b", b_events, "user_b", triple_user_b_scenario, 8)
        )
        
        a_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_a", a_events, "user_a", triple_user_a_scenario)
        )
        
        c_process.start()
        time.sleep(1)
        b_process.start()
        time.sleep(2)
        a_process.start()
        
        a_process.join(timeout=15)
        b_process.join(timeout=15)
        c_process.join(timeout=15)
        
        for p in [a_process, b_process, c_process]:
            if p.is_alive():
                p.terminate()
        
        a_success = "call_result_OK" in a_events
        b_received = "incoming_call_user_a" in b_events
        c_received = any(event.startswith("incoming_call_") for event in c_events)
        
        return a_success and b_received and c_received

if __name__ == "__main__":
    test = TripleCallTest("8081")
    if test.start_server():
        try:
            result = test.test_triple_call_chain()
            print(f"Triple call test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()