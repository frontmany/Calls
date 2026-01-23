import sys
import time
import multiprocessing
from functools import partial

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def state_query_scenario(client, handler, expected_nickname):
    """Scenario: test various state query methods"""
    print(f"[{handler.name}] Testing state queries")
    
    # After authorization (already done in run_client_flexible)
    time.sleep(0.5)
    assert client.is_authorized(), "Should be authorized after auth"
    assert client.get_my_nickname() == expected_nickname, f"Nickname should be {expected_nickname}, got {client.get_my_nickname()}"
    assert client.get_incoming_calls_count() == 0, "Should have no incoming calls initially"
    assert len(client.get_callers()) == 0, "Should have no callers initially"
    assert not client.is_outgoing_call(), "Should not be in outgoing call"
    assert not client.is_active_call(), "Should not be in active call"
    assert not client.is_screen_sharing(), "Should not be screen sharing"
    assert not client.is_camera_sharing(), "Should not be camera sharing"
    assert not client.is_viewing_remote_screen(), "Should not be viewing remote screen"
    assert not client.is_viewing_remote_camera(), "Should not be viewing remote camera"
    assert not client.is_connection_down(), "Connection should be up"
    
    # Test audio state
    initial_input_vol = client.get_input_volume()
    initial_output_vol = client.get_output_volume()
    assert 0 <= initial_input_vol <= 100, "Input volume should be in range 0-100"
    assert 0 <= initial_output_vol <= 100, "Output volume should be in range 0-100"
    
    # Test mute states
    assert not client.is_microphone_muted(), "Microphone should not be muted initially"
    assert not client.is_speaker_muted(), "Speaker should not be muted initially"
    
    handler.events.append("state_queries_passed")


class StateQueriesTest(TestRunner):
    def test_state_queries(self):
        """Test all state query methods"""
        from test_runner_base import generate_unique_nickname
        events = multiprocessing.Manager().list()
        unique_nickname = generate_unique_nickname("state_test_user")
        
        process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, unique_nickname, events, "state_test_user", partial(state_query_scenario, expected_nickname=unique_nickname), 3)
        )
        
        process.start()
        process.join(timeout=10)
        
        if process.is_alive():
            process.terminate()
            return False
        
        return "state_queries_passed" in events


if __name__ == "__main__":
    test = StateQueriesTest("8081")
    if test.start_server():
        try:
            result = test.test_state_queries()
            print(f"State queries test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
