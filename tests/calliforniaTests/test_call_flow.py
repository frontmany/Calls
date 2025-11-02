import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def caller_scenario(client, handler):
    """Caller scenario: call responder"""
    print(f"[{handler.name}] Starting call to responder")
    client.start_calling("responder")
    handler.wait_for_event("call_result_OK", 10)


def responder_scenario(client, handler):
    """Responder scenario: accept call from caller"""
    print(f"[{handler.name}] Waiting for incoming call...")
    if handler.wait_for_event("incoming_call_caller", 10):
        print(f"[{handler.name}] Accepting call from caller")
        client.accept_call("caller")
        handler.wait_for_event("accept_result_OK", 5)


class CallFlowTest(TestRunner):
    def test_call_flow(self):
        """Test complete call flow between two clients"""
        caller_handler = CallbacksHandler("caller")
        responder_handler = CallbacksHandler("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "responder", responder_handler, responder_scenario)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "caller", caller_handler, caller_scenario)
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
        
        call_success = "call_result_OK" in caller_handler.events
        accept_success = "accept_result_OK" in responder_handler.events
        
        return call_success and accept_success


if __name__ == "__main__":
    test = CallFlowTest("8081")
    if test.start_server():
        try:
            result = test.test_call_flow()
            print(f"Call flow test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

