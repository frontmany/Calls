import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def auth_only_scenario(client, handler):
    """Just authorize and wait, no additional actions needed"""
    pass


class AuthorizationTest(TestRunner):
    def test_authorization(self):
        """Test single user authorization"""
        events_list = multiprocessing.Manager().list()
        
        process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "test_user_1", events_list, "auth_test", auth_only_scenario)
        )
        
        process.start()
        process.join(timeout=10)
        
        if process.is_alive():
            process.terminate()
            return False
        
        return "auth_result_OK" in events_list


if __name__ == "__main__":
    test = AuthorizationTest("8081")
    if test.start_server():
        try:
            result = test.test_authorization()
            print(f"Authorization test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

