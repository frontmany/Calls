import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def decline_caller_scenario(client, handler):
    """Caller scenario for decline test"""
    client.start_outgoing_call("responder")
    handler.wait_for_event("call_result_OK", 10)


def decline_responder_scenario(client, handler):
    """Responder scenario for decline test"""
    if handler.wait_for_event("incoming_call_caller", 10):
        print(f"[{handler.name}] Declining call")
        client.decline_call("caller")


class IncomingCallDeclineTest(TestRunner):
    def test_incoming_call_decline(self):
        """Test declining incoming call"""
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "responder", responder_events, "responder", decline_responder_scenario)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "caller", caller_events, "caller", decline_caller_scenario)
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
        
        return "incoming_call_caller" in responder_events


if __name__ == "__main__":
    test = IncomingCallDeclineTest("8081")
    if test.start_server():
        try:
            result = test.test_incoming_call_decline()
            print(f"Incoming call decline test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

