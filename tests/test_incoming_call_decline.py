import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def decline_caller_scenario(client, handler, target_nickname):
    """Caller scenario for decline test"""
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)


def decline_responder_scenario(client, handler, caller_nickname):
    """Responder scenario for decline test"""
    if handler.wait_for_event(f"incoming_call_{caller_nickname}", 10):
        print(f"[{handler.name}] Declining call")
        client.decline_call(caller_nickname)


class IncomingCallDeclineTest(TestRunner):
    def test_incoming_call_decline(self):
        """Test declining incoming call"""
        from test_runner_base import generate_unique_nickname
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        caller_nickname = generate_unique_nickname("caller")
        responder_nickname = generate_unique_nickname("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, responder_nickname, responder_events, "responder", partial(decline_responder_scenario, caller_nickname=caller_nickname))
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, caller_nickname, caller_events, "caller", partial(decline_caller_scenario, target_nickname=responder_nickname))
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
        
        return f"incoming_call_{caller_nickname}" in responder_events


if __name__ == "__main__":
    test = IncomingCallDeclineTest("8081")
    if test.start_server():
        try:
            result = test.test_incoming_call_decline()
            print(f"Incoming call decline test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

