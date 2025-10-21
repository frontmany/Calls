import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def accept_caller_scenario(client, handler):
    """Caller scenario for accept test"""
    client.start_calling("responder")
    handler.wait_for_event("call_result_OK", 10)


def accept_responder_scenario(client, handler):
    """Responder scenario for accept test"""
    if handler.wait_for_event("incoming_call_caller", 10):
        print(f"[{handler.name}] Accepting call")
        client.accept_call("caller")
        handler.wait_for_event("accept_result_OK", 5)


class IncomingCallAcceptTest(TestRunner):
    def test_incoming_call_accept(self):
        """Test accepting incoming call"""
        caller_handler = CallbacksHandler("caller")
        responder_handler = CallbacksHandler("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "responder", responder_handler, accept_responder_scenario)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "caller", caller_handler, accept_caller_scenario)
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
        
        return "accept_result_OK" in responder_handler.events


if __name__ == "__main__":
    test = IncomingCallAcceptTest("8081")
    if test.start_server():
        try:
            result = test.test_incoming_call_accept()
            print(f"Incoming call accept test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

