import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def stop_outgoing_caller_scenario(client, handler, target_nickname):
    """Caller scenario: start call, then stop before acceptance"""
    print(f"[{handler.name}] Starting call to {target_nickname}")
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)
    
    # Wait a bit, then stop outgoing call
    time.sleep(1)
    print(f"[{handler.name}] Stopping outgoing call")
    client.stop_outgoing_call()
    handler.wait_for_event("stop_outgoing_result_OK", 5)


def stop_outgoing_responder_scenario(client, handler, caller_nickname):
    """Responder scenario: wait for call, but it should be stopped"""
    # Wait for incoming call
    if handler.wait_for_event(f"incoming_call_{caller_nickname}", 10):
        print(f"[{handler.name}] Received call, but caller may stop it")
        time.sleep(2)
        # Call might be stopped by caller, so we just wait


class StopOutgoingCallTest(TestRunner):
    def test_stop_outgoing_call(self):
        """Test stopping an outgoing call before it's accepted"""
        from test_runner_base import generate_unique_nickname
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        caller_nickname = generate_unique_nickname("caller")
        responder_nickname = generate_unique_nickname("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, responder_nickname, responder_events, "responder", partial(stop_outgoing_responder_scenario, caller_nickname=caller_nickname), 8)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, caller_nickname, caller_events, "caller", partial(stop_outgoing_caller_scenario, target_nickname=responder_nickname), 8)
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
        
        caller_stopped = "stop_outgoing_result_OK" in caller_events
        responder_received_call = f"incoming_call_{caller_nickname}" in responder_events
        
        return caller_stopped and responder_received_call


if __name__ == "__main__":
    test = StopOutgoingCallTest("8081")
    if test.start_server():
        try:
            result = test.test_stop_outgoing_call()
            print(f"Stop outgoing call test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
