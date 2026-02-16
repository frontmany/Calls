import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def end_call_caller_scenario(client, handler, target_nickname):
    """Caller scenario: start call, wait for acceptance, then end call"""
    print(f"[{handler.name}] Starting call to {target_nickname}")
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)
    
    # Wait for call to be accepted
    if handler.wait_for_event("outgoing_call_accepted", 5):
        print(f"[{handler.name}] Call accepted, ending call")
        time.sleep(1)
        client.end_call()
        handler.wait_for_event("end_call_result_OK", 5)


def end_call_responder_scenario(client, handler, caller_nickname):
    """Responder scenario: accept call, wait for remote end"""
    if handler.wait_for_event(f"incoming_call_{caller_nickname}", 10):
        print(f"[{handler.name}] Accepting call")
        client.accept_call(caller_nickname)
        handler.wait_for_event("accept_result_OK", 5)
        
        # Wait for remote user to end call
        handler.wait_for_event("remote_ended_call", 10)


class EndCallTest(TestRunner):
    def test_end_call(self):
        """Test ending an active call"""
        from test_runner_base import generate_unique_nickname
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        caller_nickname = generate_unique_nickname("caller")
        responder_nickname = generate_unique_nickname("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, responder_nickname, responder_events, "responder", partial(end_call_responder_scenario, caller_nickname=caller_nickname), 10)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, caller_nickname, caller_events, "caller", partial(end_call_caller_scenario, target_nickname=responder_nickname), 10)
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
