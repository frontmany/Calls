import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def audio_test_scenario(client, handler):
    """Audio test scenario"""
    print(f"[{handler.name}] Testing audio controls")
    client.mute_microphone(True)
    client.mute_microphone(False)
    client.set_input_volume(80)
    client.set_output_volume(90)


class AudioControlsTest(TestRunner):
    def test_audio_controls(self):
        """Test audio controls"""
        events = multiprocessing.Manager().list()
        
        process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "audio_user", events, "audio_test", audio_test_scenario)
        )
        
        process.start()
        process.join(timeout=10)
        
        if process.is_alive():
            process.terminate()
        
        return "auth_result_OK" in events


if __name__ == "__main__":
    test = AudioControlsTest("8081")
    if test.start_server():
        try:
            result = test.test_audio_controls()
            print(f"Audio controls test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

