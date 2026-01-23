import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def caller_scenario(client, handler, target_nickname):
    """Caller scenario: call target"""
    print(f"[{handler.name}] Starting call to {target_nickname}")
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)


def responder_scenario(client, handler, caller_nickname):
    """Responder scenario: accept call from caller"""
    print(f"[{handler.name}] Waiting for incoming call...")
    if handler.wait_for_event(f"incoming_call_{caller_nickname}", 10):
        print(f"[{handler.name}] Accepting call from {caller_nickname}")
        client.accept_call(caller_nickname)
        handler.wait_for_event("accept_result_OK", 5)


class CallFlowTest(TestRunner):
    def test_call_flow(self):
        """Test complete call flow between two clients"""
        from test_runner_base import generate_unique_nickname
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        caller_nickname = generate_unique_nickname("caller")
        responder_nickname = generate_unique_nickname("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, responder_nickname, responder_events, "responder", partial(responder_scenario, caller_nickname=caller_nickname))
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, caller_nickname, caller_events, "caller", partial(caller_scenario, target_nickname=responder_nickname))
        )
        
        responder_process.start()
        time.sleep(2)
        caller_process.start()
        
        caller_process.join(timeout=15)
        responder_process.join(timeout=15)
        
        if caller_process.is_alive():
            caller_process.terminate()
        if responder_process.is_alive():
            responder_process.terminate()
        
        call_success = "call_result_OK" in caller_events
        accept_success = "accept_result_OK" in responder_events
        
        return call_success and accept_success


if __name__ == "__main__":
    test = CallFlowTest("8081")
    if test.start_server():
        try:
            result = test.test_call_flow()
            print(f"Call flow test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

