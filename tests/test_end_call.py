import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def end_call_caller_scenario(client, handler):
    """Caller scenario: start call, wait for acceptance, then end call"""
    print(f"[{handler.name}] Starting call to responder")
    client.start_outgoing_call("responder")
    handler.wait_for_event("call_result_OK", 10)
    
    # Wait for call to be accepted
    if handler.wait_for_event("calling_accepted", 5):
        print(f"[{handler.name}] Call accepted, ending call")
        time.sleep(1)
        client.end_call()
        handler.wait_for_event("end_call_result_OK", 5)


def end_call_responder_scenario(client, handler):
    """Responder scenario: accept call, wait for remote end"""
    if handler.wait_for_event("incoming_call_caller", 10):
        print(f"[{handler.name}] Accepting call")
        client.accept_call("caller")
        handler.wait_for_event("accept_result_OK", 5)
        
        # Wait for remote user to end call
        handler.wait_for_event("remote_ended_call", 10)


class EndCallTest(TestRunner):
    def test_end_call(self):
        """Test ending an active call"""
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "responder", responder_events, "responder", end_call_responder_scenario, 10)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "caller", caller_events, "caller", end_call_caller_scenario, 10)
        )
        
        responder_process.start()
        time.sleep(2)
        caller_process.start()
        
        caller_process.join(timeout=20)
        responder_process.join(timeout=20)
        
        if caller_process.is_alive():
            caller_process.terminate()
        if responder_process.is_alive():
            responder_process.terminate()
        
        caller_ended = "end_call_result_OK" in caller_events
        responder_received_end = "remote_ended_call" in responder_events
        
        return caller_ended and responder_received_end


if __name__ == "__main__":
    test = EndCallTest("8081")
    if test.start_server():
        try:
            result = test.test_end_call()
            print(f"End call test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
