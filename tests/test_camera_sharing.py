import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def camera_sharing_caller_scenario(client, handler):
    """Caller scenario: start call, then start camera sharing"""
    print(f"[{handler.name}] Starting call to responder")
    client.start_outgoing_call("responder")
    handler.wait_for_event("call_result_OK", 10)
    
    # Wait for call to be accepted
    if handler.wait_for_event("calling_accepted", 5):
        print(f"[{handler.name}] Call accepted, starting camera sharing")
        client.start_camera_sharing()
        handler.wait_for_event("start_camera_sharing_OK", 5)
        
        # Send some dummy camera data
        dummy_data = bytes([0x10, 0x20, 0x30, 0x40] * 100)
        client.send_camera(dummy_data)
        time.sleep(1)
        
        # Stop camera sharing
        client.stop_camera_sharing()
        handler.wait_for_event("stop_camera_sharing_OK", 5)


def camera_sharing_responder_scenario(client, handler):
    """Responder scenario: accept call, receive camera sharing"""
    if handler.wait_for_event("incoming_call_caller", 10):
        print(f"[{handler.name}] Accepting call")
        client.accept_call("caller")
        handler.wait_for_event("accept_result_OK", 5)
        
        # Wait for incoming camera sharing
        if handler.wait_for_event("incoming_camera_sharing_started", 10):
            print(f"[{handler.name}] Received camera sharing")
            time.sleep(2)
            
            # Wait for camera sharing to stop
            handler.wait_for_event("incoming_camera_sharing_stopped", 5)


class CameraSharingTest(TestRunner):
    def test_camera_sharing(self):
        """Test camera sharing during active call"""
        caller_events = multiprocessing.Manager().list()
        responder_events = multiprocessing.Manager().list()
        
        responder_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "responder", responder_events, "responder", camera_sharing_responder_scenario, 10)
        )
        
        caller_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "caller", caller_events, "caller", camera_sharing_caller_scenario, 10)
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
        
        caller_started_sharing = "start_camera_sharing_OK" in caller_events
        responder_received_sharing = "incoming_camera_sharing_started" in responder_events
        
        return caller_started_sharing and responder_received_sharing


if __name__ == "__main__":
    test = CameraSharingTest("8081")
    if test.start_server():
        try:
            result = test.test_camera_sharing()
            print(f"Camera sharing test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
