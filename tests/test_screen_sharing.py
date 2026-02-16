import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def screen_sharing_caller_scenario(client, handler, target_nickname):
    """Caller scenario: start call, then start screen sharing"""
    print(f"[{handler.name}] Starting call to {target_nickname}")
    client.start_outgoing_call(target_nickname)
    handler.wait_for_event("call_result_OK", 10)
    
    # Wait for call to be accepted
    if handler.wait_for_event("outgoing_call_accepted", 5):
        print(f"[{handler.name}] Call accepted, starting screen sharing")
        client.start_screen_sharing()
        handler.wait_for_event("start_screen_sharing_OK", 5)
        
        # Send some dummy screen data
        dummy_data = list([0x01, 0x02, 0x03, 0x04] * 100)
        client.send_screen(dummy_data)
        time.sleep(1)
        
        # Stop screen sharing
        client.stop_screen_sharing()
        handler.wait_for_event("stop_screen_sharing_OK", 5)


def screen_sharing_responder_scenario(client, handler, caller_nickname):
    """Responder scenario: accept call, receive screen sharing"""
    if handler.wait_for_event(f"incoming_call_{caller_nickname}", 10):
        print(f"[{handler.name}] Accepting call")
        client.accept_call(caller_nickname)
        handler.wait_for_event("accept_result_OK", 5)
        
        # Wait for incoming screen sharing
        if handler.wait_for_event("incoming_screen_sharing_started", 10):
            print(f"[{handler.name}] Received screen sharing")
            time.sleep(2)
            
            # Wait for screen sharing to stop
            handler.wait_for_event("incoming_screen_sharing_stopped", 5)


class ScreenSharingTest(TestRunner):
    def test_screen_sharing(self):
        """Test screen sharing during active call"""
        from test_runner_base import generate_unique_nickname
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        caller_nickname = generate_unique_nickname("caller")
        responder_nickname = generate_unique_nickname("responder")
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, responder_nickname, responder_events, "responder", partial(screen_sharing_responder_scenario, caller_nickname=caller_nickname), 10)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, caller_nickname, caller_events, "caller", partial(screen_sharing_caller_scenario, target_nickname=responder_nickname), 10)
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
        
        caller_started_sharing = "start_screen_sharing_OK" in caller_events
        responder_received_sharing = "incoming_screen_sharing_started" in responder_events
        
        return caller_started_sharing and responder_received_sharing


if __name__ == "__main__":
    test = ScreenSharingTest("8081")
    if test.start_server():
        try:
            result = test.test_screen_sharing()
            print(f"Screen sharing test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
